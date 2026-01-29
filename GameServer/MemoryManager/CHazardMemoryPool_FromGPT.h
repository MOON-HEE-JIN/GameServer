#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>
#include <stdexcept>

// Hazard Pointer + Retire/Reclaim 기반 단순 메모리 풀
// - Lock-free free-list (Treiber stack)
// - Hazard pointer로 pop 시 ABA/Use-after-free 방지
// - Retire list는 TLS (thread_local)로 보관 후 임계치마다 스캔/회수
//
// 주의/규약:
// 1) MAX_THREADS를 초과하는 스레드가 동시에 이 풀을 사용하면 예외/assert로 실패합니다.
// 2) 풀 파괴 시점에 다른 스레드가 여전히 노드를 참조할 수 있으면 안전한 delete가 불가능합니다.
//    따라서 ClearAfterAllThreadsJoined()는 "모든 작업 스레드가 종료(join)"된 뒤에만 호출해야 합니다.
// 3) 동일 T에 대해 여러 CHMemoryPool<T> 인스턴스를 운영하는 경우 retire TLS가 풀별 분리되지 않으므로 비권장.

template <typename T, int MAX_THREADS = 16, std::size_t RETIRE_THRESHOLD = 512>
class CHMemoryPool
{
    static_assert(MAX_THREADS > 0, "MAX_THREADS must be positive.");
    static_assert(RETIRE_THRESHOLD > 0, "RETIRE_THRESHOLD must be positive.");

private:
    struct Node
    {
        Node* next;
        // T 를 담기 위한 버퍼를 storge 하는데 이 정렬을 T에 대한 정렬로 맞춤
        alignas(T) std::byte storage[sizeof(T)];

        T* data_ptr() noexcept
        {
            // placement-new로 생성된 T에 접근할 때 launder 사용 권장
            return std::launder(reinterpret_cast<T*>(storage));
        }
        const T* data_ptr() const noexcept
        {
            return std::launder(reinterpret_cast<const T*>(storage));
        }
        /*
        std::launder 는 새로운 객체의 할당을 의미한다
        필요한 이유
        alloc 할때 placement new 를 통해서 생성자를 호출 함에 따라 객체의 생명주기의 시작을 의미한다
        free 할때 소멸자 호출을 통해서 객체의 종료를 의미한다
        이때 alloc 시 사용한주 를 다시 줄경우
        ex
            T* p = alloc;
            free(p);
            T* q = alloc;

        이에 따르면 컴파일러는 동일한 할당받은 주소를 사용하게 되는데 여기서 p 와 q 를 동일 객체로 여길수 있는 여지가 생긴다
        그렇다면 이전에 접근해서 얻은 p 에 대한 값을 유지한체 사용할수 있고 문제 가 발생한다

        std::lanunder 은 컴파일러 에게 해당 주소의 새로운 객체임을 알린다(할당X, 새로운 객체)
        */
    };

    // free-list head
    std::atomic<Node*> m_freeList{ nullptr };

    // 디버그/통계: 새로 할당된 노드 수, free-list로 회수된 노드 수
    std::atomic<std::uint64_t> m_debugNewNodes{ 0 };
    std::atomic<std::uint64_t> m_debugRecycledToFreeList{ 0 };

private:
    // =========================
    // Hazard Pointer infrastructure
    // =========================

    struct HazardRecord
    {
        std::atomic<std::uintptr_t> ownerToken; // 0이면 미사용
        std::atomic<Node*> protectedPtr;

        HazardRecord() : ownerToken(0), protectedPtr(nullptr) {}
    };

    static HazardRecord s_hazardRecs[MAX_THREADS];

    // 각 스레드가 자기 토큰을 갖고 hazard record를 1개 소유
    class HazardOwner
    {
    public:
        HazardOwner()
        {
            // 스레드별 고유 토큰: TLS 주소를 사용(프로세스 내 유일성 가정)
            // (표준적으로 "주소 유일성"은 사실상 보장되는 관용적 패턴)
            m_token = reinterpret_cast<std::uintptr_t>(&s_tlsTokenMarker);
            if (m_token == 0)
                m_token = 1;

            for (int i = 0; i < MAX_THREADS; ++i)
            {
                std::uintptr_t expected = 0;
                if (s_hazardRecs[i].ownerToken.compare_exchange_strong(
                    expected, m_token,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed))
                {
                    m_rec = &s_hazardRecs[i];
                    break;
                }
            }

            if (!m_rec)
            {
                // 스레드 슬롯 고갈: 명시적으로 실패
                throw std::runtime_error("CHMemoryPool: Hazard slot exhausted (MAX_THREADS exceeded).");
            }
        }

        ~HazardOwner()
        {
            // 스레드 종료 시점에 hazard pointer를 비우고 슬롯 반환
            if (m_rec)
            {
                m_rec->protectedPtr.store(nullptr, std::memory_order_release);
                m_rec->ownerToken.store(0, std::memory_order_release);
            }
        }

        std::atomic<Node*>& protected_ptr_ref() noexcept
        {
            assert(m_rec);
            return m_rec->protectedPtr;
        }

    private:
        HazardRecord* m_rec{ nullptr };
        std::uintptr_t m_token{ 0 };

        static inline thread_local int s_tlsTokenMarker = 0; // 토큰용 마커
    };

    static thread_local HazardOwner s_tlsHazOwner;

    static std::atomic<Node*>& tls_hazard_ptr() noexcept
    {
        return s_tlsHazOwner.protected_ptr_ref();
    }

    // retire list: 스레드 로컬
    static thread_local std::vector<Node*> s_tlsRetired;

private:
    // =========================
    // internal helpers
    // =========================

    static Node* node_from_data(T* p) noexcept
    {
        // Node는 T를 직접 멤버로 포함하지 않으므로 standard-layout이고,
        // storage의 오프셋은 안전하게 취급 가능
        std::byte* b = reinterpret_cast<std::byte*>(p);
        Node* n = reinterpret_cast<Node*>(b - offsetof(Node, storage));
        return n;
    }

    Node* pop_free_list()
    {
        // Treiber stack pop with hazard pointer
        while (true)
        {
            Node* head = m_freeList.load(std::memory_order_acquire);

            // 현재 head를 hazard로 보호
            tls_hazard_ptr().store(head, std::memory_order_release);

            // head가 바뀌었으면 다시
            if (head != m_freeList.load(std::memory_order_acquire))
            {
                // 재시도 전에 보호를 해제하지 않으면
                // 다른 스레드에서 해당 노드를 회수하지 못하게 되어
                // retire/reclaim이 지연될 수 있음.
                tls_hazard_ptr().store(nullptr, std::memory_order_release);
                continue;
            }
            

            if (!head)
            {
                tls_hazard_ptr().store(nullptr, std::memory_order_release);
                return nullptr;
            }

            Node* next = head->next;

            if (m_freeList.compare_exchange_weak(
                head, next,
                std::memory_order_acq_rel,
                std::memory_order_relaxed))
            {
                // pop 성공: 보호 해제
                tls_hazard_ptr().store(nullptr, std::memory_order_release);
                head->next = nullptr;
                return head;
            }

            // compare_exchange_weak 실패로 재시도하는 경우에도
            // 다음 반복 이전에 hazard를 해제하지 않으면 동일한 문제 발생 가능.
            // 여기서는 루프 상단에서 다시 보호를 설정하므로 명시적 해제는 선택적이지만,
            // 안전을 위해 명시적으로 해제.
            tls_hazard_ptr().store(nullptr, std::memory_order_release);
        }
    }

    void push_free_list(Node* n) noexcept
    {
        Node* head = m_freeList.load(std::memory_order_relaxed);
        do
        {
            n->next = head;
        } while (!m_freeList.compare_exchange_weak(
            head, n,
            std::memory_order_release,
            std::memory_order_relaxed));

        m_debugRecycledToFreeList.fetch_add(1, std::memory_order_relaxed);
    }

    static void collect_hazards(std::vector<Node*>& out)
    {
        out.clear();
        out.reserve(MAX_THREADS);

        for (int i = 0; i < MAX_THREADS; ++i)
        {
            Node* p = s_hazardRecs[i].protectedPtr.load(std::memory_order_acquire);
            if (p)
                out.push_back(p);
        }

        // 스캔 비용을 줄이기 위해 정렬 후 이진 탐색
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
    }

    void reclaim_if_possible()
    {
        std::vector<Node*> hazards;
        collect_hazards(hazards);

        auto& retired = s_tlsRetired;
        std::size_t write = 0;

        for (std::size_t i = 0; i < retired.size(); ++i)
        {
            Node* n = retired[i];

            bool inUse = std::binary_search(hazards.begin(), hazards.end(), n);
            if (!inUse)
            {
                // 다른 스레드가 보호하지 않는 노드는 free-list로 회수
                push_free_list(n);
            }
            else
            {
                // 아직 사용 중이면 유지
                retired[write++] = n;
            }
        }

        retired.resize(write);
    }

    Node* acquire_node_raw()
    {
        if (Node* n = pop_free_list())
            return n;

        m_debugNewNodes.fetch_add(1, std::memory_order_relaxed);
        Node* n = static_cast<Node*>(::operator new(sizeof(Node)));
        n->next = nullptr;
        return n;
    }

    void retire_node(Node* n)
    {
        n->next = nullptr;
        s_tlsRetired.push_back(n);

        if (s_tlsRetired.size() >= RETIRE_THRESHOLD)
            reclaim_if_possible();
    }

public:
    CHMemoryPool() = default;

    // 소멸자는 안전한 전역 정리를 자동으로 할 수 없습니다.
    // (다른 스레드가 여전히 접근할 수 있는지 판단 불가 + 다른 스레드 TLS retire 접근 불가)
    // 따라서 명시적 정리 함수 제공.
    ~CHMemoryPool() = default;

    std::uint64_t DebugNewNodes() const noexcept
    {
        return m_debugNewNodes.load(std::memory_order_relaxed);
    }

    std::uint64_t DebugRecycledToFreeList() const noexcept
    {
        return m_debugRecycledToFreeList.load(std::memory_order_relaxed);
    }

    // -------------------------
    // API 1) Emplace/Destroy (권장)
    // -------------------------
    template <class... Args>
    T* Emplace(Args&&... args)
    {
        Node* n = acquire_node_raw();
        try
        {
            ::new (static_cast<void*>(n->storage)) T(std::forward<Args>(args)...);
        }
        catch (...)
        {
            // 생성 실패 시 노드는 바로 free-list에 돌려놓음
            push_free_list(n);
            throw;
        }
        return n->data_ptr();
    }

    void Destroy(T* p)
    {
        if (!p) return;

        Node* n = node_from_data(p);
        // T 소멸
        p->~T();
        // retire
        retire_node(n);
    }

    // -------------------------
    // API 2) Alloc/Free (호환)
    // - Alloc(): 기본 생성
    // - Free(): 소멸 + retire
    // -------------------------
    T* Alloc()
    {
        return Emplace();
    }

    void Free(T* p)
    {
        Destroy(p);
    }

    // -------------------------
    // Reclaim/clear utilities
    // -------------------------

    // 현재 스레드의 retire list에 대해서만 강제로 reclaim 시도
    void ForceReclaimCurrentThread()
    {
        reclaim_if_possible();
    }

    // 모든 작업 스레드가 종료(join)된 뒤에만 호출해야 합니다.
    // - hazard pointers는 스레드 종료 시 ~HazardOwner에서 해제되며,
    // - 각 스레드 retire list는 "그 스레드가 ForceReclaimCurrentThread()를 마지막에 호출"하거나,
    //   아니면 노드가 남아 있을 수 있습니다.
    //
    // 이 함수는 "현재 스레드 기준"으로 free-list를 전부 delete합니다.
    // (여전히 다른 스레드 TLS retire에 남아 있는 노드는 여기서 접근 불가)
    void ClearAfterAllThreadsJoined()
    {
        // 남은 retire를 최대한 free-list로 돌림(현재 스레드 것만)
        ForceReclaimCurrentThread();

        // free-list를 전부 파괴
        Node* head = m_freeList.exchange(nullptr, std::memory_order_acq_rel);
        while (head)
        {
            Node* next = head->next;
            ::operator delete(static_cast<void*>(head));
            head = next;
        }

        // 현재 스레드 retire list에 남아 있는 노드는
        // "아직 hazard로 보호되고 있다"는 뜻이므로,
        // join 이후라면 보통 남지 않아야 정상입니다.
        // 남아 있다면 사용자 코드가 마지막 reclaim을 호출하지 않았거나,
        // 스레드가 아닌 곳에서 여전히 접근 중인 버그 가능성이 큽니다.
        assert(s_tlsRetired.empty() && "Retired nodes remain in current thread. Call ForceReclaimCurrentThread() at thread shutdown.");
    }
};

// ============ static storage ============

template <typename T, int MAX_THREADS, std::size_t RETIRE_THRESHOLD>
typename CHMemoryPool<T, MAX_THREADS, RETIRE_THRESHOLD>::HazardRecord
CHMemoryPool<T, MAX_THREADS, RETIRE_THRESHOLD>::s_hazardRecs[MAX_THREADS];

template <typename T, int MAX_THREADS, std::size_t RETIRE_THRESHOLD>
thread_local typename CHMemoryPool<T, MAX_THREADS, RETIRE_THRESHOLD>::HazardOwner
CHMemoryPool<T, MAX_THREADS, RETIRE_THRESHOLD>::s_tlsHazOwner;

template <typename T, int MAX_THREADS, std::size_t RETIRE_THRESHOLD>
thread_local std::vector<typename CHMemoryPool<T, MAX_THREADS, RETIRE_THRESHOLD>::Node*>
CHMemoryPool<T, MAX_THREADS, RETIRE_THRESHOLD>::s_tlsRetired;

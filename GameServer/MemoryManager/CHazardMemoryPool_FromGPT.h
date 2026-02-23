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

// Hazard Pointer + Retire/Reclaim 湲곕 ⑥ 硫紐⑤━ 
// - Lock-free free-list (Treiber stack)
// - Hazard pointer濡 pop  ABA/Use-after-free 諛⑹
// - Retire list TLS (thread_local)濡 蹂닿  怨移留 ㅼ/
//
// 二쇱/洹:
// 1) MAX_THREADS瑜 珥怨쇳 ㅻ媛    ъ⑺硫 /assert濡 ㅽ⑦⑸.
// 2)  愿 � ㅻⅨ ㅻ媛 ъ 몃瑜 李몄“  쇰㈃ � delete媛 遺媛ν⑸.
//    곕쇱 ClearAfterAllThreadsJoined() "紐⑤  ㅻ媛 醫猷(join)" ㅼ留 몄댁 ⑸.
// 3)  T  щ CHMemoryPool<T> 몄ㅽ댁ㅻ� 댁 寃쎌 retire TLS媛 蹂 遺由щ吏 쇰濡 鍮沅.

template <typename T, int MAX_THREADS = 16, std::size_t RETIRE_THRESHOLD = 512>
class CHMemoryPool
{
    static_assert(MAX_THREADS > 0, "MAX_THREADS must be positive.");
    static_assert(RETIRE_THRESHOLD > 0, "RETIRE_THRESHOLD must be positive.");

private:
    struct Node
    {
        Node* next;
        // T 瑜 닿린  踰쇰� storge   ��ъ T  ��щ 留異
        alignas(T) std::byte storage[sizeof(T)];

        T* data_ptr() noexcept
        {
            // placement-new濡 깅 T �洹쇳  launder ъ 沅
            return std::launder(reinterpret_cast<T*>(storage));
        }
        const T* data_ptr() const noexcept
        {
            return std::launder(reinterpret_cast<const T*>(storage));
        }
        /*
        std::launder  濡 媛泥댁 뱀 誘명
         댁
        alloc  placement new 瑜 듯댁 깆瑜 몄 ⑥ 곕 媛泥댁 紐二쇨린  誘명
        free  硫몄 몄 듯댁 媛泥댁 醫猷瑜 誘명
        대 alloc  ъ⑺二 瑜 ㅼ 以寃쎌
        ex
            T* p = alloc;
            free(p);
            T* q = alloc;

        댁 곕Ⅴ硫 而댄쇰щ 쇳 밸 二쇱瑜 ъ⑺寃  ш린 p  q 瑜  媛泥대 ш만  ъ媛 湲대
        洹몃ㅻ㈃ 댁 �洹쇳댁 살 p   媛 吏泥 ъ⑺ 怨 臾몄 媛 諛

        std::lanunder  而댄쇰 寃 대 二쇱 濡 媛泥댁 由곕(X, 濡 媛泥)
        */
    };

    // free-list head
    std::atomic<Node*> m_freeList{ nullptr };

    // 踰洹/듦: 濡 밸 몃 , free-list濡  몃 
    std::atomic<std::uint64_t> m_debugNewNodes{ 0 };
    std::atomic<std::uint64_t> m_debugRecycledToFreeList{ 0 };

    // main 종료후 이미 삭제된 TLS 구역 접근 안하기
    std::atomic<bool> m_bShutdowning = false;
private:
    // =========================
    // Hazard Pointer infrastructure
    // =========================

    struct HazardRecord
    {
        std::atomic<std::uintptr_t> ownerToken; // 0대㈃ 誘몄ъ
        std::atomic<Node*> protectedPtr;

        HazardRecord() : ownerToken(0), protectedPtr(nullptr) {}
    };

    static HazardRecord s_hazardRecs[MAX_THREADS];

    // 媛 ㅻ媛 湲 곗 媛怨 hazard record瑜 1媛 
    class HazardOwner
    {
    public:
        HazardOwner()
        {
            // ㅻ蹂 怨 : TLS 二쇱瑜 ъ(濡몄  쇱 媛�)
            // (以�쇰 "二쇱 쇱" ъㅼ 蹂댁λ 愿⑹ ⑦)
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
                // ㅻ щ’ 怨媛: 紐�쇰 ㅽ
                throw std::runtime_error("CHMemoryPool: Hazard slot exhausted (MAX_THREADS exceeded).");
            }
        }

        ~HazardOwner()
        {
            // ㅻ 醫猷 � hazard pointer瑜 鍮곌� щ’ 諛
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

        static inline thread_local int s_tlsTokenMarker = 0; // 곗 留而
    };

    static thread_local HazardOwner s_tlsHazOwner;

    static std::atomic<Node*>& tls_hazard_ptr() noexcept
    {
        return s_tlsHazOwner.protected_ptr_ref();
    }

    // retire list: ㅻ 濡而
    static thread_local std::vector<Node*> s_tlsRetired;

private:
    // =========================
    // internal helpers
    // =========================

    static Node* node_from_data(T* p) noexcept
    {
        // Node T瑜 吏� 硫ㅻ濡 ы⑦吏 쇰濡 standard-layout닿�,
        // storage ㅽ �寃 痍④ 媛
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

            //  head瑜 hazard濡 蹂댄
            tls_hazard_ptr().store(head, std::memory_order_release);

            // head媛 諛쇰㈃ ㅼ
            if (head != m_freeList.load(std::memory_order_acquire))
            {
                // ъ � 蹂댄몃� 댁吏 쇰㈃
                // ㅻⅨ ㅻ 대 몃瑜 吏 紐삵寃 
                // retire/reclaim 吏곕  .
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
                // pop 깃났: 蹂댄 댁
                tls_hazard_ptr().store(nullptr, std::memory_order_release);
                head->next = nullptr;
                return head;
            }

            // compare_exchange_weak ㅽ⑤ ъ 寃쎌곗
            // ㅼ 諛蹂 댁 hazard瑜 댁吏 쇰㈃ 쇳 臾몄 諛 媛.
            // ш린 猷⑦ ⑥ ㅼ 蹂댄몃� ㅼ誘濡 紐� 댁 �댁留,
            // �  紐�쇰 댁.
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

        // ㅼ 鍮⑹ 以닿린  ��  댁 
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
                // ㅻⅨ ㅻ媛 蹂댄명吏  몃 free-list濡 
                push_free_list(n);
            }
            else
            {
                // 吏 ъ 以대㈃ 吏
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
<<<<<<< develop
        // ShutDown  s_tlsRetired 媛 癒쇱 愿댄 LF_Queue 瑜 愿대 명
        // push_back  ㅻ 諛
=======
        // ShutDown 시 s_tlsRetired 가 먼저 파괴후에 LF_Queue 를 파괴로 인해
        // push_back 에서 오류 발생
>>>>>>> main
        s_tlsRetired.push_back(n);

        if (s_tlsRetired.size() >= RETIRE_THRESHOLD)
            reclaim_if_possible();
    }

public:
    CHMemoryPool() = default;

    // 硫몄 � � �由щ� 쇰   듬.
    // (ㅻⅨ ㅻ媛 ъ �洹쇳  吏  遺媛 + ㅻⅨ ㅻ TLS retire �洹 遺媛)
    // 곕쇱 紐� �由 ⑥ �怨.
    ~CHMemoryPool() = default;

    std::uint64_t DebugNewNodes() const noexcept
    {
        return m_debugNewNodes.load(std::memory_order_relaxed);
    }

    std::uint64_t DebugRecycledToFreeList() const noexcept
    {
        return m_debugRecycledToFreeList.load(std::memory_order_relaxed);
    }

    void BeginShutdown() { m_bShutdowning.store(true); }

    // -------------------------
    // API 1) Emplace/Destroy (沅)
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
            //  ㅽ  몃 諛濡 free-list �ㅻ
            push_free_list(n);
            throw;
        }
        return n->data_ptr();
    }

    void Destroy(T* p)
    {
        if (!p) return;

        Node* n = node_from_data(p);
        // T 硫
        p->~T();

        if (m_bShutdowning.load())
        {
            push_free_list(n);
            return;
        }

        // retire
        retire_node(n);
    }

    // -------------------------
    // API 2) Alloc/Free (명)
    // - Alloc(): 湲곕낯 
    // - Free(): 硫 + retire
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

    //  ㅻ retire list 댁留 媛�濡 reclaim 
    void ForceReclaimCurrentThread()
    {
        reclaim_if_possible();
    }

    // 紐⑤  ㅻ媛 醫猷(join) ㅼ留 몄댁 ⑸.
    // - hazard pointers ㅻ 醫猷  ~HazardOwner 댁硫,
    // - 媛 ㅻ retire list "洹 ㅻ媛 ForceReclaimCurrentThread()瑜 留吏留 몄"嫄곕,
    //   硫 몃媛 ⑥   듬.
    //
    //  ⑥ " ㅻ 湲곗"쇰 free-list瑜 �遺 delete⑸.
    // (ъ ㅻⅨ ㅻ TLS retire ⑥  몃 ш린 �洹 遺媛)
    void ClearAfterAllThreadsJoined()
    {
        // ⑥ retire瑜 理 free-list濡 由( ㅻ 寃留)
        ForceReclaimCurrentThread();

        // free-list瑜 �遺 愿
        Node* head = m_freeList.exchange(nullptr, std::memory_order_acq_rel);
        while (head)
        {
            Node* next = head->next;
            ::operator delete(static_cast<void*>(head));
            head = next;
        }

        //  ㅻ retire list ⑥  몃
        // "吏 hazard濡 蹂댄몃怨 " 살대濡,
        // join 댄쇰㈃ 蹂댄 ⑥  �.
        // ⑥ ㅻ㈃ ъ⑹ 肄媛 留吏留 reclaim 몄吏 嫄곕,
        // ㅻ媛  怨녹 ъ �洹 以 踰洹 媛μ깆 쎈.
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


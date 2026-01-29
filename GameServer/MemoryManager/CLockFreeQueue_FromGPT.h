#pragma once

#include <atomic>
#include <thread>
#include <utility>   // std::move
#include <type_traits>

#if defined(_MSC_VER)
#include <immintrin.h> // _mm_pause
#endif

#include "CHazardMemoryPool_FromGPT.h"

// MPSC: Multi-Producer Single-Consumer queue
// - Producers: Enqueue only
// - Consumer : TryDequeue only (single thread)
// - Memory   : CHMemoryPool<T> (Hazard Pointer retire/reclaim)

template<typename T, int MAX_THREADS = 16, std::size_t RETIRE_THRESHOLD = 512>
class CLockFreeQueue_MPSC
{
private:
    struct Node
    {
        std::atomic<Node*> next{ nullptr };
        T data;

        Node() = default;
        explicit Node(const T& v) : next(nullptr), data(v) {}
        explicit Node(T&& v) : next(nullptr), data(std::move(v)) {}
    };

    CHMemoryPool<Node, MAX_THREADS, RETIRE_THRESHOLD> m_pool;

    // Producers share this
    std::atomic<Node*> m_tail{ nullptr };

    // Single consumer owns this (not atomic)
    Node* m_head{ nullptr }; // dummy node

public:
    CLockFreeQueue_MPSC()
    {
        Node* dummy = m_pool.Alloc();
        dummy->next.store(nullptr, std::memory_order_relaxed);
        m_head = dummy;
        m_tail.store(dummy, std::memory_order_relaxed);
    }

    ~CLockFreeQueue_MPSC()
    {
        // 전제: 파괴 시점에 producer/consumer 스레드가 모두 중지(join)된 상태여야 안전
        T out;
        while (TryDequeue(out)) {}

        // 마지막 dummy 회수
        m_pool.Free(m_head);

        // 현재 스레드의 retire 정리(권장)
        m_pool.ForceReclaimCurrentThread();

        // 모든 스레드가 이미 종료(join)된 상태라면 추가로 호출 가능
        // m_pool.ClearAfterAllThreadsJoined();
    }

    CLockFreeQueue_MPSC(const CLockFreeQueue_MPSC&) = delete;
    CLockFreeQueue_MPSC& operator=(const CLockFreeQueue_MPSC&) = delete;

public:
    // Producer: copy enqueue
    void Enqueue(const T& v)
    {
        Node* n = m_pool.Alloc();
        // placement-new 형태가 아니라 pool이 raw Node를 주므로 대입으로 초기화
        n->data = v;
        n->next.store(nullptr, std::memory_order_relaxed);

        // 1) tail을 새 노드로 교체 (멀티 producer 경쟁 지점)
        Node* prev = m_tail.exchange(n, std::memory_order_acq_rel);

        // 2) prev->next에 새 노드를 publish (consumer는 acquire로 읽음)
        prev->next.store(n, std::memory_order_release);
    }

    // Producer: move enqueue
    void Enqueue(T&& v)
    {
        Node* n = m_pool.Alloc();
        n->data = std::move(v);
        n->next.store(nullptr, std::memory_order_relaxed);

        Node* prev = m_tail.exchange(n, std::memory_order_acq_rel);
        prev->next.store(n, std::memory_order_release);
    }

    // Consumer(단일): pop
    // empty면 false, 성공하면 out 채우고 true
    bool TryDequeue(T& out)
    {
        Node* h = m_head;
        Node* next = h->next.load(std::memory_order_acquire);

        if (next == nullptr)
        {
            // 중요: producer가 tail은 바꿨는데 prev->next publish가 아직일 수 있음.
            // head != tail 이면 "진짜 empty"가 아님 → 잠깐 대기 후 재확인
            if (m_tail.load(std::memory_order_acquire) != h)
            {
                do
                {
#if defined(_MSC_VER)
                    _mm_pause();
#else
                    std::this_thread::yield();
#endif
                    next = h->next.load(std::memory_order_acquire);
                } while (next == nullptr);
            }
            else
            {
                return false; // 진짜 empty
            }
        }

        // next는 실제 데이터 노드
        out = std::move(next->data);

        // dummy를 한 칸 전진: next가 새 dummy가 됨
        m_head = next;

        // 이전 dummy(h)는 consumer가 단독으로 retire/free
        m_pool.Free(h);
        return true;
    }

    // 디버그/관찰용(정확 size는 MPSC에서도 비용/정확성 문제가 있어 보통 안 둠)
    int GetMemoryPoolSize() { return m_pool.GetAllocCount(); }

    // 필요 시: consumer 스레드에서 주기적으로 호출해 reclaim 압박 줄이기
    void ForceReclaimOnThisThread()
    {
        m_pool.ForceReclaimCurrentThread();
    }
};

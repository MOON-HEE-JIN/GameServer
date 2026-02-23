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
        m_pool.BeginShutdown();
        // 전제: 파괴 시점에 producer/consumer 스레드가 모두 중지(join)된 상태여야 안전
        T out;
        while (TryDequeue(out)) {}

        // 留吏留 dummy 
        m_pool.Free(m_head);
    }

    CLockFreeQueue_MPSC(const CLockFreeQueue_MPSC&) = delete;
    CLockFreeQueue_MPSC& operator=(const CLockFreeQueue_MPSC&) = delete;

public:
    // Producer: copy enqueue
    void Enqueue(const T& v)
    {
        Node* n = m_pool.Alloc();
        // placement-new 媛  pool raw Node瑜 二쇰濡 쇰 珥湲고
        n->data = v;
        n->next.store(nullptr, std::memory_order_relaxed);

        // 1) tail  몃濡 援泥 (硫 producer 寃쎌 吏�)
        Node* prev = m_tail.exchange(n, std::memory_order_acq_rel);

        // 2) prev->next  몃瑜 publish (consumer acquire濡 쎌)
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

    // Consumer(⑥): pop
    // empty硫 false, 깃났硫 out 梨곌� true
    bool TryDequeue(T& out)
    {
        Node* h = m_head;
        Node* next = h->next.load(std::memory_order_acquire);

        if (next == nullptr)
        {
            // 以: producer媛 tail 諛轅⑤ prev->next publish媛 吏  .
            // head != tail 대㈃ "吏吏 empty"媛   源 湲  ы
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
                return false; // 吏吏 empty
            }
        }

        // next ㅼ 곗댄 몃
        out = std::move(next->data);

        // dummy瑜  移 �吏: next媛  dummy媛 
        m_head = next;

        // 댁 dummy(h) consumer媛 ⑤쇰 retire/free
        m_pool.Free(h);
        return true;
    }

    // 踰洹/愿李곗(� size MPSC 鍮/� 臾몄媛  蹂댄  )
    int GetMemoryPoolSize() { return m_pool.GetAllocCount(); }

    //  : consumer ㅻ 二쇨린�쇰 몄 reclaim 諛 以닿린
    void ForceReclaimOnThisThread()
    {
        m_pool.ForceReclaimCurrentThread();
    }
};


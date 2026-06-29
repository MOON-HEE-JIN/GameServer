#pragma once

#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

// SPSC(Single Producer Single Consumer) 전용 Lock-Free Queue
// - Producer Thread 1개만 Enqueue 호출
// - Consumer Thread 1개만 TryDequeue 호출
// - 메모리풀을 사용하지 않고 new/delete로 노드를 할당/해제
// - 여러 Producer/Consumer가 동시에 접근하는 용도로는 사용하면 안 됨

template<typename T>
class CLockFreeQueue_SPSC
{
private:
    struct Node
    {
        std::atomic<Node*> next;
        bool hasData;
        typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;

        Node() noexcept
            : next(nullptr)
            , hasData(false)
        {
        }

        explicit Node(const T& value)
            : next(nullptr)
            , hasData(true)
        {
            new (&storage) T(value);
        }

        explicit Node(T&& value)
            : next(nullptr)
            , hasData(true)
        {
            new (&storage) T(std::move(value));
        }

        ~Node()
        {
            DestroyData();
        }

        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;

        T& Data() noexcept
        {
            return *reinterpret_cast<T*>(&storage);
        }

        void DestroyData() noexcept
        {
            if (hasData)
            {
                Data().~T();
                hasData = false;
            }
        }
    };

    // Consumer 전용 포인터입니다. 현재 dummy node를 가리킵니다.
    alignas(64) Node* m_head;

    // Producer 전용 포인터입니다. 마지막 node를 가리킵니다.
    alignas(64) Node* m_tail;

    // 디버그/모니터링용 크기입니다. 큐 동작의 선형화에는 사용하지 않습니다.
    alignas(64) std::atomic<std::size_t> m_size;

public:
    CLockFreeQueue_SPSC()
        : m_head(new Node())
        , m_tail(m_head)
        , m_size(0)
    {
    }

    ~CLockFreeQueue_SPSC()
    {
        Clear();
        delete m_head;
        m_head = nullptr;
        m_tail = nullptr;
    }

    CLockFreeQueue_SPSC(const CLockFreeQueue_SPSC&) = delete;
    CLockFreeQueue_SPSC& operator=(const CLockFreeQueue_SPSC&) = delete;

    CLockFreeQueue_SPSC(CLockFreeQueue_SPSC&&) = delete;
    CLockFreeQueue_SPSC& operator=(CLockFreeQueue_SPSC&&) = delete;

    // Producer 전용: 복사 삽입
    void Enqueue(const T& value)
    {
        PushNode(new Node(value));
    }

    // Producer 전용: 이동 삽입
    void Enqueue(T&& value)
    {
        PushNode(new Node(std::move(value)));
    }

    // Consumer 전용: 성공하면 out에 값을 이동 대입하고 true를 반환합니다.
    // 비어 있으면 false를 반환합니다.
    bool TryDequeue(T& out)
    {
        Node* const head = m_head;
        Node* const next = head->next.load(std::memory_order_acquire);

        if (next == nullptr)
        {
            return false;
        }

        out = std::move(next->Data());
        next->DestroyData();

        // 다음 노드를 새 dummy node로 사용하고, 기존 dummy node를 해제합니다.
        m_head = next;
        delete head;

        m_size.fetch_sub(1, std::memory_order_relaxed);
        return true;
    }

    // Consumer 전용 확인 함수입니다.
    bool IsEmpty() const noexcept
    {
        return m_head->next.load(std::memory_order_acquire) == nullptr;
    }

    // 모니터링용 크기입니다. SPSC에서는 정확한 값에 가깝지만,
    // 다른 스레드가 동시에 읽는 순간에는 참고용으로만 사용하세요.
    std::size_t GetSize() const noexcept
    {
        return m_size.load(std::memory_order_relaxed);
    }

    // Consumer 전용: 현재 큐에 남은 모든 값을 버립니다.
    void Clear()
    {
        Node* node = nullptr;
        while ((node = m_head->next.load(std::memory_order_acquire)) != nullptr)
        {
            m_head->next.store(node->next.load(std::memory_order_relaxed), std::memory_order_relaxed);
            node->DestroyData();
            delete node;
        }

        m_tail = m_head;
        m_size.store(0, std::memory_order_relaxed);
    }

private:
    void PushNode(Node* node)
    {
        node->next.store(nullptr, std::memory_order_relaxed);

        // Producer가 data 생성까지 끝낸 뒤 next를 publish합니다.
        m_tail->next.store(node, std::memory_order_release);
        m_tail = node;

        m_size.fetch_add(1, std::memory_order_relaxed);
    }
};
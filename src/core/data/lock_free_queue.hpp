#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <stdexcept>

namespace netsentry {
namespace data {

template <typename T>
class LockFreeQueue {
private:
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;

        Node() : next(nullptr) {}

        explicit Node(const T& value) : data(std::make_shared<T>(value)), next(nullptr) {}
        explicit Node(T&& value) : data(std::make_shared<T>(std::move(value))), next(nullptr) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    std::atomic<size_t> size_;

public:
    LockFreeQueue() : size_(0) {
        Node* dummy = new Node();
        head_.store(dummy);
        tail_.store(dummy);
    }

    ~LockFreeQueue() {
        while (pop()) {}

        Node* dummy = head_.load();
        delete dummy;
    }

    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;

    void push(const T& value) {
        Node* new_node = new Node(value);
        push_node(new_node);
    }

    void push(T&& value) {
        Node* new_node = new Node(std::move(value));
        push_node(new_node);
    }

    std::optional<T> pop() {
        Node* old_head = head_.load();
        Node* next_node = nullptr;

        do {
            next_node = old_head->next.load();
            if (!next_node) {
                return std::nullopt;
            }
        } while (!head_.compare_exchange_weak(old_head, next_node));

        std::shared_ptr<T> result = next_node->data;
        delete old_head;

        size_.fetch_sub(1, std::memory_order_relaxed);
        return *result;
    }

    size_t size() const {
        return size_.load(std::memory_order_relaxed);
    }

    bool empty() const {
        return size() == 0;
    }

private:
    void push_node(Node* new_node) {
        Node* old_tail;
        Node* null_node = nullptr;

        while (true) {
            old_tail = tail_.load();
            Node* next = old_tail->next.load();

            if (old_tail == tail_.load()) {
                if (next == nullptr) {
                    if (old_tail->next.compare_exchange_weak(null_node, new_node)) {
                        break;
                    }
                } else {
                    tail_.compare_exchange_weak(old_tail, next);
                }
            }
        }

        tail_.compare_exchange_weak(old_tail, new_node);
        size_.fetch_add(1, std::memory_order_relaxed);
    }
};

}
}

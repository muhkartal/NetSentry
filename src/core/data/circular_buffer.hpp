#pragma once

#include <array>
#include <mutex>
#include <optional>
#include <atomic>
#include <type_traits>

namespace netsentry {
namespace data {

template <typename T, size_t Capacity>
class CircularBuffer {
public:
    CircularBuffer() : head_(0), tail_(0), size_(0) {}

    bool push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (size_ == Capacity) {
            return false;
        }

        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % Capacity;
        ++size_;

        return true;
    }

    template <typename U = T>
    bool push(T&& item, typename std::enable_if<std::is_move_constructible<U>::value>::type* = nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (size_ == Capacity) {
            return false;
        }

        buffer_[tail_] = std::move(item);
        tail_ = (tail_ + 1) % Capacity;
        ++size_;

        return true;
    }

    std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (size_ == 0) {
            return std::nullopt;
        }

        T item = std::move(buffer_[head_]);
        head_ = (head_ + 1) % Capacity;
        --size_;

        return item;
    }

    std::optional<std::reference_wrapper<const T>> peek() const {
        std::lock_guard<std::mutex> lock(mutex_);

        if (size_ == 0) {
            return std::nullopt;
        }

        return std::cref(buffer_[head_]);
    }

    size_t size() const {
        std::atomic_thread_fence(std::memory_order_acquire);
        return size_;
    }

    bool empty() const {
        return size() == 0;
    }

    bool full() const {
        return size() == Capacity;
    }

private:
    std::array<T, Capacity> buffer_;
    size_t head_;
    size_t tail_;
    std::atomic<size_t> size_;
    mutable std::mutex mutex_;
};

}
}

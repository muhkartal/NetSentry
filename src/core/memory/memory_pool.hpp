#pragma once

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <new>

namespace netsentry {
namespace memory {

template <size_t BlockSize, size_t BlocksPerChunk = 128>
class MemoryPool {
public:
    MemoryPool() : free_blocks_(nullptr) {}

    ~MemoryPool() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (Block* chunk : chunks_) {
            free(chunk);
        }
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    void* allocate() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (free_blocks_ == nullptr) {
            allocateChunk();
        }

        Block* block = free_blocks_;
        free_blocks_ = free_blocks_->next;

        return block;
    }

    void deallocate(void* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(mutex_);

        Block* block = static_cast<Block*>(ptr);

        if (!isAddressInPool(block)) {
            throw std::invalid_argument("Pointer was not allocated from this pool");
        }

        block->next = free_blocks_;
        free_blocks_ = block;
    }

    size_t getBlockSize() const { return BlockSize; }
    size_t getChunkSize() const { return BlocksPerChunk; }

private:
    struct Block {
        union {
            Block* next;
            uint8_t data[BlockSize];
        };
    };

    Block* free_blocks_;
    std::vector<Block*> chunks_;
    std::mutex mutex_;

    void allocateChunk() {
        size_t alloc_size = sizeof(Block) * BlocksPerChunk;
        Block* new_chunk = static_cast<Block*>(aligned_alloc(std::max<size_t>(alignof(Block), 16), alloc_size));

        if (!new_chunk) {
            throw std::bad_alloc();
        }

        chunks_.push_back(new_chunk);

        for (size_t i = 0; i < BlocksPerChunk - 1; ++i) {
            new_chunk[i].next = &new_chunk[i + 1];
        }

        new_chunk[BlocksPerChunk - 1].next = nullptr;
        free_blocks_ = new_chunk;
    }

    bool isAddressInPool(Block* block) const {
        for (Block* chunk : chunks_) {
            if (block >= chunk && block < chunk + BlocksPerChunk) {
                return (block - chunk) % sizeof(Block) == 0;
            }
        }
        return false;
    }
};

template <typename T, size_t BlocksPerChunk = 128>
class ObjectPool {
public:
    template <typename... Args>
    T* allocate(Args&&... args) {
        void* memory = pool_.allocate();
        return new(memory) T(std::forward<Args>(args)...);
    }

    void deallocate(T* object) {
        if (!object) return;

        object->~T();
        pool_.deallocate(object);
    }

private:
    MemoryPool<sizeof(T), BlocksPerChunk> pool_;
};

template <typename T>
class PoolAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    struct rebind {
        using other = PoolAllocator<U>;
    };

    PoolAllocator() = default;

    template <typename U>
    PoolAllocator(const PoolAllocator<U>&) {}

    pointer allocate(size_type n) {
        if (n == 1) {
            return static_cast<pointer>(getPool().allocate());
        } else {
            return static_cast<pointer>(::operator new(n * sizeof(T)));
        }
    }

    void deallocate(pointer p, size_type n) {
        if (n == 1) {
            getPool().deallocate(p);
        } else {
            ::operator delete(p);
        }
    }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        ::new(static_cast<void*>(p)) U(std::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p) {
        p->~U();
    }

private:
    static ObjectPool<T>& getPool() {
        static ObjectPool<T> pool;
        return pool;
    }
};

template <typename T, typename U>
bool operator==(const PoolAllocator<T>&, const PoolAllocator<U>&) {
    return true;
}

template <typename T, typename U>
bool operator!=(const PoolAllocator<T>&, const PoolAllocator<U>&) {
    return false;
}

}
}

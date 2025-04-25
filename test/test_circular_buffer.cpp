#include "catch2/catch.hpp"
#include "../src/core/data/circular_buffer.hpp"
#include <string>
#include <vector>
#include <thread>
#include <atomic>

using namespace netsentry::data;

TEST_CASE("CircularBuffer basic operations", "[circular_buffer]") {
    CircularBuffer<int, 5> buffer;

    SECTION("Initial state is empty") {
        REQUIRE(buffer.empty());
        REQUIRE_FALSE(buffer.full());
        REQUIRE(buffer.size() == 0);
    }

    SECTION("Push adds elements") {
        REQUIRE(buffer.push(1));
        REQUIRE(buffer.size() == 1);
        REQUIRE_FALSE(buffer.empty());
        REQUIRE_FALSE(buffer.full());

        REQUIRE(buffer.push(2));
        REQUIRE(buffer.size() == 2);
    }

    SECTION("Pop removes elements") {
        buffer.push(1);
        buffer.push(2);

        auto item = buffer.pop();
        REQUIRE(item.has_value());
        REQUIRE(*item == 1);
        REQUIRE(buffer.size() == 1);

        item = buffer.pop();
        REQUIRE(item.has_value());
        REQUIRE(*item == 2);
        REQUIRE(buffer.size() == 0);
        REQUIRE(buffer.empty());
    }

    SECTION("Pop from empty buffer returns nullopt") {
        auto item = buffer.pop();
        REQUIRE_FALSE(item.has_value());
    }

    SECTION("Buffer becomes full") {
        for (int i = 0; i < 5; ++i) {
            REQUIRE(buffer.push(i));
        }

        REQUIRE(buffer.full());
        REQUIRE(buffer.size() == 5);
        REQUIRE_FALSE(buffer.push(100));
    }

    SECTION("Peek shows next item without removing") {
        buffer.push(42);

        auto item = buffer.peek();
        REQUIRE(item.has_value());
        REQUIRE(item->get() == 42);
        REQUIRE(buffer.size() == 1);
    }
}

TEST_CASE("CircularBuffer with complex types", "[circular_buffer]") {
    CircularBuffer<std::string, 3> buffer;

    SECTION("Push and pop strings") {
        REQUIRE(buffer.push("hello"));
        REQUIRE(buffer.push("world"));

        auto item = buffer.pop();
        REQUIRE(item.has_value());
        REQUIRE(*item == "hello");
    }

    SECTION("Move semantics") {
        std::string hello = "hello";
        REQUIRE(buffer.push(std::move(hello)));

        auto item = buffer.pop();
        REQUIRE(item.has_value());
        REQUIRE(*item == "hello");
    }
}

TEST_CASE("CircularBuffer thread safety", "[circular_buffer]") {
    CircularBuffer<int, 1000> buffer;

    SECTION("Multiple threads pushing and popping") {
        std::atomic<int> push_count{0};
        std::atomic<int> pop_count{0};

        std::vector<std::thread> threads;

        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&buffer, &push_count]() {
                for (int j = 0; j < 100; ++j) {
                    if (buffer.push(j)) {
                        push_count++;
                    }
                }
            });

            threads.emplace_back([&buffer, &pop_count]() {
                for (int j = 0; j < 100; ++j) {
                    if (buffer.pop().has_value()) {
                        pop_count++;
                    }
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        REQUIRE(buffer.size() == push_count - pop_count);
    }
}

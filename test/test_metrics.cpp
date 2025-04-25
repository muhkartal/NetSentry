#include "catch2/catch.hpp"
#include "../src/core/metrics/system_metrics.hpp"
#include <chrono>
#include <thread>

using namespace netsentry::metrics;

TEST_CASE("GaugeMetric operations", "[metrics]") {
    GaugeMetric gauge("test.gauge");

    SECTION("Initial value is zero") {
        REQUIRE(gauge.getCurrentValue() == 0.0);
    }

    SECTION("Update changes value") {
        gauge.update(42.5);
        REQUIRE(gauge.getCurrentValue() == 42.5);
    }

    SECTION("Historical values are stored") {
        gauge.update(10.0);

        auto now = std::chrono::system_clock::now();
        auto time_point = gauge.getValueAt(now);

        REQUIRE(time_point.has_value());
        REQUIRE(*time_point == 10.0);
    }

    SECTION("Non-existent time points return nearest value") {
        gauge.update(10.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        gauge.update(20.0);

        auto past = std::chrono::system_clock::now() - std::chrono::hours(1);
        auto future = std::chrono::system_clock::now() + std::chrono::hours(1);

        auto past_value = gauge.getValueAt(past);
        auto future_value = gauge.getValueAt(future);

        REQUIRE(past_value.has_value());
        REQUIRE(*past_value == 10.0);
        REQUIRE(future_value.has_value());
        REQUIRE(*future_value == 20.0);
    }
}

TEST_CASE("CounterMetric operations", "[metrics]") {
    CounterMetric counter("test.counter");

    SECTION("Initial value is zero") {
        REQUIRE(counter.getCurrentValue() == 0.0);
    }

    SECTION("Update changes value") {
        counter.update(42.5);
        REQUIRE(counter.getCurrentValue() == 42.5);
    }

    SECTION("Increment adds to value") {
        counter.update(10.0);
        counter.increment(5.0);
        REQUIRE(counter.getCurrentValue() == 15.0);
    }

    SECTION("Default increment adds 1.0") {
        counter.update(10.0);
        counter.increment();
        REQUIRE(counter.getCurrentValue() == 11.0);
    }

    SECTION("Historical values are stored") {
        counter.update(10.0);

        auto now = std::chrono::system_clock::now();
        auto time_point = counter.getValueAt(now);

        REQUIRE(time_point.has_value());
        REQUIRE(*time_point == 10.0);
    }
}

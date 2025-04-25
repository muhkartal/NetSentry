cmake_minimum_required(VERSION 3.14)
project(NetSentry VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

add_library(netsentry_core
    src/core/metrics/system_metrics.cpp
    src/core/collectors/cpu_collector.cpp
    src/core/collectors/memory_collector.cpp
    src/core/utils/logger.cpp
)

add_library(netsentry_network
    src/network/packet_capture.cpp
    src/network/packet_analyzer.cpp
)

add_library(netsentry_alert
    src/alert/alert_manager.cpp
    src/alert/alert_rule.cpp
)

add_executable(netsentry src/main.cpp)
target_link_libraries(netsentry
    PRIVATE
        netsentry_core
        netsentry_network
        netsentry_alert
        Threads::Threads
)

target_include_directories(netsentry_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(netsentry_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(netsentry_alert PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(UNIX AND NOT APPLE)
    target_link_libraries(netsentry_network PRIVATE pcap)
endif()

install(TARGETS netsentry RUNTIME DESTINATION bin)

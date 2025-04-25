#include <iostream>
#include <string>
#include <thread>
#include <csignal>
#include <atomic>
#include <memory>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <ctime>

#include "core/metrics/system_metrics.hpp"
#include "core/collectors/cpu_collector.hpp"
#include "core/collectors/memory_collector.hpp"
#include "core/utils/thread_pool.hpp"
#include "core/utils/logger.hpp"
#include "core/config/config_manager.hpp"
#include "network/packet_capture.hpp"
#include "network/packet_analyzer.hpp"
#include "alert/alert_manager.hpp"
#include "api/rest_api.hpp"
#include "web/dashboard.hpp"
#include "db/database.hpp"

using namespace netsentry;

static std::atomic<bool> keep_running{true};

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "Shutdown signal received, terminating..." << std::endl;
        keep_running = false;
    }
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help                   Show this help message" << std::endl;
    std::cout << "  --config <file>          Load configuration from file" << std::endl;
    std::cout << "  --interface <interface>  Network interface for packet capture" << std::endl;
    std::cout << "  --api-enable             Enable REST API server" << std::endl;
    std::cout << "  --api-port <port>        Set API server port (default: 8080)" << std::endl;
    std::cout << "  --web-enable             Enable web dashboard" << std::endl;
    std::cout << "  --web-port <port>        Set web dashboard port (default: 9090)" << std::endl;
    std::cout << "  --log-level <level>      Set log level (trace, debug, info, warning, error)" << std::endl;
    std::cout << "  --log-file <file>        Set log file path" << std::endl;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    auto& config = config::ConfigManager::getInstance();

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            if (!config.loadFromFile(argv[++i])) {
                std::cerr << "Failed to load configuration from " << argv[i] << std::endl;
                return 1;
            }
        } else if (strcmp(argv[i], "--interface") == 0 && i + 1 < argc) {
            config.set<std::string>("capture_interface", argv[++i]);
            config.set<bool>("enable_packet_capture", true);
        } else if (strcmp(argv[i], "--api-enable") == 0) {
            config.set<bool>("enable_api", true);
        } else if (strcmp(argv[i], "--api-port") == 0 && i + 1 < argc) {
            config.set<uint16_t>("api_port", static_cast<uint16_t>(std::stoi(argv[++i])));
        } else if (strcmp(argv[i], "--web-enable") == 0) {
            config.set<bool>("enable_web", true);
        } else if (strcmp(argv[i], "--web-port") == 0 && i + 1 < argc) {
            config.set<uint16_t>("web_port", static_cast<uint16_t>(std::stoi(argv[++i])));
        } else if (strcmp(argv[i], "--log-level") == 0 && i + 1 < argc) {
            config.set<std::string>("log_level", argv[++i]);
        } else if (strcmp(argv[i], "--log-file") == 0 && i + 1 < argc) {
            config.set<std::string>("log_file", argv[++i]);
        }
    }

    try {
        // Initialize logger
        utils::LogLevel log_level = utils::LogLevel::INFO;
        std::string log_level_str = config.getOrDefault<std::string>("log_level", "info");

        if (log_level_str == "trace") {
            log_level = utils::LogLevel::TRACE;
        } else if (log_level_str == "debug") {
            log_level = utils::LogLevel::DEBUG;
        } else if (log_level_str == "info") {
            log_level = utils::LogLevel::INFO;
        } else if (log_level_str == "warning") {
            log_level = utils::LogLevel::WARNING;
        } else if (log_level_str == "error") {
            log_level = utils::LogLevel::ERROR;
        } else if (log_level_str == "critical") {
            log_level = utils::LogLevel::CRITICAL;
        }

        std::string log_file = config.getOrDefault<std::string>("log_file", "netsentry.log");
        utils::Logger::initialize(log_file, log_level);

        LOG_INFO("NetSentry starting up...");

        // Initialize thread pool
        unsigned int thread_count = std::thread::hardware_concurrency();
        utils::ThreadPool thread_pool(thread_count);
        LOG_INFO("Thread pool initialized with %u threads", thread_count);

        // Initialize database
        std::string db_type = config.getOrDefault<std::string>("database_type", "memory");
        std::string db_path = config.getOrDefault<std::string>("database_path", "data/netsentry.db");

        db::DatabaseType database_type = db::DatabaseType::MEMORY;
        if (db_type == "sqlite") {
            database_type = db::DatabaseType::SQLITE;
        }

        auto database = db::DatabaseFactory::createDatabase(database_type, db_path);
        if (!database->initialize()) {
            LOG_ERROR("Failed to initialize database");
            return 1;
        }

        LOG_INFO("Database initialized: %s", db_type.c_str());

        // Initialize collectors
        std::vector<std::unique_ptr<collectors::CollectorBase>> collectors;
        collectors.push_back(std::make_unique<collectors::CpuCollector>(
            std::chrono::seconds(1)));
        collectors.push_back(std::make_unique<collectors::MemoryCollector>(
            std::chrono::seconds(1)));

        for (auto& collector : collectors) {
            collector->start();
        }

        LOG_INFO("System collectors started");

        // Initialize packet capture and analyzer
        std::unique_ptr<network::PacketCapture> packet_capture;
        std::unique_ptr<network::PacketAnalyzer> packet_analyzer;

        if (config.getOrDefault<bool>("enable_packet_capture", false)) {
            packet_capture = std::make_unique<network::PacketCapture>();
            packet_analyzer = std::make_unique<network::PacketAnalyzer>();

            packet_capture->registerHandler([&packet_analyzer, &thread_pool, &database](const network::PacketInfo& packet) {
                thread_pool.enqueue([&packet_analyzer, &database, packet]() {
                    packet_analyzer->processPacket(packet);

                    // Store connection data in database
                    auto conn_key = packet_analyzer->createConnectionKey(packet);
                    auto conn_stats = packet_analyzer->getConnectionStats(conn_key);

                    if (conn_stats) {
                        db::ConnectionRecord record;
                        record.source_ip = conn_key.source_ip;
                        record.source_port = conn_key.source_port;
                        record.dest_ip = conn_key.dest_ip;
                        record.dest_port = conn_key.dest_port;
                        record.protocol = conn_key.protocol;
                        record.bytes_sent = conn_stats->bytes_sent;
                        record.bytes_received = conn_stats->bytes_received;
                        record.packets_sent = conn_stats->packets_sent;
                        record.packets_received = conn_stats->packets_received;
                        record.first_seen = conn_stats->first_seen;
                        record.last_seen = conn_stats->last_seen;

                        database->updateConnection(record);
                    }
                });
            });

            auto interface = config.getOrDefault<std::string>("capture_interface", "eth0");
            auto result = packet_capture->startCapture(interface);
            if (result != network::CaptureError::NONE) {
                LOG_ERROR("Failed to start packet capture on interface: %s", interface.c_str());
                packet_capture.reset();
                packet_analyzer.reset();
            } else {
                LOG_INFO("Capturing packets on interface: %s", interface.c_str());
            }
        }

        // Initialize alert manager
        alert::AlertManager alert_manager;

        alert_manager.registerCallback([&database](const alert::Alert& alert) {
            LOG_WARNING("Alert triggered: %s", alert.getMessage().c_str());

            db::AlertRecord record;
            record.name = alert.getName();
            record.description = alert.getCondition().getDescription();
            record.severity = static_cast<int>(alert.getSeverity());
            record.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            record.acknowledged = false;

            database->insertAlert(record);
        });

        // Set up CPU usage alert
        auto cpu_metric = collectors[0]->getMetric("cpu.usage");
        if (cpu_metric) {
            uint32_t cpu_warning = config.getOrDefault<uint32_t>("cpu_threshold_warning", 75);
            uint32_t cpu_critical = config.getOrDefault<uint32_t>("cpu_threshold_critical", 90);

            alert_manager.createAlert(
                "High CPU Usage (Warning)",
                std::make_unique<alert::MetricThresholdCondition>(
                    cpu_metric, alert::Comparator::GREATER_THAN, cpu_warning),
                alert::Severity::WARNING);

            alert_manager.createAlert(
                "High CPU Usage (Critical)",
                std::make_unique<alert::MetricThresholdCondition>(
                    cpu_metric, alert::Comparator::GREATER_THAN, cpu_critical),
                alert::Severity::CRITICAL);
        }

        // Set up memory usage alert
        auto memory_metric = collectors[1]->getMetric("memory.usage_percent");
        if (memory_metric) {
            uint32_t mem_warning = config.getOrDefault<uint32_t>("memory_threshold_warning", 70);
            uint32_t mem_critical = config.getOrDefault<uint32_t>("memory_threshold_critical", 85);

            alert_manager.createAlert(
                "High Memory Usage (Warning)",
                std::make_unique<alert::MetricThresholdCondition>(
                    memory_metric, alert::Comparator::GREATER_THAN, mem_warning),
                alert::Severity::WARNING);

            alert_manager.createAlert(
                "High Memory Usage (Critical)",
                std::make_unique<alert::MetricThresholdCondition>(
                    memory_metric, alert::Comparator::GREATER_THAN, mem_critical),
                alert::Severity::CRITICAL);
        }

        // Initialize API server if enabled
        std::unique_ptr<api::RestApi> api_server;
        if (config.getOrDefault<bool>("enable_api", false)) {
            uint16_t api_port = config.getOrDefault<uint16_t>("api_port", 8080);
            api_server = std::make_unique<api::RestApi>(collectors, packet_analyzer);
            api_server->start(api_port);
            LOG_INFO("REST API server started on port %u", api_port);
        }

        // Initialize web dashboard if enabled
        std::unique_ptr<web::Dashboard> web_dashboard;
        if (config.getOrDefault<bool>("enable_web", false)) {
            uint16_t web_port = config.getOrDefault<uint16_t>("web_port", 9090);
            web_dashboard = std::make_unique<web::Dashboard>(collectors, packet_analyzer);
            web_dashboard->start(web_port);
            LOG_INFO("Web dashboard started on port %u", web_port);
        }

        LOG_INFO("NetSentry is running. Press Ctrl+C to exit.");

        // Main loop
        auto last_cleanup_time = std::chrono::steady_clock::now();
        bool enable_auto_cleanup = config.getOrDefault<bool>("enable_auto_cleanup", true);
        int64_t cleanup_interval = config.getOrDefault<uint32_t>("cleanup_interval_seconds", 3600);

        while (keep_running) {
            // Check alerts
            thread_pool.enqueue([&alert_manager]() {
                alert_manager.checkAlerts();
            });

            // Store metrics in database
            thread_pool.enqueue([&collectors, &database]() {
                std::vector<db::MetricDataPoint> points;

                int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                for (const auto& collector : collectors) {
                    for (const auto& name : collector->getMetricNames()) {
                        auto metric = collector->getMetric(name);
                        if (metric) {
                            db::MetricDataPoint point;
                            point.metric_name = name;
                            point.value = metric->getCurrentValue();
                            point.timestamp = now;
                            points.push_back(point);
                        }
                    }
                }

                database->insertMetrics(points);
            });

            // Periodic database cleanup
            if (enable_auto_cleanup) {
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cleanup_time).count() >= cleanup_interval) {
                    thread_pool.enqueue([&database, &config]() {
                        int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count();

                        int64_t metrics_max_age = config.getOrDefault<uint32_t>("metrics_max_age_days", 30) * 86400;
                        int64_t connections_max_age = config.getOrDefault<uint32_t>("connections_max_age_days", 7) * 86400;
                        int64_t alerts_max_age = config.getOrDefault<uint32_t>("alerts_max_age_days", 90) * 86400;

                        database->pruneMetricsBefore(now - metrics_max_age);
                        database->pruneConnectionsBefore(now - connections_max_age);
                        database->pruneAlertsBefore(now - alerts_max_age);

                        LOG_INFO("Database cleanup completed");
                    });

                    last_cleanup_time = now;
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Shutdown
        LOG_INFO("Shutting down...");

        if (web_dashboard) {
            web_dashboard->stop();
            LOG_INFO("Web dashboard stopped");
        }

        if (api_server) {
            api_server->stop();
            LOG_INFO("REST API server stopped");
        }

        for (auto& collector : collectors) {
            collector->stop();
        }
        LOG_INFO("System collectors stopped");

        if (packet_capture && packet_capture->isCapturing()) {
            packet_capture->stopCapture();
            LOG_INFO("Packet capture stopped");
        }

        LOG_INFO("NetSentry shutdown complete");
        return 0;

    } catch (const std::exception& ex) {
        LOG_CRITICAL("Fatal error: %s", ex.what());
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
}

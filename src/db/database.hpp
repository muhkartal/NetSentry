#pragma once

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <chrono>
#include <mutex>

namespace netsentry {
namespace db {

enum class DatabaseType {
    SQLITE,
    MEMORY
};

struct MetricDataPoint {
    std::string metric_name;
    double value;
    int64_t timestamp;
};

struct ConnectionRecord {
    std::string source_ip;
    uint16_t source_port;
    std::string dest_ip;
    uint16_t dest_port;
    uint8_t protocol;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t packets_sent;
    uint64_t packets_received;
    int64_t first_seen;
    int64_t last_seen;
};

struct AlertRecord {
    std::string name;
    std::string description;
    int severity;
    int64_t timestamp;
    bool acknowledged;
};

class Database {
public:
    virtual ~Database() = default;

    virtual bool initialize() = 0;
    virtual bool isInitialized() const = 0;

    // Metrics
    virtual bool insertMetric(const MetricDataPoint& point) = 0;
    virtual bool insertMetrics(const std::vector<MetricDataPoint>& points) = 0;
    virtual std::vector<MetricDataPoint> getMetricHistory(
        const std::string& metric_name,
        int64_t start_time,
        int64_t end_time,
        size_t max_points = 1000) = 0;
    virtual std::optional<double> getLatestMetricValue(const std::string& metric_name) = 0;
    virtual bool pruneMetricsBefore(int64_t timestamp) = 0;

    // Connections
    virtual bool insertConnection(const ConnectionRecord& record) = 0;
    virtual bool updateConnection(const ConnectionRecord& record) = 0;
    virtual std::vector<ConnectionRecord> getRecentConnections(size_t limit = 100) = 0;
    virtual std::vector<ConnectionRecord> getConnectionsByHost(
        const std::string& host,
        int64_t start_time,
        int64_t end_time,
        size_t limit = 100) = 0;
    virtual bool pruneConnectionsBefore(int64_t timestamp) = 0;

    // Alerts
    virtual bool insertAlert(const AlertRecord& alert) = 0;
    virtual bool acknowledgeAlert(int64_t alert_id) = 0;
    virtual std::vector<AlertRecord> getRecentAlerts(size_t limit = 100, bool include_acknowledged = false) = 0;
    virtual bool pruneAlertsBefore(int64_t timestamp) = 0;
};

class DatabaseFactory {
public:
    static std::unique_ptr<Database> createDatabase(
        DatabaseType type,
        const std::string& connection_string = ":memory:");
};

class SqliteDatabase : public Database {
public:
    explicit SqliteDatabase(const std::string& db_path);
    ~SqliteDatabase() override;

    bool initialize() override;
    bool isInitialized() const override;

    bool insertMetric(const MetricDataPoint& point) override;
    bool insertMetrics(const std::vector<MetricDataPoint>& points) override;
    std::vector<MetricDataPoint> getMetricHistory(
        const std::string& metric_name,
        int64_t start_time,
        int64_t end_time,
        size_t max_points = 1000) override;
    std::optional<double> getLatestMetricValue(const std::string& metric_name) override;
    bool pruneMetricsBefore(int64_t timestamp) override;

    bool insertConnection(const ConnectionRecord& record) override;
    bool updateConnection(const ConnectionRecord& record) override;
    std::vector<ConnectionRecord> getRecentConnections(size_t limit = 100) override;
    std::vector<ConnectionRecord> getConnectionsByHost(
        const std::string& host,
        int64_t start_time,
        int64_t end_time,
        size_t limit = 100) override;
    bool pruneConnectionsBefore(int64_t timestamp) override;

    bool insertAlert(const AlertRecord& alert) override;
    bool acknowledgeAlert(int64_t alert_id) override;
    std::vector<AlertRecord> getRecentAlerts(size_t limit = 100, bool include_acknowledged = false) override;
    bool pruneAlertsBefore(int64_t timestamp) override;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    std::string db_path_;
    bool initialized_;
    mutable std::mutex mutex_;
};

class InMemoryDatabase : public Database {
public:
    InMemoryDatabase();

    bool initialize() override;
    bool isInitialized() const override;

    bool insertMetric(const MetricDataPoint& point) override;
    bool insertMetrics(const std::vector<MetricDataPoint>& points) override;
    std::vector<MetricDataPoint> getMetricHistory(
        const std::string& metric_name,
        int64_t start_time,
        int64_t end_time,
        size_t max_points = 1000) override;
    std::optional<double> getLatestMetricValue(const std::string& metric_name) override;
    bool pruneMetricsBefore(int64_t timestamp) override;

    bool insertConnection(const ConnectionRecord& record) override;
    bool updateConnection(const ConnectionRecord& record) override;
    std::vector<ConnectionRecord> getRecentConnections(size_t limit = 100) override;
    std::vector<ConnectionRecord> getConnectionsByHost(
        const std::string& host,
        int64_t start_time,
        int64_t end_time,
        size_t limit = 100) override;
    bool pruneConnectionsBefore(int64_t timestamp) override;

    bool insertAlert(const AlertRecord& alert) override;
    bool acknowledgeAlert(int64_t alert_id) override;
    std::vector<AlertRecord> getRecentAlerts(size_t limit = 100, bool include_acknowledged = false) override;
    bool pruneAlertsBefore(int64_t timestamp) override;

private:
    bool initialized_;
    mutable std::mutex mutex_;

    std::vector<MetricDataPoint> metrics_;
    std::vector<ConnectionRecord> connections_;
    std::vector<AlertRecord> alerts_;
    int64_t next_alert_id_;
};

}
}

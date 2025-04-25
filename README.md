# NetSentry

<div align="center">
  <img src="docs/images/netsentry-logo.png" alt="NetSentry Logo" width="200"/>

![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/yourusername/netsentry/ci.yml?branch=main)
![Docker Image Size](https://img.shields.io/docker/image-size/yourusername/netsentry/latest)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

**Advanced Network Monitoring and Analysis System**

</div>

## Overview

NetSentry is a high-performance, extensible network monitoring system built with modern C++. It combines real-time system metrics collection, network packet analysis, and sophisticated alerting capabilities to provide comprehensive visibility into your network infrastructure.

<div align="center">
  <img src="docs/images/netsentry-dashboard.png" alt="NetSentry Dashboard" width="800"/>
</div>

## Key Features

-  **Advanced System Monitoring**

   -  Real-time CPU, memory, and disk metrics
   -  Process-level resource tracking
   -  Performance trend analysis

-  **Deep Network Analysis**

   -  Packet capture and inspection
   -  Protocol detection (HTTP, DNS, TLS)
   -  Traffic pattern recognition
   -  Connection tracking

-  **Sophisticated Alert System**

   -  Configurable thresholds and conditions
   -  Multi-channel notifications
   -  Alert correlation and grouping

-  **Powerful Visualization**

   -  Real-time dashboards
   -  Historical metrics exploration
   -  Network traffic graphs
   -  Prometheus/Grafana integration

-  **Modern API Architecture**

   -  RESTful API with comprehensive endpoints
   -  Real-time data streaming
   -  Seamless third-party integration

-  **Enterprise-Grade Deployment**
   -  Docker containerization
   -  Kubernetes orchestration
   -  High-availability configurations
   -  Horizontal scaling support

## Technical Architecture

NetSentry leverages cutting-edge C++17 features and software design principles:

-  **Core Metrics Engine**: High-performance time-series metrics collection
-  **Network Analysis Module**: Deep packet inspection and protocol analysis
-  **Custom Memory Management**: Optimized memory pools and allocators
-  **Thread-Safe Concurrency**: Lock-free algorithms and robust synchronization
-  **Pluggable Alert System**: Policy-based design for flexible alerting
-  **Data Storage Layer**: Configurable retention and storage backends
-  **RESTful API Layer**: Modern API for data access and control
-  **Web Dashboard**: Real-time visualization interface

## Getting Started

### Quick Start with Docker

The easiest way to get started with NetSentry is using Docker:

```bash
# Clone the repository
git clone https://github.com/yourusername/netsentry.git
cd netsentry

# Run the installation script
chmod +x install.sh
./install.sh
```

This will set up NetSentry with Prometheus and Grafana for a complete monitoring stack.

### Manual Installation

#### Prerequisites

-  C++17 compatible compiler (GCC 8+, Clang 7+)
-  CMake 3.14+
-  libpcap development libraries
-  Boost libraries (system, filesystem)

#### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/netsentry.git
cd netsentry

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make -j$(nproc)

# Install (optional)
sudo make install
```

### Kubernetes Deployment

For production environments, we provide Kubernetes manifests:

```bash
# Apply Kubernetes configuration
kubectl apply -f kubernetes/deployment.yaml

# Check deployment status
kubectl get pods -l app=netsentry
```

## Configuration

NetSentry uses YAML for configuration. The default configuration file is located at `configs/netsentry.yaml`:

```yaml
# Example configuration
log_level: "info"
enable_api: true
api_port: 8080
enable_web: true
web_port: 9090
enable_packet_capture: true
capture_interface: "eth0"
```

See the [Configuration Guide](docs/CONFIGURATION.md) for complete documentation.

## Usage Examples

### Monitoring System Resources

NetSentry automatically collects CPU, memory, and disk metrics out of the box:

```bash
# View real-time system metrics
curl http://localhost:8080/api/v1/metrics
```

### Analyzing Network Traffic

Enable packet capture to analyze network traffic:

```bash
# Start NetSentry with packet capture
./netsentry --interface eth0
```

### Setting Up Alerts

Create alerts for important metrics:

```bash
# Create a high CPU usage alert via the API
curl -X POST http://localhost:8080/api/v1/alerts \
  -H "Content-Type: application/json" \
  -d '{
    "name": "High CPU Usage",
    "metric": "cpu.usage",
    "comparator": "gt",
    "threshold": 90.0,
    "severity": "warning"
  }'
```

## API Documentation

NetSentry provides a comprehensive REST API for integration with other systems. Here are some key endpoints:

-  `GET /api/v1/metrics` - List all available metrics
-  `GET /api/v1/network/connections` - List active network connections
-  `GET /api/v1/network/hosts` - List top hosts by traffic
-  `GET /api/v1/alerts` - List active alerts

For complete API documentation, see [API.md](docs/API.md).

## Dashboard

NetSentry includes a web dashboard available at http://localhost:9090 when running with the default configuration.

The dashboard provides:

-  Real-time metrics visualization
-  Network connection tables
-  Protocol distribution charts
-  Alert management interface

## Development and Contributing

We welcome contributions from the community! See [CONTRIBUTING.md](docs/CONTRIBUTING.md) for guidelines.

### Development Setup

```bash
# Set up development environment
git clone https://github.com/yourusername/netsentry.git
cd netsentry

# Install dev dependencies
./scripts/install_dev_deps.sh

# Build with tests enabled
mkdir build && cd build
cmake -DBUILD_TESTS=ON ..
make -j$(nproc)

# Run tests
ctest
```

### Project Roadmap

For information about upcoming features and development plans, see our [ROADMAP.md](docs/ROADMAP.md).

## License

NetSentry is released under the MIT License. See [LICENSE](LICENSE) for details.

## Acknowledgements

-  [libpcap](https://www.tcpdump.org/) for packet capture functionality
-  [Boost C++ Libraries](https://www.boost.org/) for networking components
-  [Prometheus](https://prometheus.io/) for metrics storage
-  [Grafana](https://grafana.com/) for metrics visualization

---

<div align="center">
  <p>
    <strong>NetSentry</strong> - Advanced Network Monitoring in Modern C++
  </p>
  <p>
    <a href="https://github.com/yourusername/netsentry">GitHub</a> •
    <a href="https://hub.docker.com/r/yourusername/netsentry">Docker Hub</a> •
    <a href="docs/API.md">API Docs</a>
  </p>
</div>

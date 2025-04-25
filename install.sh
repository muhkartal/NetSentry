#!/bin/bash

set -e

BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

check_dependencies() {
    log_info "Checking dependencies..."

    # Check if Docker is installed
    if ! command -v docker &> /dev/null; then
        log_error "Docker is not installed. Please install Docker and try again."
        log_info "Visit https://docs.docker.com/get-docker/ for installation instructions."
        exit 1
    fi

    # Check if Docker Compose is installed
    if ! command -v docker-compose &> /dev/null; then
        log_error "Docker Compose is not installed. Please install Docker Compose and try again."
        log_info "Visit https://docs.docker.com/compose/install/ for installation instructions."
        exit 1
    fi

    # Check if libpcap is installed for host-based monitoring
    if ! command -v tcpdump &> /dev/null; then
        log_warning "libpcap/tcpdump not found. This may limit packet capture functionality."
        log_info "On Ubuntu/Debian, install with: sudo apt-get install -y libpcap-dev tcpdump"
        log_info "On CentOS/RHEL, install with: sudo yum install -y libpcap-devel tcpdump"
    fi

    log_success "All required dependencies are installed!"
}

create_directories() {
    log_info "Creating necessary directories..."

    mkdir -p data
    mkdir -p configs
    mkdir -p prometheus
    mkdir -p grafana/provisioning/dashboards

    log_success "Directories created successfully!"
}

configure_netsentry() {
    log_info "Configuring NetSentry..."

    # If config file doesn't exist, create a default one
    if [ ! -f "configs/netsentry.yaml" ]; then
        log_info "Creating default configuration file..."
        cat > configs/netsentry.yaml <<EOL
# NetSentry Configuration

# General settings
log_level: "info"
log_file: "/app/data/netsentry.log"
metric_retention_seconds: 86400
database_type: "sqlite"
database_path: "/app/data/netsentry.db"

# API settings
enable_api: true
api_port: 8080

# Web dashboard settings
enable_web: true
web_port: 9090

# Network capture settings
enable_packet_capture: true
capture_interface: "eth0"

# Prometheus exporter
enable_prometheus_exporter: true
prometheus_port: 9091
EOL
    else
        log_info "Configuration file already exists, skipping..."
    fi

    # Create Prometheus config
    if [ ! -f "prometheus/prometheus.yml" ]; then
        log_info "Creating Prometheus configuration..."
        cat > prometheus/prometheus.yml <<EOL
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'netsentry'
    static_configs:
      - targets: ['netsentry:9091']
    metrics_path: '/metrics'

  - job_name: 'prometheus'
    static_configs:
      - targets: ['localhost:9090']
EOL
    fi

    log_success "Configuration complete!"
}

deploy_netsentry() {
    log_info "Deploying NetSentry using Docker Compose..."

    # Build and start containers
    docker-compose up -d

    if [ $? -eq 0 ]; then
        log_success "NetSentry is now running!"
        log_info "API is available at: http://localhost:8080/api/v1"
        log_info "Web dashboard is available at: http://localhost:9090"
        log_info "Grafana is available at: http://localhost:3000 (admin/netsentry)"
    else
        log_error "Failed to start NetSentry. Check the logs with 'docker-compose logs'"
        exit 1
    fi
}

show_post_install() {
    echo ""
    echo "================= NetSentry Successfully Installed ================="
    echo ""
    echo "Services running:"
    echo "  - NetSentry API:       http://localhost:8080/api/v1"
    echo "  - NetSentry Dashboard: http://localhost:9090"
    echo "  - Grafana:             http://localhost:3000 (admin/netsentry)"
    echo "  - Prometheus:          http://localhost:9091"
    echo ""
    echo "Data is persisted in the following directories:"
    echo "  - ./data:       NetSentry database and logs"
    echo "  - ./prometheus: Prometheus time-series database"
    echo "  - ./grafana:    Grafana dashboards and settings"
    echo ""
    echo "Management commands:"
    echo "  - View logs:           docker-compose logs -f"
    echo "  - Stop services:       docker-compose stop"
    echo "  - Start services:      docker-compose start"
    echo "  - Restart services:    docker-compose restart"
    echo "  - Remove all services: docker-compose down"
    echo ""
    echo "=================================================================="
}

# Main installation process
echo "================= NetSentry Installation Script ================="
echo ""

check_dependencies
create_directories
configure_netsentry
deploy_netsentry
show_post_install

exit 0

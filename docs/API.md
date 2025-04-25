# NetSentry REST API Documentation

This document provides detailed information about the NetSentry REST API endpoints, request/response formats, and authentication mechanisms.

## API Base URL

The base URL for all API endpoints is:

```
http://<host>:8080/api/v1
```

## Authentication

Currently, the API doesn't require authentication, but this may change in future versions with support for API keys or JWT tokens.

## Error Handling

All API errors return an appropriate HTTP status code along with a JSON response body that includes an error message. Example:

```json
{
   "error": "Metric not found"
}
```

Common HTTP status codes:

-  `200 OK`: Request successful
-  `400 Bad Request`: Invalid request parameters
-  `404 Not Found`: Resource not found
-  `500 Internal Server Error`: Server-side error

## API Endpoints

### System Information

#### Get System Info

```
GET /api/v1/system/info
```

Returns basic information about the system being monitored.

**Example Response:**

```json
{
   "hostname": "server01",
   "platform": "Linux",
   "num_cpus": 4,
   "uptime": 86400
}
```

### Metrics

#### Get All Metrics

```
GET /api/v1/metrics
```

Returns a list of all available metrics and their current values.

**Example Response:**

```json
{
   "metrics": [
      {
         "name": "cpu.usage",
         "value": 45.2
      },
      {
         "name": "memory.usage_percent",
         "value": 67.8
      }
   ]
}
```

#### Get Specific Metric

```
GET /api/v1/metrics/{name}
```

Returns information about a specific metric.

**Parameters:**

-  `name`: The name of the metric (e.g., `cpu.usage`)

**Example Response:**

```json
{
   "name": "cpu.usage",
   "value": 45.2
}
```

#### Get Metric History

```
GET /api/v1/metrics/{name}/history
```

Returns historical values for a specific metric.

**Parameters:**

-  `name`: The name of the metric
-  `start` (optional): Start timestamp (Unix time)
-  `end` (optional): End timestamp (Unix time)
-  `limit` (optional): Maximum number of data points to return (default: 100)

**Example Response:**

```json
{
   "name": "cpu.usage",
   "datapoints": [
      {
         "timestamp": 1619712000,
         "value": 32.5
      },
      {
         "timestamp": 1619712060,
         "value": 45.2
      }
   ]
}
```

### Network Monitoring

#### Get Network Statistics

```
GET /api/v1/network/stats
```

Returns general network statistics.

**Example Response:**

```json
{
   "status": "Active",
   "interface": "eth0",
   "bytes_received": 1024000,
   "bytes_sent": 512000,
   "packets_received": 8192,
   "packets_sent": 4096,
   "connections": 42
}
```

#### Get Active Connections

```
GET /api/v1/network/connections
```

Returns a list of active network connections.

**Parameters:**

-  `limit` (optional): Maximum number of connections to return (default: 10)
-  `sort` (optional): Sort field, one of: `bytes`, `packets` (default: `bytes`)
-  `order` (optional): Sort order, one of: `asc`, `desc` (default: `desc`)

**Example Response:**

```json
{
   "connections": [
      {
         "source": "192.168.1.100:45678",
         "destination": "93.184.216.34:443",
         "protocol": 6,
         "bytes_sent": 4096,
         "bytes_received": 102400,
         "packets_sent": 32,
         "packets_received": 128
      }
   ]
}
```

#### Get Top Hosts

```
GET /api/v1/network/hosts
```

Returns a list of hosts with the most traffic.

**Parameters:**

-  `limit` (optional): Maximum number of hosts to return (default: 10)

**Example Response:**

```json
{
   "hosts": [
      {
         "ip": "93.184.216.34",
         "bytes": 106496
      },
      {
         "ip": "172.217.22.14",
         "bytes": 82944
      }
   ]
}
```

### Alert Management

#### Get Recent Alerts

```
GET /api/v1/alerts
```

Returns a list of recent alerts.

**Parameters:**

-  `limit` (optional): Maximum number of alerts to return (default: 10)
-  `include_acknowledged` (optional): Include acknowledged alerts (default: false)

**Example Response:**

```json
{
   "alerts": [
      {
         "id": 1,
         "name": "High CPU Usage",
         "description": "CPU usage > 90%",
         "severity": 2,
         "timestamp": 1619712000,
         "acknowledged": false
      }
   ]
}
```

#### Acknowledge Alert

```
POST /api/v1/alerts/{id}/acknowledge
```

Acknowledges a specific alert.

**Parameters:**

-  `id`: The ID of the alert to acknowledge

**Example Response:**

```json
{
   "success": true,
   "message": "Alert acknowledged"
}
```

## Streaming Endpoints

The NetSentry API also provides Server-Sent Events (SSE) endpoints for real-time monitoring.

### Stream Metrics

```
GET /api/v1/stream/metrics
```

Provides a real-time stream of metric updates.

**Parameters:**

-  `metrics` (optional): Comma-separated list of metrics to include (default: all)

**Example Response Stream:**

```
event: metric
data: {"name":"cpu.usage","value":45.2,"timestamp":1619712000}

event: metric
data: {"name":"memory.usage_percent","value":67.8,"timestamp":1619712000}
```

### Stream Alerts

```
GET /api/v1/stream/alerts
```

Provides a real-time stream of alerts.

**Example Response Stream:**

```
event: alert
data: {"id":1,"name":"High CPU Usage","description":"CPU usage > 90%","severity":2,"timestamp":1619712000}
```

## Using the API with cURL

### Get System Info

```bash
curl -X GET http://localhost:8080/api/v1/system/info
```

### Get All Metrics

```bash
curl -X GET http://localhost:8080/api/v1/metrics
```

### Get Network Connections

```bash
curl -X GET "http://localhost:8080/api/v1/network/connections?limit=5"
```

## Using the API with JavaScript

```javascript
// Get all metrics
fetch("http://localhost:8080/api/v1/metrics")
   .then((response) => response.json())
   .then((data) => console.log(data))
   .catch((error) => console.error("Error:", error));

// Stream metrics
const eventSource = new EventSource("http://localhost:8080/api/v1/stream/metrics");
eventSource.addEventListener("metric", (event) => {
   const metricData = JSON.parse(event.data);
   console.log(metricData);
});
```

## Rate Limiting

The API currently implements rate limiting of 60 requests per minute per IP address. If you exceed this limit, you'll receive a `429 Too Many Requests` response.

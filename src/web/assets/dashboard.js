// NetSentry Dashboard JavaScript

document.addEventListener("DOMContentLoaded", function () {
   // Initialize data refresh
   refreshData();
   setInterval(refreshData, 2000);

   // Initialize charts
   initCharts();

   // Initialize navigation
   initNavigation();
});

// Global variables for charts
let cpuChart;
let memoryChart;
let networkChart;

function initCharts() {
   const cpuCtx = document.getElementById("cpuChart").getContext("2d");
   cpuChart = new Chart(cpuCtx, {
      type: "line",
      data: {
         labels: [],
         datasets: [
            {
               label: "CPU Usage %",
               data: [],
               backgroundColor: "rgba(54, 162, 235, 0.2)",
               borderColor: "rgba(54, 162, 235, 1)",
               borderWidth: 2,
               pointRadius: 0,
               lineTension: 0.3,
            },
         ],
      },
      options: {
         responsive: true,
         maintainAspectRatio: false,
         scales: {
            y: {
               beginAtZero: true,
               max: 100,
               title: {
                  display: true,
                  text: "CPU Usage %",
               },
            },
            x: {
               title: {
                  display: true,
                  text: "Time",
               },
            },
         },
         animation: {
            duration: 0,
         },
      },
   });

   const memoryCtx = document.getElementById("memoryChart").getContext("2d");
   memoryChart = new Chart(memoryCtx, {
      type: "line",
      data: {
         labels: [],
         datasets: [
            {
               label: "Memory Usage %",
               data: [],
               backgroundColor: "rgba(255, 99, 132, 0.2)",
               borderColor: "rgba(255, 99, 132, 1)",
               borderWidth: 2,
               pointRadius: 0,
               lineTension: 0.3,
            },
         ],
      },
      options: {
         responsive: true,
         maintainAspectRatio: false,
         scales: {
            y: {
               beginAtZero: true,
               max: 100,
               title: {
                  display: true,
                  text: "Memory Usage %",
               },
            },
            x: {
               title: {
                  display: true,
                  text: "Time",
               },
            },
         },
         animation: {
            duration: 0,
         },
      },
   });

   const networkCtx = document.getElementById("networkChart").getContext("2d");
   networkChart = new Chart(networkCtx, {
      type: "line",
      data: {
         labels: [],
         datasets: [
            {
               label: "Bytes In",
               data: [],
               backgroundColor: "rgba(75, 192, 192, 0.2)",
               borderColor: "rgba(75, 192, 192, 1)",
               borderWidth: 2,
               pointRadius: 0,
               lineTension: 0.3,
            },
            {
               label: "Bytes Out",
               data: [],
               backgroundColor: "rgba(153, 102, 255, 0.2)",
               borderColor: "rgba(153, 102, 255, 1)",
               borderWidth: 2,
               pointRadius: 0,
               lineTension: 0.3,
            },
         ],
      },
      options: {
         responsive: true,
         maintainAspectRatio: false,
         scales: {
            y: {
               beginAtZero: true,
               title: {
                  display: true,
                  text: "Bytes",
               },
            },
            x: {
               title: {
                  display: true,
                  text: "Time",
               },
            },
         },
         animation: {
            duration: 0,
         },
      },
   });
}

function refreshData() {
   fetchMetrics();
   fetchNetworkConnections();
   fetchTopHosts();
   fetchSystemInfo();
}

function fetchMetrics() {
   fetch("/api/metrics")
      .then((response) => response.json())
      .then((data) => {
         updateMetricsDisplay(data);
         updateMetricsCharts(data);
      })
      .catch((error) => console.error("Error fetching metrics:", error));
}

function fetchNetworkConnections() {
   fetch("/api/network/connections?limit=10")
      .then((response) => response.json())
      .then((data) => {
         updateConnectionsTable(data);
      })
      .catch((error) => console.error("Error fetching connections:", error));
}

function fetchTopHosts() {
   fetch("/api/network/hosts?limit=10")
      .then((response) => response.json())
      .then((data) => {
         updateHostsTable(data);
      })
      .catch((error) => console.error("Error fetching hosts:", error));
}

function fetchSystemInfo() {
   fetch("/api/system/info")
      .then((response) => response.json())
      .then((data) => {
         updateSystemInfo(data);
      })
      .catch((error) => console.error("Error fetching system info:", error));
}

function updateMetricsDisplay(data) {
   // Update metric values in the overview section
   for (const metric of data.metrics) {
      const element = document.getElementById(`metric-${metric.name.replace(/\./g, "-")}`);
      if (element) {
         element.textContent = metric.value.toFixed(2);
      }
   }
}

function updateMetricsCharts(data) {
   const timestamp = new Date().toLocaleTimeString();

   // Find CPU and memory metrics
   let cpuUsage = 0;
   let memoryUsage = 0;
   let bytesIn = 0;
   let bytesOut = 0;

   for (const metric of data.metrics) {
      if (metric.name === "cpu.usage") {
         cpuUsage = metric.value;
      } else if (metric.name === "memory.usage_percent") {
         memoryUsage = metric.value;
      } else if (metric.name === "network.bytes_in_per_sec") {
         bytesIn = metric.value;
      } else if (metric.name === "network.bytes_out_per_sec") {
         bytesOut = metric.value;
      }
   }

   // Update CPU chart
   if (cpuChart.data.labels.length > 30) {
      cpuChart.data.labels.shift();
      cpuChart.data.datasets[0].data.shift();
   }
   cpuChart.data.labels.push(timestamp);
   cpuChart.data.datasets[0].data.push(cpuUsage);
   cpuChart.update();

   // Update memory chart
   if (memoryChart.data.labels.length > 30) {
      memoryChart.data.labels.shift();
      memoryChart.data.datasets[0].data.shift();
   }
   memoryChart.data.labels.push(timestamp);
   memoryChart.data.datasets[0].data.push(memoryUsage);
   memoryChart.update();

   // Update network chart
   if (networkChart.data.labels.length > 30) {
      networkChart.data.labels.shift();
      networkChart.data.datasets[0].data.shift();
      networkChart.data.datasets[1].data.shift();
   }
   networkChart.data.labels.push(timestamp);
   networkChart.data.datasets[0].data.push(bytesIn);
   networkChart.data.datasets[1].data.push(bytesOut);
   networkChart.update();
}

function updateConnectionsTable(data) {
   const tableBody = document.getElementById("connections-table-body");
   if (!tableBody) return;

   tableBody.innerHTML = "";

   for (const conn of data.connections) {
      const row = document.createElement("tr");

      const sourceCell = document.createElement("td");
      sourceCell.textContent = conn.source;
      row.appendChild(sourceCell);

      const destCell = document.createElement("td");
      destCell.textContent = conn.destination;
      row.appendChild(destCell);

      const protocolCell = document.createElement("td");
      protocolCell.textContent = getProtocolName(conn.protocol);
      row.appendChild(protocolCell);

      const bytesCell = document.createElement("td");
      bytesCell.textContent = formatBytes(conn.bytes_sent + conn.bytes_received);
      row.appendChild(bytesCell);

      const packetsCell = document.createElement("td");
      packetsCell.textContent = conn.packets_sent + conn.packets_received;
      row.appendChild(packetsCell);

      tableBody.appendChild(row);
   }
}

function updateHostsTable(data) {
   const tableBody = document.getElementById("hosts-table-body");
   if (!tableBody) return;

   tableBody.innerHTML = "";

   for (const host of data.hosts) {
      const row = document.createElement("tr");

      const ipCell = document.createElement("td");
      ipCell.textContent = host.ip;
      row.appendChild(ipCell);

      const bytesCell = document.createElement("td");
      bytesCell.textContent = formatBytes(host.bytes);
      row.appendChild(bytesCell);

      tableBody.appendChild(row);
   }
}

function updateSystemInfo(data) {
   const hostnameElement = document.getElementById("system-hostname");
   if (hostnameElement) {
      hostnameElement.textContent = data.hostname;
   }

   const platformElement = document.getElementById("system-platform");
   if (platformElement) {
      platformElement.textContent = data.platform;
   }

   const cpusElement = document.getElementById("system-cpus");
   if (cpusElement) {
      cpusElement.textContent = data.num_cpus;
   }

   const uptimeElement = document.getElementById("system-uptime");
   if (uptimeElement) {
      uptimeElement.textContent = formatUptime(data.uptime);
   }
}

function initNavigation() {
   const tabs = document.querySelectorAll(".tab");
   const tabContents = document.querySelectorAll(".tab-content");

   tabs.forEach((tab) => {
      tab.addEventListener("click", function () {
         const target = this.getAttribute("data-target");

         // Remove active class from all tabs and hide all tab contents
         tabs.forEach((t) => t.classList.remove("active"));
         tabContents.forEach((content) => (content.style.display = "none"));

         // Add active class to clicked tab and show its content
         this.classList.add("active");
         document.getElementById(target).style.display = "block";
      });
   });

   // Activate first tab by default
   if (tabs.length > 0) {
      tabs[0].click();
   }
}

// Helper functions
function getProtocolName(protocol) {
   switch (protocol) {
      case 1:
         return "ICMP";
      case 6:
         return "TCP";
      case 17:
         return "UDP";
      default:
         return protocol.toString();
   }
}

function formatBytes(bytes) {
   if (bytes === 0) return "0 B";

   const sizes = ["B", "KB", "MB", "GB", "TB"];
   const i = Math.floor(Math.log(bytes) / Math.log(1024));

   return (bytes / Math.pow(1024, i)).toFixed(2) + " " + sizes[i];
}

function formatUptime(seconds) {
   const days = Math.floor(seconds / 86400);
   seconds %= 86400;

   const hours = Math.floor(seconds / 3600);
   seconds %= 3600;

   const minutes = Math.floor(seconds / 60);
   seconds %= 60;

   let result = "";
   if (days > 0) result += days + "d ";
   if (hours > 0 || days > 0) result += hours + "h ";
   if (minutes > 0 || hours > 0 || days > 0) result += minutes + "m ";
   result += Math.floor(seconds) + "s";

   return result;
}

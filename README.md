# 🐳 Dockerized C System Monitor

A lightweight Linux system monitoring tool built using **C and Docker**. It displays system information, CPU and memory usage, and container resource limits using Linux system interfaces and **cgroups v2**.

The project demonstrates Linux systems programming, Docker containerization, resource management, and automated testing with GitHub Actions.

## 🚀 Features

- **System Information:** Displays hostname, process IDs, user permissions, operating system, and kernel version.
- **CPU Monitoring:** Shows available CPUs, CPU quotas, and utilization.
- **Memory Monitoring:** Reports host-visible memory, container memory usage, limits, and utilization percentages.
- **Real-Time Monitoring:** Continuously updates system statistics at a configurable interval.
- **JSON Output:** Supports structured output for scripts and other applications.
- **Container Security:** Uses non-root execution, multi-stage builds, and optional runtime security restrictions.
- **Automated Testing:** Includes Python tests, Docker integration tests, and GitHub Actions CI.

## 🛠️ Technologies Used

| Technology | Purpose |
|---|---|
| C | System monitoring application |
| Linux | System interfaces and process information |
| Docker | Containerization and resource restrictions |
| GCC | Compiling the C program |
| cgroups v2 | Monitoring container CPU and memory limits |
| Python unittest | Automated testing |
| GitHub Actions | Continuous integration |

## 📁 Project Structure

```text
Dockerized-C-System-Monitor/
├── .github/
│   └── workflows/
│       └── ci.yml
├── tests/
│   ├── test_monitor.py
│   └── test_docker.sh
├── .gitignore
├── Dockerfile
├── Makefile
├── monitor.c
└── README.md
```

## ⚙️ Installation and Setup

### Prerequisites

- Docker Desktop or Docker Engine
- Linux, Windows with Docker Desktop, or WSL2
- GCC, Make, and Python 3 (for local compilation and testing)

### 1. Clone the Repository

```bash
git clone https://github.com/LavenGit/Dockerized-C-System-Monitor.git
cd Dockerized-C-System-Monitor
```

### 2. Build the Docker Image

```bash
docker build -t c-system-monitor .
```

### 3. Run the System Monitor

```bash
docker run --rm c-system-monitor
```

This displays system information and available container resource statistics.

## 💻 Usage

### Run with Resource Limits

```bash
docker run --rm --memory=128m --cpus=0.5 c-system-monitor
```

Restricts the container to 128 MiB of memory and a CPU quota equivalent to half a core.

### Real-Time Monitoring

```bash
docker run --rm --memory=128m --cpus=0.5 c-system-monitor --watch 2
```

Updates system statistics every 2 seconds. Press `Ctrl+C` to stop.

### JSON Output

```bash
docker run --rm --memory=128m --cpus=0.5 c-system-monitor --json
```

Produces structured monitoring information in JSON format.

### Run with Additional Security Restrictions

```bash
docker run --rm \
  --memory=128m \
  --cpus=0.5 \
  --pids-limit=64 \
  --network=none \
  --read-only \
  --cap-drop=ALL \
  c-system-monitor
```

These options restrict container resources, disable networking, prevent filesystem writes, and remove Linux capabilities.

*Note: The multiline command above uses Linux/WSL shell syntax. For Windows PowerShell, use a single-line command.*

## 🧪 Testing

### Compile Locally (Linux/WSL)

```bash
make
```

### Run Python Tests

```bash
python3 -m unittest discover -s tests -p "test_*.py" -v
```

The tests check program output, JSON formatting, argument validation, and continuous monitoring behavior.

### Run Docker Integration Tests

First, build the Docker image:

```bash
docker build -t c-system-monitor .
```

Then run:

```bash
sh tests/test_docker.sh
```

These tests check container memory limits, CPU quotas, and non-root execution.

## 🔄 Continuous Integration

The project uses **GitHub Actions** to automatically:

1. Compile the C program.
2. Run automated Python tests.
3. Build the Docker image.
4. Run Docker integration tests.

The workflow is defined in `.github/workflows/ci.yml` and runs when changes are pushed or pull requests are opened.

## 📚 What I Learned

Through this project, I gained hands-on experience with:

- Linux system interfaces and resource monitoring.
- How Docker containers use Linux cgroups to manage CPU and memory.
- Multi-stage Docker builds and running containers with restricted privileges.
- Writing automated tests and setting up a GitHub Actions CI workflow.

## 👨‍💻 Author

Developed as a personal project to explore Linux systems programming, Docker, and container resource management.

# Flight Computer Firmware - Docker Development Environment

This document describes the Docker-based development and testing environment for the Flight Computer Firmware project, derived from the GitHub Actions workflows.

## Overview

The project uses Docker Compose to create isolated, reproducible environments for:
- **Testing**: Running unit tests with coverage reporting
- **Building**: Cross-compiling for ARM Cortex-M7 targets
- **Development**: Combined environment for interactive development

### Alpine Linux Benefits

All Docker images are based on Alpine Linux, providing:
- **Minimal Size**: ~5MB base image vs ~70MB Ubuntu
- **Fast Builds**: Reduced download and build times
- **Security**: Smaller attack surface with fewer packages
- **musl libc**: Modern, lightweight C library
- **Package Manager**: APK package manager for efficient installs

## Quick Start

### 1. Initialize the Environment
```bash
# Set up everything (build containers, init submodules)
make setup

# Or step by step:
make init-submodules
make compose-build
```

### 2. Run Tests
```bash
# Run all tests (APPA and Canard)
make test-all

# Run specific test suites
make test-appa
make test-canard

# Run a single test
make test-single-appa TEST=apogee_detect
make test-single-canard TEST=launch_detect
```

### 3. Build Firmware
```bash
# Build all targets (stability check)
make build-all

# Build specific targets
make build-single TARGET=canard
make build-single TARGET=appa-debug
make build-single TARGET=appa-release
```

## Available Make Targets

### Setup and Management
- `make setup` - Complete project setup (containers + submodules)
- `make compose-build` - Build Docker services
- `make compose-up` - Start services in background
- `make compose-down` - Stop services
- `make status` - Show project and Docker status

### Testing
- `make test-all` - Run all unit tests
- `make test-appa` - Run APPA tests: apogee_detect, launch_detect, flight, fin_calib, flash_appa, prelaunch, sensor_calibrate
- `make test-canard` - Run Canard tests: launch_detect
- `make test-single-appa TEST=<name>` - Run single APPA test
- `make test-single-canard TEST=<name>` - Run single Canard test

### Building
- `make build-all` - Build stability check (all targets)
- `make build-single TARGET=<target>` - Build specific target
  - Available targets: `canard`, `terminal`, `data-logger`, `appa-debug`, `appa-release`

### Interactive Development
- `make shell-test` - Interactive shell in test environment
- `make shell-build` - Interactive shell in build environment  
- `make shell-dev` - Interactive shell in combined dev environment

### Cleanup
- `make clean-builds` - Remove build artifacts
- `make clean-tests` - Remove test artifacts
- `make clean-docker` - Remove Docker services and images
- `make clean-all` - Clean everything

### CI Simulation
- `make ci-test` - Simulate CI test pipeline
- `make ci-build` - Simulate CI build pipeline
- `make ci-full` - Full CI simulation (tests + builds)

## Docker Services

All services are based on **Alpine Linux** for minimal size and faster builds.

### fcf-test
Test runner environment with:
- Alpine Linux base (minimal footprint)
- Python 3 with pip
- gcovr for coverage reporting
- GCC toolchain for native compilation (musl-based)
- Make, Git, and Bash

### fcf-build  
ARM cross-compilation environment with:
- Alpine Linux base (minimal footprint)
- ARM GCC toolchain (version 9-2019-q4)
- Make, Git, Bash, and archive tools (tar, bzip2)
- Configured PATH for arm-none-eabi-gcc

### fcf-dev
Combined development environment with both test and build tools:
- All tools from fcf-test and fcf-build
- Additional utilities: curl, vim
- Suitable for interactive development

## Project Structure

Based on the GitHub workflows, the project expects:

```
├── test/
│   ├── app/
│   │   ├── appa/
│   │   │   ├── apogee_detect/    # Contains Makefile with 'test' target
│   │   │   ├── launch_detect/
│   │   │   ├── flight/
│   │   │   ├── fin_calib/
│   │   │   ├── flash_appa/
│   │   │   ├── prelaunch/
│   │   │   └── sensor_calibrate/
│   │   └── canard/
│   │       └── launch_detect/    # Contains Makefile with 'test' target
├── app/
│   ├── canard/rev2/              # Contains Makefile
│   ├── terminal/rev2/            # Contains Makefile  
│   ├── flight/
│   │   ├── data-logger/rev2/     # Contains Makefile
│   │   └── appa/rev2/            # Contains Makefile with DEBUG flag support
├── lib/                          # Git submodule
├── mod/                          # Git submodule
└── init/                         # Initialization code
```

## Test Workflow

Each test directory should contain:
- `Makefile` with a `test` target
- Test produces `results.txt` file
- Success indicated by "Result: PASS" in last 3 lines of results.txt
- Coverage artifacts placed in `coverage/` subdirectory

## Build Workflow

Build targets:
- **Canard**: `app/canard/rev2/`
- **Terminal**: `app/terminal/rev2/`  
- **Data Logger**: `app/flight/data-logger/rev2/`
- **APPA Debug**: `app/flight/appa/rev2/` with `DEBUG=1` (default)
- **APPA Release**: `app/flight/appa/rev2/` with `DEBUG=0`

All Makefiles support:
- `make` - Build target
- `make clean` - Clean build artifacts

## Example Workflows

### Running a specific test
```bash
# Start the environment
make setup

# Run a specific APPA test
make test-single-appa TEST=apogee_detect

# Check results
docker exec fcf-test-runner cat /workspace/test/app/appa/apogee_detect/coverage/results.txt
```

### Interactive development
```bash
# Open development shell
make shell-dev

# Inside container:
cd /workspace/app/flight/appa/rev2
make clean && make
arm-none-eabi-size build/appa.elf
```

### CI Pipeline Simulation
```bash
# Full CI simulation
make ci-full

# This runs:
# 1. All APPA tests
# 2. All Canard tests  
# 3. All build targets
# 4. Artifact collection
```

## Troubleshooting

### Submodules not initialized
```bash
make init-submodules
```

### Container build issues
```bash
make clean-docker
make compose-build
```

### Permission issues
```bash
# Ensure Docker daemon is running and user has access
sudo usermod -aG docker $USER
# Log out and back in
```

### View container logs
```bash
make logs-test
make logs-build
```

### Alpine-specific considerations
```bash
# If you need additional packages in containers:
docker exec fcf-dev-env apk add --no-cache <package-name>

# Alpine uses musl libc instead of glibc
# Most ARM cross-compilation tools are compatible
# Python packages with C extensions usually work fine

# For debugging, access package info:
docker exec fcf-dev-env apk info
```

### Performance Benefits
Alpine containers typically:
- Start 2-3x faster than Ubuntu equivalents
- Use 50-70% less disk space
- Have faster package installation
- Reduced network transfer for image pulls

This Docker-based approach provides the same testing and building capabilities as the GitHub Actions workflows but can be run locally for development and debugging, with improved performance thanks to Alpine Linux.
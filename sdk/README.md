# ProHand & ProGlove SDK

Public SDK for ProHand robotic hand and ProGlove control systems.

## Overview

The SDK provides a clean, FFI-based API for controlling ProHand robotic hands and ProGlove devices through a local IPC (Inter-Process Communication) interface. This SDK is designed for external developers with support for multiple programming languages.

## Package Contents

```
sdk/
├── README.md                # This file
├── prohand_sdk/            # ProHand SDK
│   ├── lib/                # Pre-built native libraries
│   ├── cpp/                # C++ headers and bindings
│   ├── python/             # Python bindings
│   └── README.md
├── proglove_sdk/           # ProGlove SDK
│   ├── lib/                # Pre-built native libraries
│   ├── cpp/                # C++ headers and bindings
│   ├── python/             # Python bindings
│   └── README.md
├── demo/                   # Example applications
│   ├── python/             # Python demo scripts
│   └── cpp/                # C++ demo applications
├── bin/                    # IPC host binaries
│   ├── prohand-headless-ipc-host   # ProHand IPC host
│   ├── proglove-headless-ipc-host  # ProGlove IPC host
│   └── udcap-ctrl          # Dual-arm UDP control
└── docs/                   # Documentation
    ├── API.md              # API reference
    └── EXAMPLES.md         # Usage examples
```

## Quick Start

### Prerequisites

1. A ProHand or ProGlove device connected via USB
2. The appropriate IPC host binary running:
   - For ProHand: `bin/prohand-headless-ipc-host`
   - For ProGlove: `bin/proglove-headless-ipc-host`

### Running the IPC Host

```bash
# For ProHand
./bin/prohand-headless-ipc-host

# For ProGlove
./bin/proglove-headless-ipc-host
```

The IPC host will:
- Auto-detect the connected device
- Create ZeroMQ endpoints for communication
- Stream device status and accept commands

### Optional: Dual-Arm UDP Control

The SDK includes the `udcap-ctrl` binary for real-time dual-arm control using glove data over UDP.

```bash
# Control both hands simultaneously
./bin/udcap-ctrl --hand left --udp-port 5555 --publish-rate 60
```

See the binary's `--help` output for full configuration options.

## Installation

### Python

#### ProHand SDK

```bash
cd prohand_sdk/python
pip install -e .
```

Example usage:

```python
from prohand_sdk import ProHandClient

# Connect to the IPC host
client = ProHandClient()
client.connect()

# Send a ping
client.ping()

# Enable streaming mode
client.set_streaming_mode(True)

# Send hand command (joint angles in radians)
hand_pose = {
    "thumb": [0.0, 0.5, 0.5, 0.5],
    "index": [0.0, 1.0, 1.0, 1.0],
    "middle": [0.0, 1.0, 1.0, 1.0],
    "ring": [0.0, 0.8, 0.8, 0.8],
    "pinky": [0.0, 0.8, 0.8, 0.8],
}
client.send_hand_command(hand_pose)

# Receive status
status = client.receive_status()
print(f"Hand status: {status}")
```

#### ProGlove SDK

```bash
cd proglove_sdk/python
pip install -e .
```

Example usage:

```python
from proglove_sdk import ProGloveClient

client = ProGloveClient()
client.connect()

# Read glove sensor data
sensor_data = client.read_sensors()
print(f"Joint angles: {sensor_data['joint_angles']}")
print(f"Taxel pressures: {sensor_data['taxels']}")
```

### C++

#### ProHand SDK

```bash
cd prohand_sdk/cpp
mkdir build && cd build
cmake ..
make
```

Example usage:

```cpp
#include <prohand_sdk/ProHandClient.hpp>

int main() {
    ProHandClient client;
    client.connect();
    
    // Send ping
    client.ping();
    
    // Enable streaming
    client.setStreamingMode(true);
    
    // Send hand command
    HandPose pose;
    pose.thumb = {0.0, 0.5, 0.5, 0.5};
    pose.index = {0.0, 1.0, 1.0, 1.0};
    client.sendHandCommand(pose);
    
    // Receive status
    auto status = client.receiveStatus();
    
    return 0;
}
```

#### ProGlove SDK

```bash
cd proglove_sdk/cpp
mkdir build && cd build
cmake ..
make
```

## Demo Applications

The SDK includes demo applications in both Python and C++:

### Python Demos

```bash
cd demo/python

# ProHand demos
python -m prohand_demo.connect      # Test connection
python -m prohand_demo.cyclic_motion  # Cyclic joint motion
python -m prohand_demo.test_hand    # Comprehensive test

# ProGlove demos
python -m proglove_demo.connect     # Test connection
python -m proglove_demo.test_glove  # Read sensor data
```

### C++ Demos

```bash
cd demo/cpp
mkdir build && cd build
cmake ..
make

# ProHand demos
./connect         # Test connection
./cyclic_motion   # Cyclic joint motion
./kapandji        # Kapandji opposition test
./test_hand       # Comprehensive test

# ProGlove demos
./connect_glove   # Test connection
./test_glove      # Read sensor data
```

## API Overview

### ProHand API

#### Available Commands

- **Ping**: Test connectivity
- **TimeSync**: Synchronize timestamps
- **HandService**: Control hand service modes
  - Available: Check if service is available
  - StreamingMode: Enable/disable streaming mode
  - AutoCalibration: Enable/disable auto-calibration
  - ZeroCalib: Zero calibration for specific servos
  - ServiceMode: Enable/disable service mode
- **HandStateCommand**: High-level finger joint commands
- **WristStateCommand**: Wrist joint commands
- **RotaryGrpCommand**: Direct servo position/torque commands (16 servos)
- **LinearGrpCommand**: Linear actuator commands (2 actuators)

#### Status Messages

- **Pong**: Response to ping
- **HandService**: Service mode responses
- **RotaryState/LinearState**: Individual servo/actuator state
- **HandState**: Complete hand state
- **HandAlert/RotaryAlert/LinearAlert**: Error alerts
- **RotaryGrpStatus/LinearGrpStatus**: Bulk servo/actuator status
- **RotaryGrpTarget/LinearGrpTarget**: Echo of commanded targets
- **Handedness**: Hand chirality (left/right)

### ProGlove API

#### Available Commands

- **Ping**: Test connectivity
- **TimeSync**: Synchronize timestamps
- **StartStreaming**: Start sensor data streaming
- **StopStreaming**: Stop sensor data streaming
- **SetSamplingRate**: Configure sampling rate

#### Status Messages

- **Pong**: Response to ping
- **SensorData**: Joint angles and taxel pressures
- **DeviceStatus**: Battery, temperature, connection status
- **Alert**: Device alerts and warnings

## Communication Architecture

```
┌─────────────────┐         ┌──────────────────┐         ┌─────────────┐
│   SDK Client    │◄────────┤  IPC Host        │◄────────┤   Device    │
│   (Your App)    │  ZMQ    │  (Prebuilt)      │   USB   │  (Hardware) │
└─────────────────┘         └──────────────────┘         └─────────────┘

ZMQ Endpoints:
- Command (REQ/REP): tcp://localhost:5555 - Send commands, receive acks
- Streaming (PUB/SUB): tcp://localhost:5556 - Stream commands at high rate
- Status (PUB/SUB): tcp://localhost:5557 - Receive device status
```

## System Requirements

- **Operating System**: macOS 10.13+ (ARM64), Linux (x86_64, ARM64)
- **Dependencies**:
  - ZeroMQ library (installed automatically with Python package)
  - C++17 compiler (for C++ development)
  - Python 3.8+ (for Python development)

## Troubleshooting

### Device Not Found

1. Check USB connection
2. Verify device appears in system (macOS: `system_profiler SPUSBDataType`)
3. On Linux, check udev rules for device permissions

### Connection Failed

1. Ensure IPC host is running
2. Check firewall settings (ZeroMQ uses TCP)
3. Verify correct endpoint addresses

### Performance Issues

1. Use streaming mode for high-frequency commands (>10 Hz)
2. Consider using the `udcap-ctrl` binary for low-latency control
3. Check system CPU usage and USB bandwidth

## Advanced Features

### Dual-Arm Control

For bimanual manipulation, use the `udcap-ctrl` utility to control both hands:

```bash
# Terminal 1: Left hand
./bin/udcap-ctrl --hand left --udp-port 5555

# Terminal 2: Right hand
./bin/udcap-ctrl --hand right --udp-port 5556
```

Send glove data via UDP to drive both hands in real-time.

### Custom IPC Endpoints

You can configure custom ZeroMQ endpoints:

```python
client = ProHandClient(
    command_endpoint="tcp://192.168.1.10:5555",
    streaming_endpoint="tcp://192.168.1.10:5556",
    status_endpoint="tcp://192.168.1.10:5557"
)
```

## Limitations

This SDK provides access to the public API only. Internal service commands (firmware updates, low-level register access, diagnostic queries) are not available through the SDK.

For advanced diagnostic tools and firmware development, contact the ProHand team for access to internal tools.

## Support

For SDK support, issues, or feature requests:
- Email: support@proception.ai
- Documentation: (documentation URL)
- GitHub Issues: (repository URL)

## License

See LICENSE file for details.

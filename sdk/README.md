# ProHand, ProGlove & ProWristCam SDK

Public SDK for ProHand robotic hand, ProGlove, and ProWristCam control systems.

## Overview

The SDK provides a clean, FFI-based API for controlling ProHand robotic hands and ProGlove devices, and for receiving ProWristCam video, through a local IPC (Inter-Process Communication) interface. This SDK is designed for external developers with support for multiple programming languages.

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
├── prowrist_sdk/           # ProWristCam SDK
│   ├── lib/                # Pre-built native libraries
│   ├── cpp/                # C++ headers and bindings
│   └── python/             # Python bindings
├── demo/                   # Example applications
│   ├── python/             # Python demo scripts
│   └── cpp/                # C++ demo applications
├── bin/                    # IPC host binaries
│   ├── prohand-headless-ipc-host      # ProHand IPC host
│   ├── proglove-headless-ipc-host     # ProGlove IPC host
│   └── prowristcam-headless-ipc-host  # ProWristCam IPC host
└── docs/                   # Documentation
    ├── API.md              # API reference
    └── EXAMPLES.md         # Usage examples
```

## Quick Start

### Prerequisites

1. A ProHand, ProGlove, or ProWristCam device connected via USB
1. The appropriate IPC host binary running:
   - For ProHand: `bin/prohand-headless-ipc-host`
   - For ProGlove: `bin/proglove-headless-ipc-host`
   - For ProWristCam: `bin/prowristcam-headless-ipc-host`

### Running the IPC Host

```bash
# For ProHand
./bin/prohand-headless-ipc-host

# For ProGlove
./bin/proglove-headless-ipc-host

# For ProWristCam
./bin/prowristcam-headless-ipc-host
```

The IPC host will:

- Auto-detect the connected device
- Create ZeroMQ endpoints for communication
- Stream device status and accept commands

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

#### ProWristCam SDK

```bash
cd prowrist_sdk/python
pip install -e .
```

Example usage:

```python
from prowrist_sdk import WristCamClient

# Subscribe to the wrist-camera stream endpoint
client = WristCamClient("ipc:///tmp/prowristcam-left-stream.ipc")

while True:
    frame = client.try_recv_frame()
    if frame is not None:
        # frame.jpeg is raw JPEG bytes; frame.uid is a rolling counter
        print(f"Frame uid={frame.uid} size={len(frame.jpeg)}")
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

#### ProWristCam SDK

```bash
cd prowrist_sdk/cpp
mkdir build && cd build
cmake ..
make
```

Example usage:

```cpp
#include <prowrist_sdk/WristCamClient.hpp>

int main() {
    using namespace prowrist_sdk;

    // Subscribe to the wrist-camera stream endpoint
    WristCamClient client("ipc:///tmp/prowristcam-left-stream.ipc");

    while (true) {
        if (auto frame = client.tryRecvFrame()) {
            // frame->jpeg is raw JPEG bytes; frame->uid is a rolling counter
            printf("Frame uid=%u size=%zu\n", frame->uid, frame->jpeg.size());
        }
    }

    return 0;
}
```

#### ProHand Cap'n Proto Rust SDK

A native, zero-copy Rust client (`prohand-client`) — an alternative to the FFI
SDKs above, speaking Cap'n Proto over ZMQ directly. Shipped as self-contained
source: the crate bundles its schemas and compiles them via `build.rs`.

```bash
cd prohand_capnp_sdk
cargo build --release
```

```toml
# Add it to your project (path shown; git / crates.io once published):
[dependencies]
prohand-client = { path = "path/to/prohand_capnp_sdk" }
```

```rust
use prohand_client::{ProHandServiceClient, ProHandPublisher, ProHandSubscriber};
```

See `prohand_capnp_sdk/README.md` for the full API (publish commands, subscribe
to status, request/reply services).

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

### ProWristCam API

The ProWristCam SDK is receive-only: it subscribes to the camera stream endpoint
and delivers decoded JPEG frames. There are no commands.

#### Client

- **WristCamClient(stream_endpoint)**: Subscribe to a ZMQ stream endpoint
- **try_recv_frame() / tryRecvFrame()**: Non-blocking; returns the next frame or none
- **is_connected() / isConnected()**: Connection state
- **get_version()**: SDK version string

#### Frame

- **JpegFrame / WristCamFrame**: `jpeg` (raw JPEG bytes), `uid` (rolling frame
  counter), `timestamp` (low-16 ms since epoch)

## Communication Architecture

```
┌─────────────────┐         ┌──────────────────┐         ┌─────────────┐
│   SDK Client    │◄────────┤  IPC Host        │◄────────┤   Device    │
│   (Your App)    │  ZMQ    │  (Prebuilt)      │   USB   │  (Hardware) │
└─────────────────┘         └──────────────────┘         └─────────────┘

ZMQ Endpoints (left-side defaults shown; the right side offsets each port by
10, e.g. command 5572. All endpoints are configurable — see "Custom IPC
Endpoints" below):
- Status (PUB/SUB): tcp://localhost:5561 - Receive device status
- Command (REQ/REP): tcp://localhost:5562 - Send commands, receive acks
- Streaming (PUB/SUB): tcp://localhost:5563 - Stream hand commands at high rate
- Wrist streaming (PUB/SUB): tcp://localhost:5564 - Wrist joint stream
- WristCam (PUB/SUB): ipc:///tmp/prowristcam-<side>-stream.ipc - JPEG frame stream
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
1. Verify device appears in system (macOS: `system_profiler SPUSBDataType`)
1. On Linux, check udev rules for device permissions

### Connection Failed

1. Ensure IPC host is running
1. Check firewall settings (ZeroMQ uses TCP)
1. Verify correct endpoint addresses

### Performance Issues

1. Use streaming mode for high-frequency commands (>10 Hz)
1. Check system CPU usage and USB bandwidth

### Custom IPC Endpoints

The endpoints above are defaults. You can point the client at any host/port:

```python
client = ProHandClient(
    command_endpoint="tcp://192.168.1.10:5562",
    streaming_endpoint="tcp://192.168.1.10:5563",
    status_endpoint="tcp://192.168.1.10:5561"
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

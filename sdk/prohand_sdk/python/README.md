# ProHand SDK - Python Client

Python bindings for the ProHand robotic hand control system.

## Installation

### Option 1: pip install (recommended)

```bash
cd prohand_sdk/python
pip install -e .
```

### Option 2: Direct import

```bash
export PYTHONPATH="${PYTHONPATH}:/path/to/prohand_sdk/python"
```

## Quick Start

```python
from prohand_sdk import ProHandClient, discover_usb_devices, get_version

# Check version
print(f"SDK Version: {get_version()}")

# Discover connected devices
devices = discover_usb_devices()
for device in devices:
    print(f"Found: {device.display_name}")

# Connect to device
with ProHandClient(
    "tcp://127.0.0.1:5562",  # Command endpoint
    "tcp://127.0.0.1:5561",  # Status endpoint
    "tcp://127.0.0.1:5563",  # Hand streaming endpoint
    "tcp://127.0.0.1:5564"   # Wrist streaming endpoint
) as client:
    # Send commands
    client.send_ping()
    
    # Control joints
    positions = [0.0] * 16  # Radians
    torques = [0.45] * 16   # Normalized 0.0-1.0
    client.send_rotary_commands(positions, torques)
    
    # Poll status
    status = client.try_recv_status()
    if status:
        print(f"Positions: {status.rotary_positions}")
```

## API Reference

### ProHandClient

Main client for controlling the ProHand device.

#### `__init__(command_endpoint: str, status_endpoint: str, hand_streaming_endpoint: str, wrist_streaming_endpoint: str)`

Create a new client connection.

**Parameters:**
- `command_endpoint`: ZeroMQ endpoint for commands (e.g., "tcp://127.0.0.1:5562")
- `status_endpoint`: ZeroMQ endpoint for status (e.g., "tcp://127.0.0.1:5561")
- `hand_streaming_endpoint`: ZeroMQ endpoint for hand streaming (e.g., "tcp://127.0.0.1:5563")
- `wrist_streaming_endpoint`: ZeroMQ endpoint for wrist streaming (e.g., "tcp://127.0.0.1:5564")

#### `send_ping() -> None`

Send a ping command to the device.

#### `set_streaming_mode(enabled: bool) -> None`

Enable or disable high-frequency streaming mode.

#### `send_rotary_commands(positions: List[float], torques: List[float]) -> None`

Control the 16 finger joints.

**Parameters:**
- `positions`: List of 16 joint positions in radians
- `torques`: List of 16 torque values (0.0 to 1.0)

#### `send_linear_commands(positions: List[float], speeds: List[float]) -> None`

Control the 2 wrist motors (low-level actuator control).

**Parameters:**
- `positions`: List of 2 positions in radians
- `speeds`: List of 2 speed values (0.0 to 1.0)

#### `send_wrist_command(positions: List[float], use_profiler: bool = False) -> None`

Control the 2 wrist joints (high-level joint control with inverse kinematics).

**Parameters:**
- `positions`: List of 2 wrist joint angles in radians
- `use_profiler`: Whether to enable wrist motion profiling (position-only, implicit max velocity)

**Note:** This is the high-level API that uses inverse kinematics. For low-level actuator control, use `send_linear_commands()`.

#### `send_wrist_streams(positions: List[float], use_profiler: bool = False) -> None`

Control the 2 wrist joints via streaming channel (high-frequency, requires streaming mode).

**Parameters:**
- `positions`: List of 2 wrist joint angles in radians
- `use_profiler`: Whether to enable wrist motion profiling (position-only, implicit max velocity)

**Requires:** Client created with streaming endpoint AND driver in streaming mode.

#### `send_hand_command(positions: List[float], torque: float = 0.45) -> None`

Control all 20 finger joints (high-level joint control with inverse kinematics).

**Parameters:**
- `positions`: List of 20 joint angles in radians (5 fingers × 4 joints)
  - Order: thumb[0-3], index[4-7], middle[8-11], ring[12-15], pinky[16-19]
- `torque`: Single torque value (0.0 to 1.0) applied to all joints

#### `send_hand_streams(positions: List[float], torque: float = 0.45) -> None`

Control all 20 finger joints via streaming channel (high-frequency, requires streaming mode).

**Parameters:**
- `positions`: List of 20 joint angles in radians (5 fingers × 4 joints)
- `torque`: Single torque value (0.0 to 1.0) applied to all joints

**Requires:** Client created with streaming endpoint AND driver in streaming mode.

#### `send_zero_calibration(mask: List[bool]) -> None`

Calibrate zero position for selected joints.

**Parameters:**
- `mask`: List of 16 boolean values indicating which joints to calibrate

#### `try_recv_status() -> Optional[HandStatus]`

Non-blocking status poll. Returns status if available, None otherwise.

### Functions

#### `discover_usb_devices(max_devices: int = 10) -> List[UsbDevice]`

Find connected ProHand devices via USB.

#### `get_version() -> str`

Get SDK version string.

### Data Classes

#### `HandStatus`

```python
@dataclass
class HandStatus:
    is_valid: bool
    status_type: int  # 0=unknown, 1=rotary, 2=linear
    rotary_positions: List[float]  # 16 joint positions in radians
    linear_positions: List[float]  # 2 wrist positions in radians
```

#### `UsbDevice`

```python
@dataclass
class UsbDevice:
    port_name: str
    display_name: str
```

### Streaming Mode

For high-frequency control (100+ Hz), use streaming mode:

```python
# Create client with all endpoints
client = ProHandClient(
    "tcp://127.0.0.1:5562",  # Command endpoint
    "tcp://127.0.0.1:5561",  # Status endpoint
    "tcp://127.0.0.1:5563",  # Hand streaming endpoint
    "tcp://127.0.0.1:5564"   # Wrist streaming endpoint
)

# Enable streaming mode
client.set_streaming_mode(True)

# Wait for streaming to be ready
if client.wait_for_streaming_ready():
    # Now use streaming methods (velocities are implicit max for wrist)
    client.send_hand_streams(positions, torque)
    client.send_wrist_streams(wrist_positions)
```

## Requirements

- Python 3.7 or later
- ProHand headless IPC host running

## Error Handling

```python
from prohand_sdk import ProHandClient, ProHandError, ConnectionError

try:
    client = ProHandClient(
        "tcp://127.0.0.1:5562",  # Command endpoint
        "tcp://127.0.0.1:5561",  # Status endpoint
        "tcp://127.0.0.1:5563",  # Hand streaming endpoint
        "tcp://127.0.0.1:5564"   # Wrist streaming endpoint
    )
    client.send_ping()
except ConnectionError as e:
    print(f"Connection failed: {e}")
except ProHandError as e:
    print(f"SDK error: {e}")
```

## Context Manager

ProHandClient supports the context manager protocol for automatic cleanup:

```python
with ProHandClient(...) as client:
    client.send_ping()
    # Automatically cleaned up on exit
```

## Notes

- The SDK uses ctypes to interface with the native library
- No external Python dependencies required
- Thread-safe for single client instance
- Status polling is non-blocking
- The native library is located in `../lib/` and is automatically discovered by the Python bindings
- Uses `pyproject.toml` for modern Python packaging (PEP 518/621)

## SDK Package

The Python bindings are included in this SDK package and ready to use.

This regenerates:
- `prohand_sdk/prohand_sdk.py`
- `../lib/libprohand_client_sdk.{dylib,so,dll}` (shared library location)

## License

© Proception AI, Inc. 2024-2025

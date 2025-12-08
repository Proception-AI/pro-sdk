# ProHand SDK API Reference

## Overview

The ProHand SDK provides a Cap'n Proto-based IPC interface for controlling ProHand robotic hands. This document describes the available commands and status messages.

## Communication Protocol

The SDK uses ZeroMQ (ZMQ) for IPC communication with three main endpoints:

1. **Command Endpoint** (REQ/REP pattern): Send service commands and receive acknowledgments
2. **Streaming Endpoint** (PUB/SUB pattern): Send high-frequency control commands
3. **Status Endpoint** (PUB/SUB pattern): Receive device status updates

## Message Types

### ProHandCommandSDK

The main command message type supporting the following variants:

#### ping
Test connectivity to the IPC host.

```
ping: Void
```

**Usage**: Send a ping to check if the IPC host is responsive.

#### timeSync
Synchronize timestamps between client and device.

```
timeSync: UInt64
```

**Parameters**:
- `timestamp`: Unix timestamp in milliseconds

#### handService
Control hand service modes.

```
handService: HandServiceType
```

**HandServiceType variants**:
- `available: Void` - Check if service is available
- `streamingMode: Bool` - Enable/disable streaming mode
- `autoCalibration: Bool` - Enable/disable auto-calibration
- `zeroCalib: List(Bool)` - Zero calibration mask (16 servos)
- `serviceMode: Bool` - Enable/disable service mode

**Example**: Enable streaming mode
```
handService = (streamingMode = true)
```

#### handStateCommand
High-level finger joint control.

```
handStateCommand: HandCommand
```

**HandCommand structure**:
```
struct HandCommand {
  timestamp: UInt16;
  uid: UInt16;
  thumb: List(CompactJointState);   # 4 joints
  index: List(CompactJointState);   # 4 joints
  middle: List(CompactJointState);  # 4 joints
  ring: List(CompactJointState);    # 4 joints
  pinky: List(CompactJointState);   # 4 joints
}
```

**CompactJointState**:
```
struct CompactJointState {
  normalizedPos: Int16;        # -32768 to 32767 (maps to joint range)
  normalizedVelOrTau: Int16;   # -32768 to 32767 (velocity or torque)
}
```

#### wristStateCommand
Wrist joint control.

```
wristStateCommand: WristCommand
```

**WristCommand structure**:
```
struct WristCommand {
  timestamp: UInt16;
  uid: UInt16;
  wrist: List(CompactJointState);  # 2 joints (pitch, yaw)
}
```

#### rotaryGrpCommand
Direct servo position/torque control (low-level).

```
rotaryGrpCommand: List(RotaryCommand)
```

**RotaryCommand structure**:
```
struct RotaryCommand {
  position: Int16;  # Target position
  torque: UInt16;   # Torque limit
}
```

**Note**: List must contain exactly 16 commands (one per servo).

#### linearGrpCommand
Linear actuator control.

```
linearGrpCommand: List(LinearCommand)
```

**LinearCommand structure**:
```
struct LinearCommand {
  position: Int16;  # Target position
  speed: UInt16;    # Speed
}
```

**Note**: List must contain exactly 2 commands (one per actuator).

---

### ProHandStatusSDK

Status messages received from the device:

#### pong
Response to ping command.

```
pong: Void
```

#### handService
Hand service status response.

```
handService: HandServiceType
```

#### rotaryState / linearState
Individual servo or actuator state.

```
rotaryState: ServoState
linearState: ServoState
```

**ServoState structure**:
```
struct ServoState {
  id: UInt8;
  state: ServoStateEnum;  # Idle, Running, Error, etc.
  position: Int16;
  velocity: Int16;
  torque: Int16;
  temperature: UInt8;
  voltage: UInt16;
}
```

#### handState
Complete hand state (all joints).

```
handState: HandState
```

#### handAlert / rotaryAlert / linearAlert
Error alerts from hand or servos.

```
handAlert: HandError
rotaryAlert: ServoError
linearAlert: ServoError
```

#### rotaryGrpStatus / linearGrpStatus
Bulk status for all servos/actuators.

```
rotaryGrpStatus: List(RotaryStatus)  # 16 items
linearGrpStatus: List(LinearStatus)  # 2 items
```

**RotaryStatus structure**:
```
struct RotaryStatus {
  position: Int16;
  velocity: Int16;
  torque: Int16;
  temperature: UInt8;
  voltage: UInt16;
}
```

**LinearStatus structure**:
```
struct LinearStatus {
  position: Int16;
  current: Int16;
  speed: Int16;
  error: UInt8;
  temp: UInt8;
}
```

#### rotaryGrpTarget / linearGrpTarget
Echo of commanded targets (confirmation).

```
rotaryGrpTarget: List(RotaryCommand)  # 16 items
linearGrpTarget: List(LinearCommand)  # 2 items
```

#### handedness
Hand chirality (left or right).

```
handedness: Handedness
```

**Handedness enum**:
- `left`
- `right`

---

## Usage Patterns

### Service Commands (REQ/REP)

Service commands use the request/reply pattern. Client sends a command and waits for an acknowledgment.

**Typical flow**:
1. Connect to command endpoint
2. Send command (serialized ProHandCommandSDK)
3. Wait for reply (empty acknowledgment)
4. Check status endpoint for actual response

**Use cases**:
- Ping
- Enable/disable streaming mode
- Zero calibration
- Service mode control

### Streaming Commands (PUB/SUB)

Streaming commands are published at high frequency without acknowledgment.

**Typical flow**:
1. Enable streaming mode (via service command)
2. Connect to streaming endpoint
3. Publish commands continuously
4. No reply expected

**Use cases**:
- HandStateCommand (finger control)
- WristStateCommand
- RotaryGrpCommand (servo control)
- LinearGrpCommand

### Status Messages (PUB/SUB)

Status messages are published by the IPC host based on device updates.

**Typical flow**:
1. Connect to status endpoint
2. Subscribe to all topics (empty subscription)
3. Receive status messages asynchronously

**Use cases**:
- Monitor servo states
- Detect errors
- Confirm commanded targets
- Track hand state

---

## Error Handling

### Connection Errors

If connection fails:
- Check that IPC host is running
- Verify endpoint addresses
- Check firewall settings (if using TCP)

### Deserialization Errors

If messages fail to deserialize:
- Verify schema compatibility
- Check that you're using the SDK schema (not internal)
- Ensure Cap'n Proto version matches

### Command Errors

If commands are not executed:
- Check streaming mode is enabled (for streaming commands)
- Verify device is connected
- Check status messages for error alerts

---

## Schema Files

All message structures are defined in Cap'n Proto schema files located in `schemas/`:

- `prohand_sdk.capnp` - Main SDK schema
- `msg/hand.capnp` - Hand-level message types
- `msg/joint.capnp` - Joint state types
- `msg/rotary.capnp` - Rotary servo types
- `msg/linear.capnp` - Linear actuator types

---

## Type Ranges and Units

### Position Values
- **Normalized**: `-32768` to `32767` (Int16)
- **Interpretation**: Maps linearly to joint range
  - For rotary servos: typically -180° to +180°
  - For linear actuators: typically 0mm to max stroke

### Velocity/Torque Values
- **Normalized**: `-32768` to `32767` (Int16)
- **Interpretation**: Context-dependent
  - For fingers: torque percentage (-100% to +100%)
  - For wrist: velocity percentage

### Temperature
- **Range**: `0` to `255` (UInt8)
- **Unit**: Degrees Celsius

### Voltage
- **Range**: `0` to `65535` (UInt16)
- **Unit**: Millivolts (mV)

---

## Best Practices

1. **Enable Streaming Mode**: Always enable streaming mode before sending high-frequency commands
2. **Use Service Commands for Configuration**: Ping, calibration, and mode changes should use the command endpoint
3. **Subscribe to Status**: Always monitor the status endpoint for errors and confirmations
4. **Set Appropriate Torque Limits**: When using RotaryGrpCommand, set reasonable torque limits to protect the hardware
5. **Handle Disconnections Gracefully**: Implement reconnection logic for robust operation

---

## Version Compatibility

The SDK schema uses semantic versioning. Check the schema file header for version information:

```capnp
# Generated: 2025-11-11 20:52:08 UTC
# Hash: ca7f57ec2030fbb1
# Edition: Public SDK
```

The IPC host supports both SDK and internal schemas, so SDK clients can coexist with diagnostic tools.


# ProHand SDK Examples

This document provides practical examples for using the ProHand SDK in various programming languages.

## Table of Contents

1. [Setup and Connection](#setup-and-connection)
2. [Basic Commands](#basic-commands)
3. [Hand Control](#hand-control)
4. [Streaming Mode](#streaming-mode)
5. [Status Monitoring](#status-monitoring)
6. [Error Handling](#error-handling)
7. [Complete Applications](#complete-applications)

---

## Setup and Connection

### Rust

```rust
use prohand_client::{ProHandServiceClient, ProHandPublisher, ProHandSubscriber};
use std::error::Error;

fn main() -> Result<(), Box<dyn Error>> {
    // Initialize clients
    let mut service_client = ProHandServiceClient::new("tcp://localhost:5555");
    let mut publisher = ProHandPublisher::new("tcp://localhost:5556");
    let mut subscriber = ProHandSubscriber::new("tcp://localhost:5557");
    
    // Connect to IPC endpoints
    service_client.connect()?;
    publisher.connect()?;
    subscriber.connect()?;
    
    println!("Connected to ProHand IPC host");
    
    Ok(())
}
```

### Python

```python
import zmq
from prohand_sdk import ProHandClient

def main():
    # Create client with default endpoints
    client = ProHandClient(
        command_endpoint="tcp://localhost:5555",
        streaming_endpoint="tcp://localhost:5556",
        status_endpoint="tcp://localhost:5557"
    )
    
    print("Connected to ProHand IPC host")
    return client

if __name__ == "__main__":
    client = main()
```

### C++

```cpp
#include "prohand_client.hpp"
#include <iostream>

int main() {
    try {
        // Create clients
        prohand::ServiceClient service_client("tcp://localhost:5555");
        prohand::Publisher publisher("tcp://localhost:5556");
        prohand::Subscriber subscriber("tcp://localhost:5557");
        
        std::cout << "Connected to ProHand IPC host" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

---

## Basic Commands

### Ping

Test connectivity to the IPC host.

**Rust:**
```rust
// Send ping with 1000ms timeout
let success = service_client.send_ping(1000)?;
if success {
    println!("Ping successful!");
} else {
    println!("Ping timeout");
}
```

**Python:**
```python
# Send ping
if client.ping(timeout_ms=1000):
    print("Ping successful!")
else:
    print("Ping timeout")
```

### Enable Streaming Mode

Enable high-frequency command streaming.

**Rust:**
```rust
// Enable streaming mode
let success = service_client.send_streaming_mode(true, 1000)?;
if success {
    println!("Streaming mode enabled");
}
```

**Python:**
```python
# Enable streaming mode
if client.set_streaming_mode(True, timeout_ms=1000):
    print("Streaming mode enabled")
```

### Zero Calibration

Calibrate specific servos to zero position.

**Rust:**
```rust
// Calibrate servo ID 3
let success = service_client.send_zero_calibration(Some(3), 5000)?;

// Calibrate all servos (pass None)
let success = service_client.send_zero_calibration(None, 10000)?;
```

**Python:**
```python
# Calibrate servo ID 3
client.send_zero_calibration(motor_id=3, timeout_ms=5000)

# Calibrate all servos
client.send_zero_calibration(motor_id=None, timeout_ms=10000)
```

---

## Hand Control

### High-Level Hand Commands

Control fingers using joint angles.

**Rust:**
```rust
use std::collections::HashMap;

// Create hand pose (angles in radians)
let mut hand_pose = HashMap::new();
hand_pose.insert("thumb".to_string(), vec![0.0, 0.5, 0.5, 0.5]);
hand_pose.insert("index".to_string(), vec![0.0, 1.0, 1.0, 1.0]);
hand_pose.insert("middle".to_string(), vec![0.0, 1.0, 1.0, 1.0]);
hand_pose.insert("ring".to_string(), vec![0.0, 0.8, 0.8, 0.8]);
hand_pose.insert("pinky".to_string(), vec![0.0, 0.8, 0.8, 0.8]);
hand_pose.insert("wrist".to_string(), vec![0.0, 0.0]); // pitch, yaw

// Send command (angles in radians, 30% torque)
publisher.send_hand_command(&hand_pose, false, 0.3, None)?;
```

**Python:**
```python
# Create hand pose (angles in radians)
hand_pose = {
    "thumb": [0.0, 0.5, 0.5, 0.5],
    "index": [0.0, 1.0, 1.0, 1.0],
    "middle": [0.0, 1.0, 1.0, 1.0],
    "ring": [0.0, 0.8, 0.8, 0.8],
    "pinky": [0.0, 0.8, 0.8, 0.8],
    "wrist": [0.0, 0.0],  # pitch, yaw
}

# Send command (angles in radians, 30% torque)
client.send_hand_command(hand_pose, degrees=False, torque_level=0.3)
```

**Using Degrees:**
```rust
// Same pose in degrees
hand_pose.insert("index".to_string(), vec![0.0, 57.3, 57.3, 57.3]);
publisher.send_hand_command(&hand_pose, true, 0.3, None)?; // degrees=true
```

```python
# Same pose in degrees
hand_pose["index"] = [0.0, 57.3, 57.3, 57.3]
client.send_hand_command(hand_pose, degrees=True, torque_level=0.3)
```

### Low-Level Servo Control

Direct control of individual servos.

**Rust:**
```rust
// Create rotary commands (position, torque for each servo)
let positions: Vec<i16> = vec![100, 200, 150, 300, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
let torques: Vec<u16> = vec![50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

publisher.send_rotary_command(&positions, &torques)?;
```

**Python:**
```python
# Create rotary commands
positions = [100, 200, 150, 300] + [0] * 12
torques = [50, 50, 50, 50] + [0] * 12

client.send_rotary_command(positions, torques)
```

---

## Streaming Mode

### Continuous Hand Control Loop

**Rust:**
```rust
use std::time::Duration;
use std::thread;

// Enable streaming mode first
service_client.send_streaming_mode(true, 1000)?;

// Streaming loop
for i in 0..1000 {
    // Calculate smooth hand motion
    let angle = (i as f32 * 0.01).sin() * 0.5; // Oscillate 0 to 0.5 rad
    
    let mut hand_pose = HashMap::new();
    hand_pose.insert("index".to_string(), vec![0.0, angle, angle, angle]);
    hand_pose.insert("middle".to_string(), vec![0.0, angle, angle, angle]);
    
    // Send at ~100Hz
    publisher.send_hand_command(&hand_pose, false, 0.3, None)?;
    thread::sleep(Duration::from_millis(10));
}
```

**Python:**
```python
import time
import math

# Enable streaming mode
client.set_streaming_mode(True)

# Streaming loop
for i in range(1000):
    # Calculate smooth hand motion
    angle = math.sin(i * 0.01) * 0.5  # Oscillate 0 to 0.5 rad
    
    hand_pose = {
        "index": [0.0, angle, angle, angle],
        "middle": [0.0, angle, angle, angle],
    }
    
    # Send at ~100Hz
    client.send_hand_command(hand_pose, degrees=False, torque_level=0.3)
    time.sleep(0.01)
```

---

## Status Monitoring

### Receive and Process Status Messages

**Rust:**
```rust
use prohand_capnp::ProHandStatus;

// Poll for status messages
loop {
    match subscriber.try_receive_status()? {
        Some(status) => {
            match status {
                ProHandStatus::Pong => {
                    println!("Received pong");
                }
                ProHandStatus::RotaryGrpStatus(statuses) => {
                    println!("Servo positions: {:?}", 
                        statuses.iter().map(|s| s.position).collect::<Vec<_>>());
                }
                ProHandStatus::HandState(state) => {
                    println!("Hand state: {:?}", state);
                }
                ProHandStatus::HandAlert(error) => {
                    eprintln!("Hand error: {:?}", error);
                }
                _ => {}
            }
        }
        None => {
            // No message available
            thread::sleep(Duration::from_millis(1));
        }
    }
}
```

**Python:**
```python
# Continuously monitor status
while True:
    status = client.receive_status(timeout_ms=100)
    
    if status is None:
        continue
        
    if status['type'] == 'pong':
        print("Received pong")
    elif status['type'] == 'rotary_group_status':
        positions = [s['position'] for s in status['servos']]
        print(f"Servo positions: {positions}")
    elif status['type'] == 'hand_state':
        print(f"Hand state: {status['state']}")
    elif status['type'] == 'hand_alert':
        print(f"Hand error: {status['error']}")
```

---

## Error Handling

### Robust Connection with Retry

**Rust:**
```rust
fn connect_with_retry(max_retries: usize) -> Result<ProHandServiceClient, Box<dyn Error>> {
    let mut client = ProHandServiceClient::new("tcp://localhost:5555");
    
    for attempt in 1..=max_retries {
        match client.connect() {
            Ok(()) => {
                println!("Connected successfully");
                return Ok(client);
            }
            Err(e) => {
                eprintln!("Connection attempt {} failed: {}", attempt, e);
                if attempt < max_retries {
                    thread::sleep(Duration::from_secs(2));
                }
            }
        }
    }
    
    Err("Failed to connect after retries".into())
}
```

**Python:**
```python
import time

def connect_with_retry(max_retries=5):
    for attempt in range(1, max_retries + 1):
        try:
            client = ProHandClient(
                command_endpoint="tcp://localhost:5555",
                streaming_endpoint="tcp://localhost:5556",
                status_endpoint="tcp://localhost:5557"
            )
            print("Connected successfully")
            return client
        except Exception as e:
            print(f"Connection attempt {attempt} failed: {e}")
            if attempt < max_retries:
                time.sleep(2)
    
    raise Exception("Failed to connect after retries")
```

---

## Complete Applications

### Example: Gesture Player

Play back pre-recorded hand gestures.

**Rust:**
```rust
use prohand_client::{ProHandServiceClient, ProHandPublisher};
use std::collections::HashMap;
use std::time::Duration;
use std::thread;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Connect
    let mut service = ProHandServiceClient::new("tcp://localhost:5555");
    service.connect()?;
    
    let mut publisher = ProHandPublisher::new("tcp://localhost:5556");
    publisher.connect()?;
    
    // Enable streaming
    service.send_streaming_mode(true, 1000)?;
    
    // Define gestures
    let open_hand: HashMap<String, Vec<f32>> = [
        ("thumb", vec![0.0, 0.0, 0.0, 0.0]),
        ("index", vec![0.0, 0.0, 0.0, 0.0]),
        ("middle", vec![0.0, 0.0, 0.0, 0.0]),
        ("ring", vec![0.0, 0.0, 0.0, 0.0]),
        ("pinky", vec![0.0, 0.0, 0.0, 0.0]),
    ].iter().cloned().map(|(k, v)| (k.to_string(), v)).collect();
    
    let closed_hand: HashMap<String, Vec<f32>> = [
        ("thumb", vec![0.0, 1.2, 1.2, 1.2]),
        ("index", vec![0.0, 1.5, 1.5, 1.5]),
        ("middle", vec![0.0, 1.5, 1.5, 1.5]),
        ("ring", vec![0.0, 1.5, 1.5, 1.5]),
        ("pinky", vec![0.0, 1.5, 1.5, 1.5]),
    ].iter().cloned().map(|(k, v)| (k.to_string(), v)).collect();
    
    // Play gesture sequence
    loop {
        // Open hand
        println!("Opening hand...");
        for _ in 0..100 {
            publisher.send_hand_command(&open_hand, false, 0.3, None)?;
            thread::sleep(Duration::from_millis(10));
        }
        
        thread::sleep(Duration::from_secs(1));
        
        // Close hand
        println!("Closing hand...");
        for _ in 0..100 {
            publisher.send_hand_command(&closed_hand, false, 0.5, None)?;
            thread::sleep(Duration::from_millis(10));
        }
        
        thread::sleep(Duration::from_secs(1));
    }
}
```

### Example: Teleoperation

Mirror input device to ProHand.

**Python:**
```python
import time
from prohand_sdk import ProHandClient

def map_input_to_hand(input_data):
    """Map your input device data to hand pose"""
    # Implement your mapping logic here
    # This is a placeholder
    return {
        "thumb": input_data.get("thumb", [0.0] * 4),
        "index": input_data.get("index", [0.0] * 4),
        "middle": input_data.get("middle", [0.0] * 4),
        "ring": input_data.get("ring", [0.0] * 4),
        "pinky": input_data.get("pinky", [0.0] * 4),
    }

def main():
    # Connect
    client = ProHandClient(
        command_endpoint="tcp://localhost:5555",
        streaming_endpoint="tcp://localhost:5556",
        status_endpoint="tcp://localhost:5557"
    )
    
    # Enable streaming
    client.set_streaming_mode(True)
    
    print("Starting teleoperation...")
    
    while True:
        # Read from your input device
        # input_data = read_input_device()
        input_data = {}  # Placeholder
        
        # Map to hand pose
        hand_pose = map_input_to_hand(input_data)
        
        # Send command
        client.send_hand_command(hand_pose, degrees=False, torque_level=0.3)
        
        # Maintain loop rate
        time.sleep(0.01)  # 100 Hz

if __name__ == "__main__":
    main()
```

---

## Tips and Best Practices

1. **Always enable streaming mode** before sending high-frequency commands
2. **Monitor status messages** to detect errors early
3. **Use appropriate torque limits** to protect hardware (typically 0.2-0.5)
4. **Implement graceful shutdown** to return hand to safe position
5. **Handle disconnections** with automatic reconnection logic
6. **Limit command rate** to reasonable frequencies (50-200 Hz)
7. **Test with low torque** when developing new behaviors

## Troubleshooting

### Connection Issues
- Ensure IPC host is running
- Check endpoint addresses match
- Verify no firewall blocking ports

### Commands Not Executed
- Verify streaming mode is enabled
- Check device is connected
- Monitor status for error messages

### Poor Performance
- Reduce command frequency
- Check CPU usage on IPC host
- Verify USB connection quality

---

For more information, see [API.md](API.md) for complete API reference.


"""
ProHand Client SDK - Python Bindings

This module provides Python bindings for the ProHand Client SDK using ctypes.

Usage:
    from prohand_sdk import ProHandClient

    # Create client with all endpoints
    client = ProHandClient(
        "tcp://127.0.0.1:5562",  # Command endpoint
        "tcp://127.0.0.1:5561",  # Status endpoint
        "tcp://127.0.0.1:5563",   # Hand streaming endpoint
        "tcp://127.0.0.1:5564"    # Wrist streaming endpoint
    )

    # Check connection
    if client.is_connected():
        print("Connected to ProHand device")

    # Send commands via command channel
    positions = [0.0] * 16
    torques = [0.45] * 16
    client.send_rotary_commands(positions, torques)

    # Send commands via streaming channel (high-frequency)
    client.set_streaming_mode(True)
    client.wait_for_streaming_ready()
    client.send_rotary_streams(positions, torques)

    # Receive status
    status = client.try_recv_status()
    if status and status.is_valid:
        if status.status_type == 1:
            print(f"Rotary positions: {status.rotary_positions}")
        elif status.status_type == 2:
            print(f"Linear positions: {status.linear_positions}")

    # Clean up
    client.close()

Requirements:
    - Place the compiled library (libprohand_client_sdk.so/.dylib/.dll) in:
      - ../lib/ relative to this script (recommended shared location)
      - Same directory as this script (legacy)
      - System library path
      - Or set PROHAND_SDK_LIB environment variable
"""

import ctypes
import os
import sys
from ctypes import (
    POINTER, c_char_p, c_int, c_float, c_uint16, c_uint64, c_bool,
    Structure, pointer, cast
)
from typing import List, Optional, Tuple
from dataclasses import dataclass
from enum import IntEnum


# ============================================================================
# LIBRARY LOADING
# ============================================================================

def _find_library():
    """Find the ProHand SDK library"""
    # Check environment variable first
    env_path = os.environ.get('PROHAND_SDK_LIB')
    if env_path and os.path.exists(env_path):
        return env_path
    
    # Determine library name based on platform and architecture
    import platform
    machine = platform.machine()
    
    if sys.platform == 'darwin':
        lib_name = 'libprohand_client_sdk.dylib'
    elif sys.platform == 'win32':
        lib_name = 'prohand_client_sdk.dll'
    elif sys.platform.startswith('linux'):
        # On aarch64 (Jetson Nano), use the _aarch64 variant
        if machine == 'aarch64':
            lib_name = 'libprohand_client_sdk_aarch64.so'
        elif machine in ('x86_64', 'amd64', 'i686', 'i386'):
            lib_name = 'libprohand_client_sdk.so'
        else:
            raise RuntimeError(
                f"Unsupported Linux architecture: {machine}. "
                f"Supported architectures: x86_64, aarch64"
            )
    else:
        raise RuntimeError(
            f"Unsupported platform: {sys.platform}. "
            f"Supported platforms: darwin, win32, linux"
        )
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Check in ../lib/ (shared library location)
    lib_path = os.path.join(script_dir, '..', '..', 'lib', lib_name)
    if os.path.exists(lib_path):
        return lib_path
    
    # Try loading from system library path
    try:
        return lib_name
    except Exception:
        raise RuntimeError(
            f"Could not find ProHand SDK library. "
            f"Expected '{lib_name}' in:\n"
            f"  - {os.path.join(script_dir, '..', '..', 'lib')}\n"
            f"  - System library path\n"
            f"  - Or set PROHAND_SDK_LIB environment variable"
        )


# Load the library
_lib_path = _find_library()
_lib = ctypes.CDLL(_lib_path)


# ============================================================================
# ENUMS AND CONSTANTS
# ============================================================================

class ProHandResult(IntEnum):
    """Result codes returned by SDK functions"""
    SUCCESS = 0
    ERROR_NULL = -1
    ERROR_CONNECTION = -2
    ERROR_INVALID_ARGUMENT = -3
    ERROR_NOT_CONNECTED = -4
    ERROR_UNSUPPORTED = -5
    ERROR_OTHER = -99


# ============================================================================
# STRUCTURES
# ============================================================================

class ProHandClientHandle(Structure):
    """Opaque handle to ProHand client (do not instantiate directly)"""
    pass


class ProHandUsbDeviceInfo(Structure):
    """USB device information"""
    _fields_ = [
        ("port_name", c_char_p),
        ("display_name", c_char_p),
    ]


class ProHandStatusInfo(Structure):
    """Hand status information"""
    _fields_ = [
        ("is_valid", c_int),
        ("status_type", c_int),
        ("rotary_positions", c_float * 16),
        ("linear_positions", c_float * 2),
    ]


@dataclass
class UsbDevice:
    """Python-friendly USB device info"""
    port_name: str
    display_name: str


@dataclass
class HandStatus:
    """Python-friendly hand status"""
    is_valid: bool
    status_type: int  # 0=unknown, 1=rotary, 2=linear
    rotary_positions: List[float]
    linear_positions: List[float]


# ============================================================================
# FUNCTION SIGNATURES
# ============================================================================

# Client lifecycle
_lib.prohand_client_create.argtypes = [c_char_p, c_char_p, c_char_p, c_char_p]
_lib.prohand_client_create.restype = POINTER(ProHandClientHandle)

_lib.prohand_client_destroy.argtypes = [POINTER(ProHandClientHandle)]
_lib.prohand_client_destroy.restype = None

_lib.prohand_client_is_connected.argtypes = [POINTER(ProHandClientHandle)]
_lib.prohand_client_is_connected.restype = c_int

# Commands
_lib.prohand_send_ping.argtypes = [POINTER(ProHandClientHandle)]
_lib.prohand_send_ping.restype = c_int

_lib.prohand_set_streaming_mode.argtypes = [POINTER(ProHandClientHandle), c_int]
_lib.prohand_set_streaming_mode.restype = c_int

_lib.prohand_send_rotary_commands.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(c_float),
    POINTER(c_float)
]
_lib.prohand_send_rotary_commands.restype = c_int

_lib.prohand_send_rotary_streams.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(c_float),
    POINTER(c_float),
]
_lib.prohand_send_rotary_streams.restype = c_int

_lib.prohand_send_linear_commands.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(c_float),
    POINTER(c_float)
]
_lib.prohand_send_linear_commands.restype = c_int

_lib.prohand_send_linear_streams.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(c_float),
    POINTER(c_float),
]
_lib.prohand_send_linear_streams.restype = c_int

# Wrist command (high-level wrist joints) - REQ/REP and streaming channels
_lib.prohand_send_wrist_command.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(c_float),  # 2 positions (wrist joints)
    c_bool,            # use_profiler
]
_lib.prohand_send_wrist_command.restype = c_int

_lib.prohand_send_wrist_streams.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(c_float),  # 2 positions (wrist joints)
    c_bool,            # use_profiler
]
_lib.prohand_send_wrist_streams.restype = c_int

# Wrist limits (optional; works only if motion profiler enabled in the build)
_lib.prohand_set_wrist_limits.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(c_float),  # max_velocity[2]
    POINTER(c_float),  # max_acceleration[2]
    POINTER(c_float),  # max_jerk[2]
]
_lib.prohand_set_wrist_limits.restype = c_int

# Hand command (high-level joint angles) - REQ/REP command channel
_lib.prohand_send_hand_command.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(c_float),  # 20 positions (5 fingers × 4 joints)
    c_float,  # torque
]
_lib.prohand_send_hand_command.restype = c_int

# Hand command (high-level joint angles) - PUB/SUB streaming channel
_lib.prohand_send_hand_streams.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(c_float),  # 20 positions (5 fingers × 4 joints)
    c_float,  # torque
]
_lib.prohand_send_hand_streams.restype = c_int

_lib.prohand_send_zero_calibration.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(c_int)
]
_lib.prohand_send_zero_calibration.restype = c_int

# USB discovery
_lib.prohand_discover_usb_devices.argtypes = [
    POINTER(ProHandUsbDeviceInfo),
    c_int
]
_lib.prohand_discover_usb_devices.restype = c_int

_lib.prohand_free_string.argtypes = [c_char_p]
_lib.prohand_free_string.restype = None

# Status polling
_lib.prohand_try_recv_status.argtypes = [
    POINTER(ProHandClientHandle),
    POINTER(ProHandStatusInfo)
]
_lib.prohand_try_recv_status.restype = c_int

_lib.prohand_is_running_state.argtypes = [POINTER(ProHandClientHandle)]
_lib.prohand_is_running_state.restype = c_int

# Version
_lib.prohand_get_version.argtypes = []
_lib.prohand_get_version.restype = c_char_p


# ============================================================================
# EXCEPTIONS
# ============================================================================

class ProHandError(Exception):
    """Base exception for ProHand SDK errors"""
    pass


class ConnectionError(ProHandError):
    """Connection-related errors"""
    pass


class InvalidArgumentError(ProHandError):
    """Invalid argument errors"""
    pass


def _check_result(result: int, operation: str = "operation"):
    """Check result code and raise exception if error"""
    if result == ProHandResult.SUCCESS:
        return
    elif result == ProHandResult.ERROR_NULL:
        raise ProHandError(f"{operation}: Null pointer error")
    elif result == ProHandResult.ERROR_CONNECTION:
        raise ConnectionError(f"{operation}: Connection error")
    elif result == ProHandResult.ERROR_INVALID_ARGUMENT:
        raise InvalidArgumentError(f"{operation}: Invalid argument")
    elif result == ProHandResult.ERROR_NOT_CONNECTED:
        raise ConnectionError(f"{operation}: Not connected")
    elif result == ProHandResult.ERROR_UNSUPPORTED:
        raise ProHandError(f"{operation}: Feature not supported (may be disabled in build)")
    else:
        raise ProHandError(f"{operation}: Unknown error ({result})")


# ============================================================================
# HIGH-LEVEL PYTHON API
# ============================================================================

class ProHandClient:
    """
    ProHand Client SDK - Python Interface
    
    This class provides a high-level Python interface to the ProHand device.
    """

    def __init__(
        self,
        command_endpoint: str,
        status_endpoint: str,
        hand_streaming_endpoint: str,
        wrist_streaming_endpoint: str,
    ):
        """
        Create a new ProHand client

        Args:
            command_endpoint: ZeroMQ endpoint for commands (e.g., "tcp://127.0.0.1:5562")
            status_endpoint: ZeroMQ endpoint for status (e.g., "tcp://127.0.0.1:5561")
            hand_streaming_endpoint: ZeroMQ endpoint for hand streaming (e.g., "tcp://127.0.0.1:5563")
            wrist_streaming_endpoint: ZeroMQ endpoint for wrist streaming (e.g., "tcp://127.0.0.1:5564")

        Raises:
            ConnectionError: If connection fails
        """
        cmd_bytes = command_endpoint.encode('utf-8')
        status_bytes = status_endpoint.encode('utf-8')
        hand_streaming_bytes = hand_streaming_endpoint.encode('utf-8')
        wrist_streaming_bytes = wrist_streaming_endpoint.encode('utf-8')

        self._handle = _lib.prohand_client_create(
            cmd_bytes, status_bytes, hand_streaming_bytes, wrist_streaming_bytes
        )

        if not self._handle:
            raise ConnectionError("Failed to create ProHand client")

        self._closed = False

    def __del__(self):
        """Clean up resources"""
        self.close()

    def __enter__(self):
        """Context manager entry"""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.close()

    def close(self):
        """Close the client and free resources"""
        if not self._closed and self._handle:
            _lib.prohand_client_destroy(self._handle)
            self._handle = None
            self._closed = True

    def is_connected(self) -> bool:
        """Check if connected to device"""
        if self._closed:
            return False
        return bool(_lib.prohand_client_is_connected(self._handle))

    # ========================================================================
    # COMMAND METHODS
    # ========================================================================

    def send_ping(self):
        """Send a ping command"""
        result = _lib.prohand_send_ping(self._handle)
        _check_result(result, "send_ping")

    def set_streaming_mode(self, enabled: bool):
        """Enable or disable streaming mode"""
        result = _lib.prohand_set_streaming_mode(self._handle, int(enabled))
        _check_result(result, "set_streaming_mode")

    def is_running_state(self) -> bool:
        """
        Check if the driver is in Running state (streaming active)

        Polls the status channel and checks if RotaryState or LinearState
        is in Running mode, which indicates streaming is truly active.

        Returns:
            True if in running state, False otherwise
        """
        result = _lib.prohand_is_running_state(self._handle)
        return result == 1

    def wait_for_streaming_ready(
        self, timeout: float = 1.0, retry_interval: float = 0.3
    ) -> bool:
        """
        Wait for streaming connection to be established with state verification

        This method repeatedly sends set_streaming_mode(True) and polls for
        Running state until confirmed or timeout.

        Args:
            timeout: Maximum time to wait in seconds (default: 1.0)
            retry_interval: How often to retry set_streaming_mode in seconds (default: 0.3)

        Returns:
            True if ready and in Running state, False if timeout

        Example:
            client.set_streaming_mode(True)
            if client.wait_for_streaming_ready():
                # Driver is confirmed in Running state
                client.send_rotary_streams(positions, torques)
        """
        import time

        # First, verify command channel is working
        try:
            self.send_ping()
        except:
            return False

        start_time = time.time()
        last_retry_time = start_time
        poll_interval = 0.05  # Poll every 50ms

        # Initial delay for ZMQ PUB/SUB connection to establish
        time.sleep(0.2)

        # Keep retrying set_streaming_mode until Running state is detected
        while (time.time() - start_time) < timeout:
            # Check if driver reports Running state
            if self.is_running_state():
                return True

            # Retry set_streaming_mode if enough time has passed
            elapsed_since_retry = time.time() - last_retry_time
            if elapsed_since_retry >= retry_interval:
                try:
                    self.set_streaming_mode(True)
                    last_retry_time = time.time()
                except:
                    pass  # Ignore errors, keep trying

            # Wait before next poll
            remaining = timeout - (time.time() - start_time)
            if remaining > 0:
                time.sleep(min(poll_interval, remaining))
            else:
                break

        # Timeout - check one last time
        return self.is_running_state()

    def send_rotary_commands(self, positions: List[float], torques: List[float]):
        """
        Send rotary motor commands via REQ/REP command channel

        Uses the command socket. Not suitable for high-frequency control.
        For high-frequency commands, use send_rotary_streams() instead.

        Args:
            positions: List of 16 position values in radians
            torques: List of 16 torque values (normalized 0.0 to 1.0)
        """
        if len(positions) != 16 or len(torques) != 16:
            raise InvalidArgumentError("positions and torques must have 16 elements")

        pos_array = (c_float * 16)(*positions)
        torque_array = (c_float * 16)(*torques)

        result = _lib.prohand_send_rotary_commands(
            self._handle, pos_array, torque_array
        )
        _check_result(result, "send_rotary_commands")

    def send_rotary_streams(self, positions: List[float], torques: List[float]):
        """
        Send rotary motor commands via PUB/SUB streaming channel

        Uses the streaming socket for high-frequency control (100+ Hz).
        Requires: Client created with streaming endpoint AND driver in streaming mode.

        Args:
            positions: List of 16 position values in radians
            torques: List of 16 torque values (normalized 0.0 to 1.0)

        Raises:
            ConnectionError: If streaming endpoint was not provided or driver not in streaming mode
        """
        if len(positions) != 16 or len(torques) != 16:
            raise InvalidArgumentError("positions and torques must have 16 elements")

        pos_array = (c_float * 16)(*positions)
        torque_array = (c_float * 16)(*torques)

        result = _lib.prohand_send_rotary_streams(self._handle, pos_array, torque_array)
        _check_result(result, "send_rotary_streams")

    def send_linear_commands(self, positions: List[float], speeds: List[float]):
        """
        Send linear motor commands via REQ/REP command channel

        Uses the command socket. For high-frequency commands, use
        send_linear_streams() instead.

        Args:
            positions: List of 2 position values in radians
            speeds: List of 2 speed values (normalized 0.0 to 1.0)
        """
        if len(positions) != 2 or len(speeds) != 2:
            raise InvalidArgumentError("positions and speeds must have 2 elements")

        pos_array = (c_float * 2)(*positions)
        vel_array = (c_float * 2)(*speeds)

        result = _lib.prohand_send_linear_commands(self._handle, pos_array, vel_array)
        _check_result(result, "send_linear_commands")

    def send_linear_streams(self, positions: List[float], speeds: List[float]):
        """
        Send linear motor commands via PUB/SUB streaming channel

        Uses the streaming socket for high-frequency control.
        Requires: Client created with streaming endpoint AND driver in streaming mode.

        Args:
            positions: List of 2 position values in radians
            speeds: List of 2 speed values (normalized 0.0 to 1.0)

        Raises:
            ConnectionError: If streaming endpoint was not provided or driver not in streaming mode
        """
        if len(positions) != 2 or len(speeds) != 2:
            raise InvalidArgumentError("positions and speeds must have 2 elements")

        pos_array = (c_float * 2)(*positions)
        vel_array = (c_float * 2)(*speeds)

        result = _lib.prohand_send_linear_streams(self._handle, pos_array, vel_array)
        _check_result(result, "send_linear_streams")

    def send_wrist_command(self, positions: List[float], use_profiler: bool = False):
        """
        Send wrist joint command via REQ/REP command channel (high-level wrist joints)

        Uses the command socket. For high-frequency commands, use
        send_wrist_streams() instead.

        Args:
            positions: List of 2 wrist joint angles in radians
            use_profiler: Whether to enable wrist motion profiling (position-only, implicit max velocity)
        """
        if len(positions) != 2:
            raise InvalidArgumentError("positions must have 2 elements")
        pos_array = (c_float * 2)(*positions)
        if use_profiler:
            result = _lib.prohand_send_wrist_command(self._handle, pos_array, True)
        else:
            result = _lib.prohand_send_wrist_command(self._handle, pos_array, False)
        _check_result(result, "send_wrist_command")

    def send_wrist_streams(self, positions: List[float], use_profiler: bool = False):
        """
        Send wrist joint command via PUB/SUB streaming channel (high-level wrist joints)

        Uses the streaming socket for high-frequency control.
        Requires: Client created with streaming endpoint AND driver in streaming mode.

        Args:
            positions: List of 2 wrist joint angles in radians
            use_profiler: Whether to enable wrist motion profiling (position-only, implicit max velocity)
        """
        if len(positions) != 2:
            raise InvalidArgumentError("positions must have 2 elements")
        pos_array = (c_float * 2)(*positions)
        if use_profiler:
            result = _lib.prohand_send_wrist_streams(self._handle, pos_array, True)
        else:
            result = _lib.prohand_send_wrist_streams(self._handle, pos_array, False)
        _check_result(result, "send_wrist_streams")

    def set_wrist_limits(self, max_velocity: List[float], max_acceleration: List[float], max_jerk: List[float]):
        """
        Configure wrist motion limits (rad units). Effective only if the SDK was built with the motion profiler feature.
        """
        if len(max_velocity) != 2 or len(max_acceleration) != 2 or len(max_jerk) != 2:
            raise InvalidArgumentError("wrist limits must have 2 elements each")
        vel = (c_float * 2)(*max_velocity)
        acc = (c_float * 2)(*max_acceleration)
        jerk = (c_float * 2)(*max_jerk)
        result = _lib.prohand_set_wrist_limits(self._handle, vel, acc, jerk)
        _check_result(result, "set_wrist_limits")

    def send_hand_command(self, positions: List[float], torque: float = 0.45):
        """
        Send hand command via REQ/REP command channel (high-level joint angles, uses inverse kinematics)

        Uses the command socket. For high-frequency commands, use
        send_hand_streams() instead.

        This sends joint angles per finger, which the firmware processes through
        inverse kinematics to compute actuator positions. This is the high-level API.

        Args:
            positions: List of 20 floats (5 fingers × 4 joints) in radians
                      Order: thumb[0-3], index[4-7], middle[8-11], ring[12-15], pinky[16-19]
            torque: Single torque value (normalized 0.0 to 1.0) applied to all joints

        Example:
            # All fingers at zero
            positions = [0.0] * 20
            client.send_hand_command(positions, 0.45)

            # Index finger metacarpal at 30 degrees
            positions = [0.0] * 20
            positions[4] = math.radians(30.0)  # index metacarpal
            client.send_hand_command(positions, 0.45)
        """
        if len(positions) != 20:
            raise InvalidArgumentError("positions must have 20 elements (5 fingers × 4 joints)")

        pos_array = (c_float * 20)(*positions)
        result = _lib.prohand_send_hand_command(self._handle, pos_array, c_float(torque))
        _check_result(result, "send_hand_command")

    def send_hand_streams(self, positions: List[float], torque: float = 0.45):
        """
        Send hand command via PUB/SUB streaming channel (high-level joint angles, uses inverse kinematics)

        Uses the streaming socket for high-frequency control.
        Requires: Client created with streaming endpoint AND driver in streaming mode.

        This sends joint angles per finger, which the firmware processes through
        inverse kinematics to compute actuator positions. This is the high-level API.

        Args:
            positions: List of 20 floats (5 fingers × 4 joints) in radians
                      Order: thumb[0-3], index[4-7], middle[8-11], ring[12-15], pinky[16-19]
            torque: Single torque value (normalized 0.0 to 1.0) applied to all joints

        Raises:
            ConnectionError: If streaming endpoint was not provided or driver not in streaming mode

        Example:
            # Setup
            client.set_streaming_mode(True)
            client.wait_for_streaming_ready()

            # High-frequency loop
            for _ in range(100):
                positions = [0.0] * 20
                positions[4] = math.radians(30.0)  # index metacarpal
                client.send_hand_streams(positions, 0.45)
        """
        if len(positions) != 20:
            raise InvalidArgumentError("positions must have 20 elements (5 fingers × 4 joints)")

        pos_array = (c_float * 20)(*positions)
        result = _lib.prohand_send_hand_streams(self._handle, pos_array, c_float(torque))
        _check_result(result, "send_hand_streams")

    def send_zero_calibration(self, mask: List[bool]):
        """
        Perform zero calibration on selected joints
        
        Args:
            mask: List of 16 boolean values indicating which joints to calibrate
        """
        if len(mask) != 16:
            raise InvalidArgumentError("mask must have 16 elements")

        mask_array = (c_int * 16)(*[int(b) for b in mask])
        result = _lib.prohand_send_zero_calibration(self._handle, mask_array)
        _check_result(result, "send_zero_calibration")

    # ========================================================================
    # STATUS POLLING
    # ========================================================================

    def try_recv_status(self) -> Optional[HandStatus]:
        """
        Try to receive status (non-blocking)
        
        Returns:
            HandStatus if available, None otherwise
        """
        status_info = ProHandStatusInfo()
        result = _lib.prohand_try_recv_status(self._handle, pointer(status_info))

        if result > 0:
            return HandStatus(
                is_valid=bool(status_info.is_valid),
                status_type=int(status_info.status_type),
                rotary_positions=list(status_info.rotary_positions),
                linear_positions=list(status_info.linear_positions)
            )
        elif result == 0:
            return None
        else:
            # _check_result raises an exception on error, so this line is unreachable
            _check_result(result, "try_recv_status")
            return None  # Satisfy linter (never reached)


# ============================================================================
# MODULE-LEVEL FUNCTIONS
# ============================================================================

def discover_usb_devices(max_devices: int = 10) -> List[UsbDevice]:
    """
    Discover connected ProHand USB devices
    
    Args:
        max_devices: Maximum number of devices to return
    
    Returns:
        List of UsbDevice objects
    """
    devices_array = (ProHandUsbDeviceInfo * max_devices)()
    count = _lib.prohand_discover_usb_devices(devices_array, max_devices)
    
    if count < 0:
        _check_result(count, "discover_usb_devices")
    
    result = []
    for i in range(count):
        dev = devices_array[i]
        result.append(UsbDevice(
            port_name=dev.port_name.decode('utf-8') if dev.port_name else "",
            display_name=dev.display_name.decode('utf-8') if dev.display_name else ""
        ))
        
        # Free the strings allocated by C
        if dev.port_name:
            _lib.prohand_free_string(dev.port_name)
        if dev.display_name:
            _lib.prohand_free_string(dev.display_name)
    
    return result


def get_version() -> str:
    """Get SDK version"""
    version_ptr = _lib.prohand_get_version()
    return version_ptr.decode('utf-8') if version_ptr else "unknown"


# ============================================================================
# EXAMPLE USAGE
# ============================================================================

def _example():
    """Example usage of the SDK"""
    print(f"ProHand SDK Version: {get_version()}")
    
    # Discover USB devices
    devices = discover_usb_devices()
    print(f"\nFound {len(devices)} USB device(s):")
    for dev in devices:
        print(f"  - Display: {dev.display_name}, Port: {dev.port_name}")
    
    # Create client
    with ProHandClient(
        "tcp://127.0.0.1:5562",  # Command endpoint
        "tcp://127.0.0.1:5561",  # Status endpoint
        "tcp://127.0.0.1:5563",  # Hand streaming endpoint
        "tcp://127.0.0.1:5564"   # Wrist streaming endpoint
    ) as client:
        print(f"\nConnected: {client.is_connected()}")
        
        # Send ping
        client.send_ping()
        print("Sent ping")
        
        # Open hand (all fingers extended)
        positions = [0.0] * 20  # 20 joints: 5 fingers × 4 joints each
        client.send_hand_command(positions, torque=0.45)
        print("Opening hand...")
        
        # Poll status
        status = client.try_recv_status()
        if status and status.is_valid:
            print(f"\nStatus:")
            print(f"  Valid: {status.is_valid}")
            print(f"  Type: {status.status_type}")
            if status.status_type == 1:
                print(f"  Rotary positions: {status.rotary_positions[:4]}...")
            elif status.status_type == 2:
                print(f"  Linear positions: {status.linear_positions}...")


if __name__ == '__main__':
    _example()

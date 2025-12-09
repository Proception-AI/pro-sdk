/**
 * ProHand Client SDK - C API
 * 
 * This header provides a minimal C interface to the ProHand Client SDK.
 * 
 * Usage:
 * 1. Create a client with prohand_client_create()
 * 2. Send commands using prohand_send_* functions
 * 3. Poll status with prohand_try_recv_status()
 * 4. Clean up with prohand_client_destroy()
 * 
 * Linking:
 * - Link against libprohand_client_sdk.so (Linux), libprohand_client_sdk.dylib (macOS),
 *   or prohand_client_sdk.dll (Windows)
 * - The library is typically located in ../lib/ relative to the header directory
 * 
 * Thread Safety: The client handle is NOT thread-safe. Use external synchronization
 * if accessing from multiple threads.
 * 
 * Version: 0.1.0
 */

#ifndef PROHAND_SDK_H
#define PROHAND_SDK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ========================================================================== */
/* TYPES AND STRUCTURES                                                        */
/* ========================================================================== */

/**
 * Opaque handle to a ProHand client instance
 */
typedef struct ProHandClientHandle ProHandClientHandle;

/**
 * Result codes returned by SDK functions
 */
typedef enum {
    PROHAND_SUCCESS = 0,
    PROHAND_ERROR_NULL = -1,
    PROHAND_ERROR_CONNECTION = -2,
    PROHAND_ERROR_INVALID_ARGUMENT = -3,
    PROHAND_ERROR_NOT_CONNECTED = -4,
    PROHAND_ERROR_UNSUPPORTED = -5,
    PROHAND_ERROR_OTHER = -99
} ProHandResult;

/**
 * USB device information
 */
typedef struct {
    const char* port_name;      /* Path to device - must be freed with prohand_free_string() */
    const char* display_name;    /* Display name with serial info - must be freed with prohand_free_string() */
} ProHandUsbDeviceInfo;

/**
 * Hand status information
 */
typedef struct {
    int is_valid;        /* Whether status data is valid */
    int status_type;     /* Type of status (0=unknown, 1=rotary, 2=linear) */
    float rotary_positions[16];  /* Rotary positions in radians */
    float linear_positions[2];   /* Linear positions in radians */
} ProHandStatusInfo;

/* ========================================================================== */
/* CLIENT LIFECYCLE                                                            */
/* ========================================================================== */

/**
 * Create a new ProHand IPC client with all endpoints
 *
 * @param command_endpoint ZeroMQ endpoint for commands (e.g., "tcp://127.0.0.1:5562")
 * @param status_endpoint ZeroMQ endpoint for status (e.g., "tcp://127.0.0.1:5561")
 * @param hand_streaming_endpoint ZeroMQ endpoint for hand streaming (e.g., "tcp://127.0.0.1:5563")
 * @param wrist_streaming_endpoint ZeroMQ endpoint for wrist streaming (e.g., "tcp://127.0.0.1:5564")
 * @return Pointer to client handle on success, NULL on failure
 *
 * The hand and wrist streaming endpoints use separate PUB/SUB channels for
 * high-frequency control. Hand streaming is for finger commands, wrist streaming
 * is for wrist commands.
 *
 * Example:
 *   ProHandClientHandle* client = prohand_client_create(
 *       "tcp://127.0.0.1:5562",   // Command endpoint
 *       "tcp://127.0.0.1:5561",   // Status endpoint
 *       "tcp://127.0.0.1:5563",   // Hand streaming endpoint
 *       "tcp://127.0.0.1:5564"    // Wrist streaming endpoint
 *   );
 */
ProHandClientHandle* prohand_client_create(
    const char* command_endpoint,
    const char* status_endpoint,
    const char* hand_streaming_endpoint,
    const char* wrist_streaming_endpoint
);

/**
 * Destroy a ProHand client handle and free resources
 * 
 * @param handle Client handle to destroy
 */
void prohand_client_destroy(ProHandClientHandle* handle);

/**
 * Check if client is connected to the device
 * 
 * @param handle Client handle
 * @return 1 if connected, 0 if not connected
 */
int prohand_client_is_connected(const ProHandClientHandle* handle);

/* ========================================================================== */
/* COMMAND FUNCTIONS                                                           */
/* ========================================================================== */

/**
 * Send a ping command to the device
 * 
 * @param handle Client handle
 * @return PROHAND_SUCCESS on success, error code on failure
 */
ProHandResult prohand_send_ping(const ProHandClientHandle* handle);

/**
 * Enable or disable streaming mode
 * 
 * In streaming mode, commands are sent at high frequency with lower latency.
 * 
 * @param handle Client handle
 * @param enabled Non-zero to enable, 0 to disable
 * @return PROHAND_SUCCESS on success, error code on failure
 */
ProHandResult prohand_set_streaming_mode(
    const ProHandClientHandle* handle,
    int enabled
);

/**
 * Send rotary motor commands via REQ/REP command channel (16 motors)
 *
 * Uses the command socket. For high-frequency commands, use
 * prohand_send_rotary_streams() instead.
 *
 * @param handle Client handle
 * @param positions Array of 16 position values in radians
 * @param torques Array of 16 torque values (normalized 0.0 to 1.0)
 * @return PROHAND_SUCCESS on success, error code on failure
 *
 * Example:
 *   float positions[16] = {0.0f, 0.5f, ...};
 *   float torques[16] = {0.45f, 0.45f, ...};
 *   prohand_send_rotary_commands(client, positions, torques);
 */
ProHandResult prohand_send_rotary_commands(
    const ProHandClientHandle* handle,
    const float* positions,
    const float* torques
);

/**
 * Send rotary motor commands via PUB/SUB streaming channel (16 motors)
 *
 * Uses the streaming socket for high-frequency commands.
 * Requires: Client created with streaming endpoint AND driver in streaming mode
 *           (call prohand_set_streaming_mode(handle, 1) first).
 *
 * @param handle Client handle
 * @param positions Array of 16 position values in radians
 * @param torques Array of 16 torque values (normalized 0.0 to 1.0)
 * @return PROHAND_SUCCESS on success,
 *         PROHAND_ERROR_NOT_CONNECTED if streaming not available
 *
 * Example:
 *   // Setup
 *   client = prohand_client_create(
 *       "tcp://127.0.0.1:5562",   // Command endpoint
 *       "tcp://127.0.0.1:5561",   // Status endpoint
 *       "tcp://127.0.0.1:5563",   // Hand streaming endpoint
 *       "tcp://127.0.0.1:5564"    // Wrist streaming endpoint
 *   );
 *   prohand_set_streaming_mode(client, 1);  // Enable on driver
 *
 *   // High-frequency loop
 *   for (...) {
 *       prohand_send_rotary_streams(client, positions, torques);
 *   }
 */
ProHandResult prohand_send_rotary_streams(
    const ProHandClientHandle *handle,
    const float *positions,
    const float *torques);

/**
 * Send linear motor commands via REQ/REP command channel (2 motors)
 *
 * Uses the command socket. For high-frequency commands, use
 * prohand_send_linear_streams() instead.
 *
 * @param handle Client handle
 * @param positions Array of 2 position values in radians
 * @param speeds Array of 2 speed values (normalized 0.0 to 1.0)
 * @return PROHAND_SUCCESS on success, error code on failure
 *
 * Example:
 *   float positions[2] = {0.0f, 0.0f};
 *   float speeds[2] = {1.0f, 1.0f};
 *   prohand_send_linear_commands(client, positions, speeds);
 */
ProHandResult prohand_send_linear_commands(
    const ProHandClientHandle* handle,
    const float* positions,
    const float* speeds
);

/**
 * Send linear motor commands via PUB/SUB streaming channel (2 motors)
 *
 * Uses the streaming socket for high-frequency commands.
 * Requires: Client created with streaming endpoint AND driver in streaming mode.
 *
 * @param handle Client handle
 * @param positions Array of 2 position values in radians
 * @param speeds Array of 2 speed values (normalized 0.0 to 1.0)
 * @return PROHAND_SUCCESS on success,
 *         PROHAND_ERROR_NOT_CONNECTED if streaming not available
 */
ProHandResult prohand_send_linear_streams(
    const ProHandClientHandle *handle,
    const float *positions,
    const float *speeds);

/**
 * Send wrist joint command via REQ/REP command channel (high-level wrist joints)
 *
 * Uses the command socket. For high-frequency commands, use
 * prohand_send_wrist_streams() instead.
 *
 * @param handle Client handle
 * @param positions Array of 2 wrist joint angles in radians
 * @param use_profiler Whether to use the wrist motion profiler (position-only profiling, commands max velocity; velocities are implicit [1.0, 1.0])
 * @return PROHAND_SUCCESS on success, error code on failure
 */
ProHandResult prohand_send_wrist_command(
    const ProHandClientHandle* handle,
    const float* positions,
    bool use_profiler
);

/**
 * Send wrist joint command via PUB/SUB streaming channel (high-level wrist joints)
 *
 * Uses the streaming socket for high-frequency commands.
 * Requires: Client created with streaming endpoint AND driver in streaming mode.
 *
 * @param handle Client handle
 * @param positions Array of 2 wrist joint angles in radians
 * @param use_profiler Whether to use the wrist motion profiler (position-only profiling, commands max velocity; velocities are implicit [1.0, 1.0])
 * @return PROHAND_SUCCESS on success,
 *         PROHAND_ERROR_NOT_CONNECTED if streaming not available
 */
ProHandResult prohand_send_wrist_streams(
    const ProHandClientHandle* handle,
    const float* positions,
    bool use_profiler
);

/**
 * Configure wrist motion limits (only applies if motion profiler is enabled).
 *
 * @param handle Client handle
 * @param max_velocity Array of 2 values (rad/s)
 * @param max_acceleration Array of 2 values (rad/s^2)
 * @param max_jerk Array of 2 values (rad/s^3)
 * @return PROHAND_SUCCESS on success, PROHAND_ERROR_UNSUPPORTED if profiler disabled in the build
 */
ProHandResult prohand_set_wrist_limits(
    const ProHandClientHandle* handle,
    const float max_velocity[2],
    const float max_acceleration[2],
    const float max_jerk[2]
);

/**
 * Send hand command via REQ/REP command channel (high-level joint angles, uses inverse kinematics)
 *
 * Uses the command socket. For high-frequency commands, use
 * prohand_send_hand_streams() instead.
 *
 * This sends joint angles per finger, which the firmware processes through
 * inverse kinematics to compute actuator positions. This is the high-level API.
 *
 * @param handle Client handle
 * @param positions Array of 20 floats (5 fingers × 4 joints) in radians
 *                  Order: thumb[0-3], index[4-7], middle[8-11], ring[12-15], pinky[16-19]
 * @param torque Single torque value (normalized 0.0 to 1.0) applied to all joints
 * @return PROHAND_SUCCESS on success, error code on failure
 *
 * Example:
 *   float positions[20] = {0.0, 0.0, 0.0, 0.0,  // thumb
 *                          0.0, 0.0, 0.0, 0.0,  // index
 *                          0.0, 0.0, 0.0, 0.0,  // middle
 *                          0.0, 0.0, 0.0, 0.0,  // ring
 *                          0.0, 0.0, 0.0, 0.0}; // pinky
 *   prohand_send_hand_command(client, positions, 0.45);
 */
ProHandResult prohand_send_hand_command(
    const ProHandClientHandle* handle,
    const float* positions,
    float torque
);

/**
 * Send hand command via PUB/SUB streaming channel (high-level joint angles, uses inverse kinematics)
 *
 * Uses the streaming socket for high-frequency commands.
 * Requires: Client created with streaming endpoint AND driver in streaming mode.
 *
 * This sends joint angles per finger, which the firmware processes through
 * inverse kinematics to compute actuator positions. This is the high-level API.
 *
 * @param handle Client handle
 * @param positions Array of 20 floats (5 fingers × 4 joints) in radians
 *                  Order: thumb[0-3], index[4-7], middle[8-11], ring[12-15], pinky[16-19]
 * @param torque Single torque value (normalized 0.0 to 1.0) applied to all joints
 * @return PROHAND_SUCCESS on success,
 *         PROHAND_ERROR_NOT_CONNECTED if streaming not available
 *
 * Example:
 *   // Setup
 *   client = prohand_client_create(
 *       "tcp://127.0.0.1:5562",   // Command endpoint
 *       "tcp://127.0.0.1:5561",   // Status endpoint
 *       "tcp://127.0.0.1:5563",   // Hand streaming endpoint
 *       "tcp://127.0.0.1:5564"    // Wrist streaming endpoint
 *   );
 *   prohand_set_streaming_mode(client, 1);  // Enable on driver
 *
 *   // High-frequency loop
 *   for (...) {
 *       float positions[20] = {...};
 *       prohand_send_hand_streams(client, positions, 0.45);
 *   }
 */
ProHandResult prohand_send_hand_streams(
    const ProHandClientHandle* handle,
    const float* positions,
    float torque
);

/**
 * Perform zero calibration on selected joints
 * 
 * This sets the current position of selected joints as the zero position.
 * 
 * @param handle Client handle
 * @param mask Array of 16 boolean values (0 or 1) indicating which joints to calibrate
 * @return PROHAND_SUCCESS on success, error code on failure
 * 
 * Example:
 *   int mask[16] = {1, 1, 1, 1, 0, 0, ...}; // Calibrate first 4 joints
 *   prohand_send_zero_calibration(client, mask);
 */
ProHandResult prohand_send_zero_calibration(
    const ProHandClientHandle* handle,
    const int* mask
);

/* ========================================================================== */
/* USB DISCOVERY                                                               */
/* ========================================================================== */

/**
 * Discover connected ProHand USB devices
 * 
 * @param out_devices Output array to store device info
 * @param max_devices Maximum number of devices to return
 * @return Number of devices found, or negative error code
 * 
 * Note: Call prohand_free_string() on serial_number and port_name for each device.
 * 
 * Example:
 *   ProHandUsbDeviceInfo devices[10];
 *   int count = prohand_discover_usb_devices(devices, 10);
 *   for (int i = 0; i < count; i++) {
 *       printf("Device: %s\n", devices[i].display_name);
 *       prohand_free_string((char*)devices[i].port_name);
 *       prohand_free_string((char*)devices[i].display_name);
 *   }
 */
int prohand_discover_usb_devices(
    ProHandUsbDeviceInfo* out_devices,
    int max_devices
);

/**
 * Free a string allocated by the library
 * 
 * @param s String pointer to free
 */
void prohand_free_string(char* s);

/* ========================================================================== */
/* STATUS POLLING                                                              */
/* ========================================================================== */

/**
 * Try to receive status (non-blocking)
 * 
 * @param handle Client handle
 * @param out_status Output structure to fill with status data
 * @return 1 if status was received, 0 if no status available, negative on error
 * 
 * Example:
 *   ProHandStatusInfo status;
 *   if (prohand_try_recv_status(client, &status) > 0) {
 *       if (status.is_valid) {
 *           if (status.status_type == 1) {
 *               printf("Rotary position 0: %f rad\n", status.rotary_positions[0]);
 *           } else if (status.status_type == 2) {
 *               printf("Linear position 0: %f rad\n", status.linear_positions[0]);
 *           }
 *       }
 *   }
 */
int prohand_try_recv_status(
    const ProHandClientHandle* handle,
    ProHandStatusInfo* out_status
);

/**
 * Check if the driver is in Running state (streaming active)
 *
 * Polls the status channel and checks if RotaryState or LinearState
 * is in Running mode, which indicates streaming is active.
 *
 * @param handle Client handle
 * @return 1 if in running state, 0 if not, negative on error
 *
 * Example:
 *   if (prohand_is_running_state(client)) {
 *       // Driver confirmed in Running state
 *       prohand_send_rotary_streams(client, positions, torques);
 *   }
 */
int prohand_is_running_state(
    const ProHandClientHandle *handle);

/* ========================================================================== */
/* VERSION INFO                                                                */
/* ========================================================================== */

/**
 * Get library version string
 * 
 * @return Version string (do not free)
 */
const char* prohand_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* PROHAND_SDK_H */

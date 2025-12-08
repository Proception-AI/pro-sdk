/**
 * ProGlove Client SDK - C API
 * 
 * This header provides a minimal C interface to the ProGlove Client SDK.
 * 
 * Usage:
 * 1. Create a client with proglove_client_create()
 * 2. Poll status with proglove_try_recv_status()
 * 3. Clean up with proglove_client_destroy()
 * 
 * Linking:
 * - Link against libproglove_client_sdk.so (Linux), libproglove_client_sdk.dylib (macOS),
 *   or proglove_client_sdk.dll (Windows)
 * - The library is typically located in ../lib/ relative to the header directory
 * 
 * Thread Safety: The client handle is NOT thread-safe. Use external synchronization
 * if accessing from multiple threads.
 * 
 * Version: 0.1.0
 */

#ifndef PROGLOVE_SDK_H
#define PROGLOVE_SDK_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* TYPES AND STRUCTURES                                                        */
/* ========================================================================== */

/**
 * Opaque handle to a ProGlove client instance
 */
typedef struct ProGloveClientHandle ProGloveClientHandle;

/**
 * Result codes returned by SDK functions
 */
typedef enum {
    PROGLOVE_SUCCESS = 0,
    PROGLOVE_ERROR_NULL = -1,
    PROGLOVE_ERROR_CONNECTION = -2,
    PROGLOVE_ERROR_INVALID_ARGUMENT = -3,
    PROGLOVE_ERROR_NOT_CONNECTED = -4,
    PROGLOVE_ERROR_UNSUPPORTED = -5,
    PROGLOVE_ERROR_OTHER = -99
} ProGloveResult;

/**
 * USB device information
 */
typedef struct {
    const char* port_name;      /* Path to device - must be freed with proglove_free_string() */
    const char* display_name;   /* Display name with serial info - must be freed with proglove_free_string() */
} ProGloveUsbDeviceInfo;

/* Taxel array sizes per segment (from taxel_mapping_v0.yaml) */
/* Thumb segments (larger than other fingers) */
#define PROGLOVE_TAXELS_T_DIP 6
#define PROGLOVE_TAXELS_T_MCP 10
#define PROGLOVE_TAXELS_T_PIP 4
/* Index finger segments */
#define PROGLOVE_TAXELS_I_DIP 4
#define PROGLOVE_TAXELS_I_MCP 2
#define PROGLOVE_TAXELS_I_PIP 2
/* Middle finger segments */
#define PROGLOVE_TAXELS_M_DIP 4
#define PROGLOVE_TAXELS_M_MCP 2
#define PROGLOVE_TAXELS_M_PIP 2
/* Ring finger segments */
#define PROGLOVE_TAXELS_R_DIP 4
#define PROGLOVE_TAXELS_R_MCP 2
#define PROGLOVE_TAXELS_R_PIP 2
/* Pinky finger segments */
#define PROGLOVE_TAXELS_P_DIP 4
#define PROGLOVE_TAXELS_P_MCP 2
#define PROGLOVE_TAXELS_P_PIP 2
/* Palm segments */
#define PROGLOVE_TAXELS_UPPER_PALM 16
#define PROGLOVE_TAXELS_MIDDLE_PALM 16
#define PROGLOVE_TAXELS_LOWER_PALM 16

/**
 * Tactile status from glove sensors (segment-based)
 * 
 * Contains tactile pressure values organized by joint segment.
 * Each finger has DIP (distal), MCP (metacarpal), and PIP (proximal) segments.
 * Values are 0-255, where higher values indicate more pressure.
 * 
 * Segment sizes (from taxel_mapping_v0.yaml):
 * - Thumb: DIP(6) + MCP(10) + PIP(4) = 20 taxels
 * - Index/Middle/Ring/Pinky: DIP(4) + MCP(2) + PIP(2) = 8 taxels each
 * - Palm: upper(16) + middle(16) + lower(16) = 48 taxels
 * - Total: 100 taxels per hand
 */
typedef struct {
    int is_valid;                                       /* 1 if data is valid, 0 otherwise */
    unsigned int timestamp;                             /* Timestamp (milliseconds, wrapped) */
    unsigned int uid;                                   /* Unique identifier for this sample */
    /* Thumb segments (6+10+4 = 20 taxels) */
    unsigned char t_dip[PROGLOVE_TAXELS_T_DIP];         /* Thumb DIP segment (6 taxels) */
    unsigned char t_mcp[PROGLOVE_TAXELS_T_MCP];         /* Thumb MCP segment (10 taxels) */
    unsigned char t_pip[PROGLOVE_TAXELS_T_PIP];         /* Thumb PIP segment (4 taxels) */
    /* Index finger segments (4+2+2 = 8 taxels) */
    unsigned char i_dip[PROGLOVE_TAXELS_I_DIP];         /* Index DIP segment (4 taxels) */
    unsigned char i_mcp[PROGLOVE_TAXELS_I_MCP];         /* Index MCP segment (2 taxels) */
    unsigned char i_pip[PROGLOVE_TAXELS_I_PIP];         /* Index PIP segment (2 taxels) */
    /* Middle finger segments (4+2+2 = 8 taxels) */
    unsigned char m_dip[PROGLOVE_TAXELS_M_DIP];         /* Middle DIP segment (4 taxels) */
    unsigned char m_mcp[PROGLOVE_TAXELS_M_MCP];         /* Middle MCP segment (2 taxels) */
    unsigned char m_pip[PROGLOVE_TAXELS_M_PIP];         /* Middle PIP segment (2 taxels) */
    /* Ring finger segments (4+2+2 = 8 taxels) */
    unsigned char r_dip[PROGLOVE_TAXELS_R_DIP];         /* Ring DIP segment (4 taxels) */
    unsigned char r_mcp[PROGLOVE_TAXELS_R_MCP];         /* Ring MCP segment (2 taxels) */
    unsigned char r_pip[PROGLOVE_TAXELS_R_PIP];         /* Ring PIP segment (2 taxels) */
    /* Pinky finger segments (4+2+2 = 8 taxels) */
    unsigned char p_dip[PROGLOVE_TAXELS_P_DIP];         /* Pinky DIP segment (4 taxels) */
    unsigned char p_mcp[PROGLOVE_TAXELS_P_MCP];         /* Pinky MCP segment (2 taxels) */
    unsigned char p_pip[PROGLOVE_TAXELS_P_PIP];         /* Pinky PIP segment (2 taxels) */
    /* Palm segments (16+16+16 = 48 taxels) */
    unsigned char upper_palm[PROGLOVE_TAXELS_UPPER_PALM];   /* Upper palm (16 taxels) */
    unsigned char middle_palm[PROGLOVE_TAXELS_MIDDLE_PALM]; /* Middle palm (16 taxels) */
    unsigned char lower_palm[PROGLOVE_TAXELS_LOWER_PALM];   /* Lower palm (16 taxels) */
} ProGloveTactileStatus;

/* ========================================================================== */
/* CLIENT LIFECYCLE                                                            */
/* ========================================================================== */

/**
 * Create a new ProGlove IPC client
 * 
 * @param status_endpoint ZeroMQ endpoint for status (e.g., "ipc:///tmp/proglove-left-status.ipc")
 * @return Pointer to client handle on success, NULL on failure
 * 
 * Example:
 *   ProGloveClientHandle* client = proglove_client_create(
 *       "ipc:///tmp/proglove-left-status.ipc"
 *   );
 */
ProGloveClientHandle* proglove_client_create(const char* status_endpoint);

/**
 * Destroy a ProGlove client handle and free resources
 * 
 * @param handle Client handle to destroy
 */
void proglove_client_destroy(ProGloveClientHandle* handle);

/**
 * Check if client is connected to the device
 * 
 * @param handle Client handle
 * @return 1 if connected, 0 if not connected
 */
int proglove_client_is_connected(const ProGloveClientHandle* handle);

/* ========================================================================== */
/* COMMAND FUNCTIONS                                                           */
/* ========================================================================== */

/**
 * Send a ping command to verify connection
 * 
 * Since ProGlove uses PUB/SUB (not REQ/REP like ProHand), this method
 * waits for tactile data to be received, confirming the connection is working.
 * 
 * @param handle Client handle
 * @return PROGLOVE_SUCCESS on success, PROGLOVE_ERROR_OTHER on failure
 */
ProGloveResult proglove_send_ping(const ProGloveClientHandle* handle);

/* ========================================================================== */
/* USB DISCOVERY                                                               */
/* ========================================================================== */

/**
 * Discover connected ProGlove USB devices
 * 
 * Enumerates USB devices that match ProGlove identification patterns.
 * Looks for devices with serial numbers starting with "PRO-G" or "02D".
 * 
 * @param out_devices Output array to store device info
 * @param max_devices Maximum number of devices to return
 * @return Number of devices found, or negative error code
 * 
 * Note: Call proglove_free_string() on port_name and display_name for each device.
 * 
 * Example:
 *   ProGloveUsbDeviceInfo devices[10];
 *   int count = proglove_discover_usb_devices(devices, 10);
 *   for (int i = 0; i < count; i++) {
 *       printf("Device: %s\n", devices[i].display_name);
 *       proglove_free_string((char*)devices[i].port_name);
 *       proglove_free_string((char*)devices[i].display_name);
 *   }
 */
int proglove_discover_usb_devices(
    ProGloveUsbDeviceInfo* out_devices,
    int max_devices
);

/**
 * Free a string allocated by the library
 * 
 * @param s String pointer to free
 */
void proglove_free_string(char* s);

/* ========================================================================== */
/* STATUS POLLING                                                              */
/* ========================================================================== */

/**
 * Try to receive tactile status (non-blocking)
 * 
 * @param handle Client handle
 * @param out_status Output structure to fill with tactile data
 * @return 1 if status was received, 0 if no status available, negative on error
 * 
 * Example:
 *   ProGloveTactileStatus status;
 *   if (proglove_try_recv_status(client, &status) > 0) {
 *       if (status.is_valid) {
 *           printf("Thumb DIP[0]: %d\n", status.t_dip[0]);
 *       }
 *   }
 */
int proglove_try_recv_status(
    const ProGloveClientHandle* handle,
    ProGloveTactileStatus* out_status
);

/* ========================================================================== */
/* VERSION INFO                                                                */
/* ========================================================================== */

/**
 * Get library version string
 * 
 * @return Version string (do not free)
 */
const char* proglove_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* PROGLOVE_SDK_H */

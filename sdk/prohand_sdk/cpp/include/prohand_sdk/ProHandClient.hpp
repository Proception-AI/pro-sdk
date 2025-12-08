/**
 * ProHand Client C++ Wrapper
 *
 * RAII wrapper around the C API from prohand_sdk.h
 */

#pragma once

#include <prohand_sdk/prohand_sdk.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <optional>
#include <chrono>
#include <thread>

namespace prohand_sdk {

    /**
     * Exception thrown by SDK operations
     */
    class SdkException : public std::runtime_error
    {
    public:
        explicit SdkException(const std::string &message)
            : std::runtime_error(message) {}
    };

/**
 * USB Device information
 */
struct UsbDevice {
    std::string portName;
    std::string displayName;
};

/**
 * Hand status information
 */
struct HandStatus
{
    bool isValid;
    int statusType;  // 0=unknown, 1=rotary, 2=linear
    std::vector<float> rotaryPositions; // Radians [16]
    std::vector<float> linearPositions; // Radians [2]
};

/**
 * ProHand Client - RAII wrapper for ProHandClientHandle
 */
class ProHandClient {
private:
    ProHandClientHandle *handle_;
    std::string commandEndpoint_;
    std::string statusEndpoint_;

public:
    /**
     * Create a new ProHand client
     *
     * @param commandEndpoint ZMQ command endpoint (e.g., "tcp://127.0.0.1:5562")
     * @param statusEndpoint ZMQ status endpoint (e.g., "tcp://127.0.0.1:5561")
     * @param handStreamingEndpoint ZMQ hand streaming endpoint (e.g., "tcp://127.0.0.1:5563")
     * @param wristStreamingEndpoint ZMQ wrist streaming endpoint (e.g., "tcp://127.0.0.1:5564")
     * @throws SdkException if connection fails
     */
    ProHandClient(const std::string &commandEndpoint,
                  const std::string &statusEndpoint,
                  const std::string &handStreamingEndpoint,
                  const std::string &wristStreamingEndpoint)
        : handle_(nullptr), commandEndpoint_(commandEndpoint), statusEndpoint_(statusEndpoint)
    {
        handle_ = prohand_client_create(
            commandEndpoint.c_str(),
            statusEndpoint.c_str(),
            handStreamingEndpoint.c_str(),
            wristStreamingEndpoint.c_str());

        if (!handle_)
        {
            throw SdkException("Failed to create ProHand client");
        }
    }

    /**
     * Configure wrist motion limits (only effective if motion profiler is enabled in the SDK build)
     *
     * @param max_velocity 2 values (rad/s)
     * @param max_acceleration 2 values (rad/s^2)
     * @param max_jerk 2 values (rad/s^3)
     * @throws SdkException on error
     */
    void setWristLimits(const std::vector<float> &max_velocity,
                        const std::vector<float> &max_acceleration,
                        const std::vector<float> &max_jerk)
    {
        checkHandle();
        if (max_velocity.size() != 2 || max_acceleration.size() != 2 || max_jerk.size() != 2)
        {
            throw SdkException("wrist limits must have 2 elements each");
        }
        auto result = prohand_set_wrist_limits(handle_, max_velocity.data(), max_acceleration.data(), max_jerk.data());
        checkResult(result, "setWristLimits");
    }

    // Disable copy
    ProHandClient(const ProHandClient &) = delete;
    ProHandClient &operator=(const ProHandClient &) = delete;

    // Enable move
    ProHandClient(ProHandClient &&other) noexcept
        : handle_(other.handle_), commandEndpoint_(std::move(other.commandEndpoint_)), statusEndpoint_(std::move(other.statusEndpoint_))
    {
        other.handle_ = nullptr;
    }

    ProHandClient &operator=(ProHandClient &&other) noexcept
    {
        if (this != &other)
        {
            if (handle_)
            {
                prohand_client_destroy(handle_);
            }
            handle_ = other.handle_;
            commandEndpoint_ = std::move(other.commandEndpoint_);
            statusEndpoint_ = std::move(other.statusEndpoint_);
            other.handle_ = nullptr;
        }
        return *this;
    }

    /**
     * Destructor - cleanup resources
     */
    ~ProHandClient()
    {
        if (handle_)
        {
            prohand_client_destroy(handle_);
            handle_ = nullptr;
        }
    }

    /**
     * Check if connected to device
     */
    bool isConnected() const
    {
        if (!handle_)
            return false;
        return prohand_client_is_connected(handle_) != 0;
    }

    /**
     * Send a ping command
     */
    void sendPing()
    {
        checkHandle();
        auto result = prohand_send_ping(handle_);
        checkResult(result, "sendPing");
    }

    /**
     * Enable or disable streaming mode
     */
    void setStreamingMode(bool enabled)
    {
        checkHandle();
        auto result = prohand_set_streaming_mode(handle_, enabled ? 1 : 0);
        checkResult(result, "setStreamingMode");
    }

    /**
     * Check if the driver is in Running state (streaming active)
     *
     * Polls the status channel and checks if RotaryState or LinearState
     * is in Running mode, which indicates streaming is truly active.
     *
     * @return true if in running state, false otherwise
     */
    bool isRunningState()
    {
        checkHandle();
        return prohand_is_running_state(handle_) == 1;
    }

    /**
     * Wait for streaming connection to be established with state verification
     *
     * This method repeatedly sends setStreamingMode(true) and polls for
     * Running state until confirmed or timeout.
     *
     * @param timeout Maximum time to wait in seconds (default: 1.0)
     * @param retryInterval How often to retry setStreamingMode in seconds (default: 0.3)
     * @return true if ready and in Running state, false if timeout
     *
     * Example:
     *   client.setStreamingMode(true);
     *   if (client.waitForStreamingReady()) {
     *       // Driver is confirmed in Running state
     *       client.sendRotaryStreams(positions, torques);
     *   }
     */
    bool waitForStreamingReady(double timeout = 1.0, double retryInterval = 0.3)
    {
        checkHandle();

        // First, verify command channel is working
        try
        {
            sendPing();
        }
        catch (...)
        {
            return false;
        }

        auto start = std::chrono::steady_clock::now();
        auto lastRetry = start;
        const double pollInterval = 0.05; // Poll every 50ms

        // Initial delay for ZMQ PUB/SUB connection to establish
        std::this_thread::sleep_for(std::chrono::duration<double>(0.2));

        // Keep retrying setStreamingMode until Running state is detected
        while (true)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration<double>(now - start).count();

            if (elapsed >= timeout)
            {
                break; // Timeout
            }

            // Check if driver reports Running state
            if (isRunningState())
            {
                return true;
            }

            // Retry setStreamingMode if enough time has passed
            auto elapsedSinceRetry = std::chrono::duration<double>(now - lastRetry).count();
            if (elapsedSinceRetry >= retryInterval)
            {
                try
                {
                    setStreamingMode(true);
                    lastRetry = now;
                }
                catch (...)
                {
                    // Ignore errors, keep trying
                }
            }

            // Wait before next poll
            double remaining = timeout - elapsed;
            if (remaining > 0)
            {
                std::this_thread::sleep_for(
                    std::chrono::duration<double>(std::min(pollInterval, remaining)));
            }
            else
            {
                break;
            }
        }

        // Timeout - check one last time
        return isRunningState();
    }

    /**
     * Send rotary commands (16 finger joints)
     *
     * @param positions 16 position values in radians
     * @param torques 16 torque values (normalized 0.0 to 1.0)
     */
    void sendRotaryCommands(const std::vector<float> &positions,
                            const std::vector<float> &torques)
    {
        checkHandle();

        if (positions.size() != 16 || torques.size() != 16)
        {
            throw SdkException("positions and torques must have 16 elements");
        }

        auto result = prohand_send_rotary_commands(
            handle_,
            positions.data(),
            torques.data());
        checkResult(result, "sendRotaryCommands");
    }

    /**
     * Send linear commands (2 wrist motors)
     *
     * @param positions 2 position values in radians
     * @param speeds 2 speed values (normalized 0.0 to 1.0)
     */
    void sendLinearCommands(const std::vector<float> &positions,
                            const std::vector<float> &speeds)
    {
        checkHandle();

        if (positions.size() != 2 || speeds.size() != 2)
        {
            throw SdkException("positions and speeds must have 2 elements");
        }

        auto result = prohand_send_linear_commands(
            handle_,
            positions.data(),
            speeds.data());
        checkResult(result, "sendLinearCommands");
    }

    /**
     * Send rotary commands via PUB/SUB streaming channel (16 finger joints)
     *
     * Uses the streaming socket for high-frequency control (100+ Hz).
     * Requires: Client created with streaming endpoint AND driver in streaming mode.
     *
     * @param positions 16 position values in radians
     * @param torques 16 torque values (normalized 0.0 to 1.0)
     * @throws SdkException if streaming not available or driver not in streaming mode
     */
    void sendRotaryStreams(const std::vector<float> &positions,
                           const std::vector<float> &torques)
    {
        checkHandle();

        if (positions.size() != 16 || torques.size() != 16)
        {
            throw SdkException("positions and torques must have 16 elements");
        }

        auto result = prohand_send_rotary_streams(
            handle_,
            positions.data(),
            torques.data());
        checkResult(result, "sendRotaryStreams");
    }

    /**
     * Send linear commands via PUB/SUB streaming channel (2 wrist motors)
     *
     * Uses the streaming socket for high-frequency control.
     * Requires: Client created with streaming endpoint AND driver in streaming mode.
     *
     * @param positions 2 position values in radians
     * @param speeds 2 speed values (normalized 0.0 to 1.0)
     * @throws SdkException if streaming not available or driver not in streaming mode
     */
    void sendLinearStreams(const std::vector<float> &positions,
                           const std::vector<float> &speeds)
    {
        checkHandle();

        if (positions.size() != 2 || speeds.size() != 2)
        {
            throw SdkException("positions and speeds must have 2 elements");
        }

        auto result = prohand_send_linear_streams(
            handle_,
            positions.data(),
            speeds.data());
        checkResult(result, "sendLinearStreams");
    }

    /**
     * Send wrist joint command via REQ/REP command channel (high-level wrist joints)
     *
     * Uses the command socket. For high-frequency commands, use
     * sendWristStreams() instead.
     *
     * @param positions 2 wrist joint angles in radians
     * @param use_profiler Whether to enable wrist motion profiling (position-only, implicit max velocity)
     * @throws SdkException on error
     */
    void sendWristCommands(const std::vector<float> &positions,
                           bool use_profiler = false)
    {
        checkHandle();
        if (positions.size() != 2)
        {
            throw SdkException("positions must have 2 elements");
        }
        auto result = prohand_send_wrist_command(
            handle_,
            positions.data(),
            use_profiler);
        checkResult(result, "sendWristCommands");
    }

    /**
     * Send wrist joint command via PUB/SUB streaming channel (high-level wrist joints)
     *
     * Uses the streaming socket for high-frequency commands.
     * Requires: Client created with streaming endpoint AND driver in streaming mode.
     *
     * @param positions 2 wrist joint angles in radians
     * @param use_profiler Whether to enable wrist motion profiling (position-only, implicit max velocity)
     * @throws SdkException if streaming not available or driver not in streaming mode
     */
    void sendWristStreams(const std::vector<float> &positions,
                          bool use_profiler = false)
    {
        checkHandle();
        if (positions.size() != 2)
        {
            throw SdkException("positions must have 2 elements");
        }
        auto result = prohand_send_wrist_streams(
            handle_,
            positions.data(),
            use_profiler);
        checkResult(result, "sendWristStreams");
    }

    /**
     * Send hand command via REQ/REP command channel (high-level joint angles, uses inverse kinematics)
     *
     * Uses the command socket. For high-frequency commands, use
     * sendHandStreams() instead.
     *
     * This sends joint angles per finger, which the firmware processes through
     * inverse kinematics to compute actuator positions. This is the high-level API.
     *
     * @param positions 20 position values in radians (5 fingers × 4 joints)
     *                  Order: thumb[0-3], index[4-7], middle[8-11], ring[12-15], pinky[16-19]
     * @param torque Single torque value (normalized 0.0 to 1.0) applied to all joints
     * @throws SdkException on error
     */
    void sendHandCommands(const std::vector<float> &positions, float torque)
    {
        checkHandle();

        if (positions.size() != 20)
        {
            throw SdkException("positions must have 20 elements (5 fingers × 4 joints)");
        }

        auto result = prohand_send_hand_command(
            handle_,
            positions.data(),
            torque);
        checkResult(result, "sendHandCommands");
    }

    /**
     * Send hand command via PUB/SUB streaming channel (high-level joint angles, uses inverse kinematics)
     *
     * Uses the streaming socket for high-frequency commands.
     * Requires: Client created with streaming endpoint AND driver in streaming mode.
     *
     * This sends joint angles per finger, which the firmware processes through
     * inverse kinematics to compute actuator positions. This is the high-level API.
     *
     * @param positions 20 position values in radians (5 fingers × 4 joints)
     *                  Order: thumb[0-3], index[4-7], middle[8-11], ring[12-15], pinky[16-19]
     * @param torque Single torque value (normalized 0.0 to 1.0) applied to all joints
     * @throws SdkException if streaming not available or driver not in streaming mode
     */
    void sendHandStreams(const std::vector<float> &positions, float torque)
    {
        checkHandle();

        if (positions.size() != 20)
        {
            throw SdkException("positions must have 20 elements (5 fingers × 4 joints)");
        }

        auto result = prohand_send_hand_streams(
            handle_,
            positions.data(),
            torque);
        checkResult(result, "sendHandStreams");
    }

    /**
     * Perform zero calibration on selected joints
     *
     * @param mask 16 boolean values indicating which joints to calibrate
     */
    void sendZeroCalibration(const std::vector<bool> &mask)
    {
        checkHandle();

        if (mask.size() != 16)
        {
            throw SdkException("mask must have 16 elements");
        }

        std::vector<int> intMask(16);
        for (size_t i = 0; i < 16; ++i)
        {
            intMask[i] = mask[i] ? 1 : 0;
        }

        auto result = prohand_send_zero_calibration(handle_, intMask.data());
        checkResult(result, "sendZeroCalibration");
    }

    /**
     * Try to receive status (non-blocking)
     *
     * @return HandStatus if available, nullopt otherwise
     */
    std::optional<HandStatus> tryRecvStatus()
    {
        checkHandle();

        ProHandStatusInfo statusInfo;
        int result = prohand_try_recv_status(handle_, &statusInfo);

        if (result > 0)
        {
            HandStatus status;
            status.isValid = statusInfo.is_valid != 0;
            status.statusType = statusInfo.status_type;
            status.rotaryPositions.assign(
                statusInfo.rotary_positions,
                statusInfo.rotary_positions + 16);
            status.linearPositions.assign(
                statusInfo.linear_positions,
                statusInfo.linear_positions + 2);
            return status;
        }
        else if (result == 0)
        {
            return std::nullopt;
        }
        else
        {
            checkResult(result, "tryRecvStatus");
            return std::nullopt; // Unreachable
        }
    }

    /**
     * Discover USB devices (static method)
     */
    static std::vector<UsbDevice> discoverUsbDevices()
    {
        ProHandUsbDeviceInfo devices[10];
        int count = prohand_discover_usb_devices(devices, 10);

        if (count < 0)
        {
            throw SdkException("USB discovery failed");
        }

        std::vector<UsbDevice> result;
        for (int i = 0; i < count; ++i)
        {
            UsbDevice dev;
            if (devices[i].port_name)
            {
                dev.portName = devices[i].port_name;
                prohand_free_string(const_cast<char *>(devices[i].port_name));
            }
            if (devices[i].display_name)
            {
                dev.displayName = devices[i].display_name;
                prohand_free_string(const_cast<char *>(devices[i].display_name));
            }
            result.push_back(std::move(dev));
        }

        return result;
    }

    /**
     * Get SDK version
     */
    static std::string getVersion()
    {
        const char *ver = prohand_get_version();
        return ver ? std::string(ver) : "unknown";
    }

private:
    void checkHandle() const
    {
        if (!handle_)
        {
            throw SdkException("Client handle is null");
        }
    }

    void checkResult(int result, const std::string &operation) const
    {
        if (result == PROHAND_SUCCESS)
        {
            return;
        }

        std::string error = operation + " failed: ";
        switch (result)
        {
        case PROHAND_ERROR_NULL:
            error += "Null pointer error";
            break;
        case PROHAND_ERROR_CONNECTION:
            error += "Connection error";
            break;
        case PROHAND_ERROR_INVALID_ARGUMENT:
            error += "Invalid argument";
            break;
        case PROHAND_ERROR_NOT_CONNECTED:
            error += "Not connected";
            break;
        default:
            error += "Unknown error (" + std::to_string(result) + ")";
            break;
        }
        throw SdkException(error);
    }
};

} // namespace prohand_sdk

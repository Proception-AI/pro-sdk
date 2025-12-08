/**
 * ProGlove Client C++ Wrapper
 *
 * RAII wrapper around the C API from proglove_sdk.h
 */

#pragma once

#include <proglove_sdk/proglove_sdk.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <optional>

namespace proglove_sdk {

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
 * Tactile status data wrapper
 *
 * Contains tactile pressure values organized by joint segment.
 * Each finger has DIP (distal), MCP (metacarpal), and PIP (proximal) segments.
 */
struct TactileStatus
{
    bool isValid;
    unsigned int timestamp;
    unsigned int uid;

    // Thumb segments (6+10+4 = 20 taxels)
    std::vector<unsigned char> t_dip;
    std::vector<unsigned char> t_mcp;
    std::vector<unsigned char> t_pip;

    // Index segments (4+2+2 = 8 taxels)
    std::vector<unsigned char> i_dip;
    std::vector<unsigned char> i_mcp;
    std::vector<unsigned char> i_pip;

    // Middle segments (4+2+2 = 8 taxels)
    std::vector<unsigned char> m_dip;
    std::vector<unsigned char> m_mcp;
    std::vector<unsigned char> m_pip;

    // Ring segments (4+2+2 = 8 taxels)
    std::vector<unsigned char> r_dip;
    std::vector<unsigned char> r_mcp;
    std::vector<unsigned char> r_pip;

    // Pinky segments (4+2+2 = 8 taxels)
    std::vector<unsigned char> p_dip;
    std::vector<unsigned char> p_mcp;
    std::vector<unsigned char> p_pip;

    // Palm segments (16+16+16 = 48 taxels)
    std::vector<unsigned char> upper_palm;
    std::vector<unsigned char> middle_palm;
    std::vector<unsigned char> lower_palm;
};

/**
 * ProGlove Client - RAII wrapper for ProGloveClientHandle
 */
class ProGloveClient {
private:
    ProGloveClientHandle *handle_;
    std::string statusEndpoint_;

public:
    /**
     * Create a new ProGlove client
     *
     * @param statusEndpoint ZMQ status endpoint (e.g., "ipc:///tmp/proglove-left-status.ipc")
     * @throws SdkException if connection fails
     */
    ProGloveClient(const std::string &statusEndpoint)
        : handle_(nullptr), statusEndpoint_(statusEndpoint)
    {
        handle_ = proglove_client_create(statusEndpoint.c_str());
        if (!handle_)
        {
            throw SdkException("Failed to create ProGlove client for endpoint: " + statusEndpoint);
        }
    }

    // Disable copy
    ProGloveClient(const ProGloveClient &) = delete;
    ProGloveClient &operator=(const ProGloveClient &) = delete;

    // Enable move
    ProGloveClient(ProGloveClient &&other) noexcept
        : handle_(other.handle_), statusEndpoint_(std::move(other.statusEndpoint_))
    {
        other.handle_ = nullptr;
    }

    ProGloveClient &operator=(ProGloveClient &&other) noexcept
    {
        if (this != &other)
        {
            if (handle_)
            {
                proglove_client_destroy(handle_);
            }
            handle_ = other.handle_;
            statusEndpoint_ = std::move(other.statusEndpoint_);
            other.handle_ = nullptr;
        }
        return *this;
    }

    /**
     * Destructor - cleanup resources
     */
    ~ProGloveClient()
    {
        if (handle_)
        {
            proglove_client_destroy(handle_);
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
        return proglove_client_is_connected(handle_) != 0;
    }

    /**
     * Send a ping command
     *
     * Since ProGlove uses PUB/SUB (not REQ/REP), this waits for
     * tactile data to confirm the connection is working.
     */
    void sendPing()
    {
        checkHandle();
        auto result = proglove_send_ping(handle_);
        // 0 means success (PROGLOVE_SUCCESS), negative means error
        if (result < 0) {
            checkResult(result, "sendPing");
        }
    }

    /**
     * Try to receive tactile status (non-blocking)
     *
     * @return TactileStatus if available, nullopt otherwise
     */
    std::optional<TactileStatus> tryRecvStatus()
    {
        checkHandle();

        ProGloveTactileStatus cStatus = {};
        int result = proglove_try_recv_status(handle_, &cStatus);

        if (result > 0)
        {
            TactileStatus status;
            status.isValid = cStatus.is_valid != 0;
            status.timestamp = cStatus.timestamp;
            status.uid = cStatus.uid;

            // Thumb segments
            status.t_dip.assign(cStatus.t_dip, cStatus.t_dip + PROGLOVE_TAXELS_T_DIP);
            status.t_mcp.assign(cStatus.t_mcp, cStatus.t_mcp + PROGLOVE_TAXELS_T_MCP);
            status.t_pip.assign(cStatus.t_pip, cStatus.t_pip + PROGLOVE_TAXELS_T_PIP);

            // Index segments
            status.i_dip.assign(cStatus.i_dip, cStatus.i_dip + PROGLOVE_TAXELS_I_DIP);
            status.i_mcp.assign(cStatus.i_mcp, cStatus.i_mcp + PROGLOVE_TAXELS_I_MCP);
            status.i_pip.assign(cStatus.i_pip, cStatus.i_pip + PROGLOVE_TAXELS_I_PIP);

            // Middle segments
            status.m_dip.assign(cStatus.m_dip, cStatus.m_dip + PROGLOVE_TAXELS_M_DIP);
            status.m_mcp.assign(cStatus.m_mcp, cStatus.m_mcp + PROGLOVE_TAXELS_M_MCP);
            status.m_pip.assign(cStatus.m_pip, cStatus.m_pip + PROGLOVE_TAXELS_M_PIP);

            // Ring segments
            status.r_dip.assign(cStatus.r_dip, cStatus.r_dip + PROGLOVE_TAXELS_R_DIP);
            status.r_mcp.assign(cStatus.r_mcp, cStatus.r_mcp + PROGLOVE_TAXELS_R_MCP);
            status.r_pip.assign(cStatus.r_pip, cStatus.r_pip + PROGLOVE_TAXELS_R_PIP);

            // Pinky segments
            status.p_dip.assign(cStatus.p_dip, cStatus.p_dip + PROGLOVE_TAXELS_P_DIP);
            status.p_mcp.assign(cStatus.p_mcp, cStatus.p_mcp + PROGLOVE_TAXELS_P_MCP);
            status.p_pip.assign(cStatus.p_pip, cStatus.p_pip + PROGLOVE_TAXELS_P_PIP);

            // Palm segments
            status.upper_palm.assign(cStatus.upper_palm, cStatus.upper_palm + PROGLOVE_TAXELS_UPPER_PALM);
            status.middle_palm.assign(cStatus.middle_palm, cStatus.middle_palm + PROGLOVE_TAXELS_MIDDLE_PALM);
            status.lower_palm.assign(cStatus.lower_palm, cStatus.lower_palm + PROGLOVE_TAXELS_LOWER_PALM);

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
        ProGloveUsbDeviceInfo devices[10];
        int count = proglove_discover_usb_devices(devices, 10);

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
                proglove_free_string(const_cast<char *>(devices[i].port_name));
            }
            if (devices[i].display_name)
            {
                dev.displayName = devices[i].display_name;
                proglove_free_string(const_cast<char *>(devices[i].display_name));
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
        const char *ver = proglove_get_version();
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
        if (result == PROGLOVE_SUCCESS)
        {
            return;
        }

        std::string error = operation + " failed: ";
        switch (result)
        {
        case PROGLOVE_ERROR_NULL:
            error += "Null pointer error";
            break;
        case PROGLOVE_ERROR_CONNECTION:
            error += "Connection error";
            break;
        case PROGLOVE_ERROR_INVALID_ARGUMENT:
            error += "Invalid argument";
            break;
        case PROGLOVE_ERROR_NOT_CONNECTED:
            error += "Not connected";
            break;
        case PROGLOVE_ERROR_UNSUPPORTED:
            error += "Unsupported operation";
            break;
        default:
            error += "Unknown error (" + std::to_string(result) + ")";
            break;
        }
        throw SdkException(error);
    }
};

} // namespace proglove_sdk

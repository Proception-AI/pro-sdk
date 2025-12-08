# Proception SDK

Official SDK, drivers, and tools for ProHand and ProGlove robotic hands.

**Current Version**: v1.0.0-rc

## Quick Start

This repository uses a flat structure where the SDK is unpacked (not zipped) for easy browsing and use.

### Directory Structure

```
pro-sdk/
├── README.md              This file
├── INDEX.md               Release information
├── MANIFEST.txt           Checksums and file details
├── sdk/                   SDK source code (unpacked)
│   ├── prohand_sdk/       ProHand SDK (C++, Python)
│   ├── proglove_sdk/      ProGlove SDK (C++, Python)
│   ├── demo/              Example applications
│   └── docs/              Documentation
├── driver/                Platform-specific driver binaries (tarballs)
│   ├── prohand-driver-v1.0.0-rc-macos-arm64.tar.gz
│   ├── prohand-driver-v1.0.0-rc-linux-arm64.tar.gz
│   └── prohand-driver-v1.0.0-rc-linux-x64.tar.gz
├── firmware/              ESP32 firmware binaries (unpacked)
│   ├── ProHand-fw-esp-L-v1.0.0-rc-*-ota.bin
│   └── ProHand-fw-esp-R-v1.0.0-rc-*-ota.bin
└── app/                   macOS diagnostic application
    └── ProHand Diagnostic v1.0.0-rc.dmg
```

## Usage

### SDK

The SDK is provided as unpacked source code. Simply clone and use:

```bash
git clone https://github.com/proception/pro-sdk.git
cd pro-sdk/sdk

# Python example
cd prohand_sdk/python
python3 example.py

# C++ example
cd prohand_sdk/cpp
# See README for build instructions
```

### Driver Binaries

Download the appropriate driver package for your platform:

```bash
# Example: macOS ARM64
tar -xzf driver/prohand-driver-v1.0.0-rc-macos-arm64.tar.gz
./prohand-headless-ipc-host --help
```

**Available platforms**:
- `macos-arm64` - macOS Apple Silicon (M1/M2/M3/M4)
- `linux-arm64` - Linux ARM64 (Jetson Orin, ARM servers)
- `linux-x64` - Linux x64 (Intel/AMD)

### Firmware

ESP32 firmware OTA binaries for ProHand (ready to flash):

```bash
# Flash left hand firmware
espflash write-bin 0x10000 firmware/ProHand-fw-esp-L-*.bin --port YOUR_PORT

# Flash right hand firmware
espflash write-bin 0x10000 firmware/ProHand-fw-esp-R-*.bin --port YOUR_PORT
```

### macOS App

Double-click the DMG file to install the ProHand Diagnostic application.

## Version Management

This repository uses git tags for versioning:

```bash
# List all versions
git tag -l

# Checkout specific version
git checkout v1.0.0-rc
```

## Documentation

- [SDK Documentation](sdk/docs/)
- [Release Index](INDEX.md)
- [Manifest](MANIFEST.txt)

## Platform Support

| Platform | SDK | Drivers | App |
|----------|-----|---------|-----|
| macOS ARM64 (M-series) | ✓ | ✓ | ✓ |
| macOS x64 (Intel) | ✓ | - | - |
| Linux ARM64 (Jetson) | ✓ | ✓ | - |
| Linux x64 | ✓ | ✓ | - |

## Support

- **Issues**: https://github.com/proception/pro-sdk/issues
- **Email**: support@proception.ai
- **Documentation**: See `sdk/docs/`

## License

See [LICENSE](LICENSE) for details.

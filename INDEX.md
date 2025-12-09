
# ProHand SDK Release Index

**Current Release**: ${SDK_VERSION}  
**Release Date**: $(date +"%Y-%m-%d")

## Version Information

- **SDK Version**: ${SDK_VERSION}
- **Firmware Version**: ${FW_VERSION}
- **macOS App Version**: ${APP_VERSION}

## Release Contents

### SDK (Unpacked in \`sdk/\`)
The SDK is provided as unpacked source code and libraries, not as a tarball. This allows for:
- Easy browsing on GitHub
- Direct cloning and use
- Better git diff viewing

**Includes**:
- \`sdk/prohand_sdk/\` - ProHand SDK (C++, Python)
- \`sdk/proglove_sdk/\` - ProGlove SDK (C++, Python)
- \`sdk/demo/\` - Example applications
- \`sdk/docs/\` - Documentation

### Driver Binaries (\`driver/\`)
#### (Defaults) Platform-specific driver bundles (unpacked folders):
- \`driver/macos-arm64/\` - macOS Apple Silicon
- \`driver/linux-arm64/\` - Linux ARM64 (Jetson, etc.)
- \`driver/linux-x64/\` - Linux x64 (Intel/AMD)

#### (Not In this Package) Platform-specific driver binaries packaged as tarballs:
- \`prohand-driver-${SDK_VERSION}-macos-arm64.tar.gz\` - macOS Apple Silicon
- \`prohand-driver-${SDK_VERSION}-linux-arm64.tar.gz\` - Linux ARM64 (Jetson, etc.)
- \`prohand-driver-${SDK_VERSION}-linux-x64.tar.gz\` - Linux x64 (Intel/AMD)

**Included in each driver package**:
- \`prohand-headless-ipc-host\` - ProHand IPC host driver
- \`proglove-headless-ipc-host\` - ProGlove IPC host driver

### Firmware (\`firmware/\`)
- \`ProHand-fw-esp-L-*-ota.bin\` - Left hand OTA firmware
- \`ProHand-fw-esp-R-*-ota.bin\` - Right hand OTA firmware

### macOS Application (\`app/\`)
- \`ProHand Diagnostic ${APP_VERSION}.dmg\` - Diagnostic and configuration tool

## Installation

### Using the SDK

Since the SDK is unpacked, you can use it directly:

\`\`\`bash
# Clone the repository
git clone https://github.com/proception/pro-sdk.git
cd pro-sdk

# Use Python SDK
cd sdk/prohand_sdk/python
python3 example.py

# Use C++ SDK
cd sdk/prohand_sdk/cpp
# See README for build instructions
\`\`\`

### Using Driver Binaries

Download and extract the driver package for your platform:

\`\`\`bash
# Example for macOS ARM64
wget https://github.com/proception/pro-sdk/releases/download/${SDK_VERSION}/prohand-driver-${SDK_VERSION}-macos-arm64.tar.gz
tar -xzf prohand-driver-${SDK_VERSION}-macos-arm64.tar.gz
./prohand-headless-ipc-host --help
\`\`\`

## Version History

Version management is handled via git tags. To see all releases:
\`\`\`bash
git tag -l
\`\`\`

To checkout a specific version:
\`\`\`bash
git checkout ${SDK_VERSION}
\`\`\`

## Manifest

See [MANIFEST.txt](MANIFEST.txt) for checksums and detailed file information.

## Support

For documentation and support:
- SDK Documentation: See \`sdk/docs/\`
- Issues: https://github.com/proception/pro-sdk/issues
- Email: support@proception.ai
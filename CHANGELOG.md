# Changelog

All notable changes to the Proception SDK will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- New features go here

### Changed
- Changes to existing functionality go here

### Fixed
- Bug fixes go here

---

## [0.3.0.0] - 2026-07-22

### Added
- ProGlove raw tactile API — `try_recv_raw_tactile()` and `has_raw_tactile()`
  (C++ and Python) expose pre-filter hardware ADC frames from the driver's
  secondary raw node, pollable alongside processed frames
- ProGlove Rerun demo (`proglove-rerun` / `just rerun [left|right]`) — logs
  raw + processed tactile to [Rerun](https://rerun.io); install with the
  `rerun` extra
- SDK version 0.3.0.0
- Firmware version 0.9.1.0
- macOS App version 0.3.0.0

---

## [0.2.6.0] - 2026-07-15

### Added
- ProWristCam SDK (C++, Python) — subscribes to a ZeroMQ stream endpoint and
  delivers JPEG frames from the wrist-mounted camera (`WristCamClient`)
- `prowristcam-headless-ipc-host` driver (macOS ARM64)
- `udcap-ctrl` utility — UDP capture control for dual-arm systems (macOS ARM64)
- ProWristCam demos: `connect_wristcam` / `test_wristcam` (C++) and the
  `prowrist_demo` package with a live frame viewer (Python)
- ProGlove OTA update and filter-control demos
- SDK version 0.2.6.0
- Firmware version 0.8.56.0
- macOS App version 0.2.6.0

### Hardware Support
- ProWristCam wrist camera (JPEG streaming)

---

## [v1.0.0-rc] - 2025-12-09

### Added
- Initial SDK release
- ProHand SDK with C++ and Python bindings
- ProGlove SDK with C++ and Python bindings
- 15+ demo applications showcasing SDK capabilities
- Comprehensive API documentation
- Firmware OTA update support
- macOS Diagnostic application

### Platform Support
- macOS ARM64 (Apple Silicon M1/M2/M3/M4)
- Linux ARM64 (Jetson Orin, ARM servers)
- Linux x64 (Intel/AMD processors)

### Hardware Support
- ProHand robotic hand (left and right configurations)
- ProGlove robotic glove
- ESP32-based firmware with wireless connectivity

### Notes
- First public release candidate
- Full IPC communication via ZeroMQ
- Real-time telemetry and control

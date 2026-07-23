# Proception SDK — top-level entry point.
#
# All SDK commands live in ./sdk and are exposed here as the `sdk` module, so you
# can drive everything from the repo root:
#
#   just sdk                list SDK commands
#   just sdk check          verify the libraries for this platform are present
#   just sdk install [pfx]  install C/C++ libs + headers (default /usr/local)
#   just sdk install-python install the Python SDKs into your env (uv, else pip)
#   just sdk demo ...       runnable examples (e.g. `just sdk demo python ping`)
#
# Driver host binaries are under ./driver/<platform>.

mod sdk 'sdk/justfile'

# Show the SDK command listing (forwards to the sdk module's help).
default:
    @just sdk

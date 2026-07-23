#!/usr/bin/env python3
"""
ProGlove SDK Demo: Filter Control

Demonstrates toggling the host-side tactile filter via
GloveIpcClient.set_denoise_enabled() / set_filter_enabled(), and shows the
effect on a live taxel value: filter enabled applies baseline subtraction +
EMA smoothing + deadzone hysteresis (+ denoise if on); disabling the whole
pipeline gives fully raw ADC values.
"""

import sys
import argparse
import time
from .utils import DemoBase


class FilterControlDemo(DemoBase):
    """Tactile filter enable/disable demo."""

    def __init__(self):
        super().__init__("ProGlove Filter Control Demo")

    def _sample(self, client, taxel_index: int, seconds: float) -> None:
        """Poll tactile status for `seconds`, printing one taxel's value."""
        deadline = time.time() + seconds
        last = None
        while time.time() < deadline:
            status = client.try_recv_status()
            if status and status.is_valid:
                last = status.upper_palm[taxel_index]
                print(f"    upper_palm[{taxel_index}] = {last:4d}", end="\r")
            time.sleep(0.01)
        print()
        if last is None:
            self.warning("No tactile data received - is streaming enabled?")

    def run(self, endpoint: str, taxel_index: int, hold_seconds: float) -> int:
        """Run the filter control demo."""
        self.banner()
        print(f"\nStatus endpoint: {endpoint}")
        print(f"Watching:        upper_palm[{taxel_index}]")

        client = None
        try:
            self.section(f"Connecting to {endpoint}...")
            client = self.sdk.ProGloveClient(endpoint)
            client.send_ping()
            self.success("Connected!")

            self.section(
                "Filter enabled (default): baseline + EMA + deadzone + denoise"
            )
            self._sample(client, taxel_index, hold_seconds)

            self.section("Disabling denoise (median pre-filter + stuck-pixel masking)")
            client.set_denoise_enabled(False)
            self._sample(client, taxel_index, hold_seconds)

            self.section("Re-enabling denoise")
            client.set_denoise_enabled(True)
            self._sample(client, taxel_index, hold_seconds)

            self.section("Disabling whole filter pipeline: fully raw ADC values")
            client.set_filter_enabled(False)
            self._sample(client, taxel_index, hold_seconds)

            self.section("Re-enabling whole filter pipeline")
            client.set_filter_enabled(True)
            self._sample(client, taxel_index, hold_seconds)

            self.success("Filter control demo completed!")
            return 0

        except self.sdk.ConnectionError as e:
            self.error(f"Connection failed: {e}")
            print("\nMake sure proglove-headless-ipc-host is running")
            return 1
        except self.sdk.ProGloveError as e:
            self.error(f"Demo failed: {e}")
            return 1
        except KeyboardInterrupt:
            self.info("\nInterrupted by user")
            return 0
        except Exception as e:
            self.error(f"Unexpected error: {e}")
            import traceback

            traceback.print_exc()
            return 1
        finally:
            if client is not None:
                try:
                    client.close()
                except Exception:
                    pass


def main() -> int:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Toggle the ProGlove tactile filter and observe the effect",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python -m proglove_demo.filter_control --status-endpoint ipc:///tmp/proglove-left-status.ipc
  python -m proglove_demo.filter_control --status-endpoint ipc:///tmp/proglove-right-status.ipc --taxel-index 3 --hold-seconds 5

Default endpoints:
  Left hand (IPC):  ipc:///tmp/proglove-left-status.ipc
  Right hand (IPC): ipc:///tmp/proglove-right-status.ipc
""",
    )
    parser.add_argument(
        "--status-endpoint",
        type=str,
        required=True,
        help="ZeroMQ status endpoint (e.g., ipc:///tmp/proglove-left-status.ipc)",
    )
    parser.add_argument(
        "--taxel-index",
        type=int,
        default=0,
        help="Index into upper_palm[] to watch while pressing on it (default: 0)",
    )
    parser.add_argument(
        "--hold-seconds",
        type=float,
        default=3.0,
        help="Seconds to sample in each filter state (default: 3.0)",
    )
    args = parser.parse_args()

    demo = FilterControlDemo()
    return demo.run(args.status_endpoint, args.taxel_index, args.hold_seconds)


if __name__ == "__main__":
    sys.exit(main())

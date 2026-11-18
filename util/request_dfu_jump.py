#!/usr/bin/env python3
"""Ask a running QMK keyboard to reboot into its bootloader (VIA command 0x0B)."""

import glob
import os


def main() -> int:
    for node in sorted(glob.glob('/sys/class/hidraw/hidraw*')):
        try:
            with open(os.path.join(node, 'device/report_descriptor'), 'rb') as descriptor_file:
                descriptor = descriptor_file.read(4)
        except OSError:
            continue
        # Usage page 0xFF60, usage 0x61: the QMK raw HID interface
        if descriptor != b'\x06\x60\xff\x09':
            continue
        try:
            with open('/dev/' + os.path.basename(node), 'r+b', buffering=0) as device:
                device.write(bytes([0x0B]) + bytes(31))
        except OSError:
            continue
        print(f'{os.path.basename(node)}: asked the keyboard to enter its bootloader')
        return 0
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

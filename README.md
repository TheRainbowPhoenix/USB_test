# ClassPad RAM dumper — host-selected range

JustUI progress and fxlink USB on ClassPad II. Windows selects inclusive
start/end addresses. Default: 0x8C000000 through 0x8CFFFFFF (16 MiB).

## Build

Use the fxSDK/gint/JustUI environment or this repository's GitHub Actions:

    fxsdk build-cp

Install MyAddin.hh3, launch it, tap Start dump, and connect USB.

## Windows

Put libfxlink.dll and libusb-1.0.dll from EXWord-Explorer's Windows
host GitHub Actions artifact beside host/receiver.py. Use 64-bit Python
3.10+ and WinUSB for the fxlink interface.

    py -3 host/receiver.py

or with explicit inclusive addresses:

    py -3 host/receiver.py --from 0x8C000000 --to 0x8CFFFFFF ram.bin

For an arbitrarily short range:

    py -3 host/receiver.py --from 0x8C000000 --to 0x8C000FFF tiny.bin

Output defaults to ram_START_END.bin, and incomplete runs leave .partial.

## Protocol

Receiver sends an 8-byte little-endian start:u32/end:u32 in an fxlink
ramdump:start message. Device sends ramdump:data with absolute_address:u32,
size:u32, then raw bytes. Up to 512 KiB of data per logical message;
the final message can be shorter. Device sends in 2 KiB USB slices.
JustUI updates progress between messages.

Max requested length is 32 MiB and addresses must be in
0x80000000..0x9FFFFFFF. These are merely numeric guards, NOT proof
of readable RAM. Unmapped access may crash the ClassPad; save work.
Not hardware-tested. 512 KiB logical messages may exceed limits of
some fxlink versions; change CHUNK in both firmware and receiver if needed.

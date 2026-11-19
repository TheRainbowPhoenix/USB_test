# ClassPad RAM dumper (32 MiB)

JustUI progress screen and fxlink USB bulk transfer for **ClassPad II** (fx-CP).
Reads the fixed virtual range **0x88000000–0x89FFFFFF** into a 32 MiB PC file.
This is a read-only diagnostic, NOT a probe of physical installed RAM. If
any address is not readable on your ClassPad, a CPU exception can reset/crash
the add-in. Close apps and save your work before trying it.

## Build the ClassPad app

Use the existing fxSDK/gint/JustUI toolchain (or the repository's GitHub
Actions CI workflow):

    fxsdk build-cp

Install `MyAddin.hh3` on the ClassPad and launch it. Tap **Start dump**.
Connect the USB cable, then start the Windows program. The JustUI display
updates once per 512 KiB message (64 messages).

## Windows receiver

Download the Windows host artifact from the EXWord-Explorer repository's
GitHub Actions (includes `libfxlink.dll` and `libusb-1.0.dll`). Put the
two 64-bit DLLs next to `host/receiver.py`; install 64-bit Python 3.10+
and ensure the fxlink USB interface is bound to WinUSB (eg via Zadig).

In PowerShell:

    py -3 host/receiver.py ram_88000000.bin

The receiver sends a `ramdump:start` request, waits for exactly 64
`ramdump:data` messages, checks offsets/lengths, and saves a
32 MiB binary. Interrupted dumps retain a `.partial` file.

## Wire protocol

* Host -> ClassPad: fxlink application `ramdump`, type `start`,
  4-byte ASCII payload `GO32`.
* ClassPad -> host: application `ramdump`, type `data`, body:
  `u32 little-endian offset`, `u32 little-endian size`,
  `size` raw bytes, with size = 524288.
* Logical message size is 524296 bytes; gint writes it in 2048-byte slices.
  This **does not** mean USB hardware packets are 512 KiB.
* At most 2 KiB sent at a time by the firmware; no 512 KiB firmware buffer.

## Known limitations

Not hardware-tested. The fixed address range is assumed readable, and a
fault cannot be recovered by this add-in. fxlink host and gint USB drivers
must accept 512 KiB logical messages; some builds may impose smaller
message limits. Try lowering `MESSAGE_SIZE` in firmware and the
`CHUNK` constant in the receiver if this fails, keeping both equal and
dividing 32 MiB exactly. The display is refreshed only between messages,
not continuously inside the current 512 KiB transfer.

import ctypes
import sys
import time
import os

# --- Struct Definitions ---

class fxlink_filter(ctypes.Structure):
    _fields_ = [
        ("p7", ctypes.c_bool),
        ("mass_storage", ctypes.c_bool),
        ("intf_fxlink", ctypes.c_bool),
        ("intf_cesg502", ctypes.c_bool),
        ("series_cg", ctypes.c_bool),
        ("series_g3", ctypes.c_bool),
        ("serial", ctypes.c_char_p),
    ]

class fxlink_message(ctypes.Structure):
    _fields_ = [
        ("version", ctypes.c_uint32),
        ("size", ctypes.c_uint32),
        ("transfer_size", ctypes.c_uint32),
        ("application", ctypes.c_char * 16),
        ("type", ctypes.c_char * 16),
        ("_padding", ctypes.c_int),
        ("data", ctypes.c_void_p),
    ]

# Opaque structs
class libusb_context(ctypes.Structure):
    pass
class fxlink_device(ctypes.Structure):
    pass

# --- Loading Libraries ---

def load_libs():
    # Load libusb first
    try:
        libusb = ctypes.CDLL("./libusb-1.0.dll")
    except OSError:
        try:
            libusb = ctypes.CDLL("libusb-1.0.dll")
        except OSError:
            print("Error: Could not load libusb-1.0.dll. Ensure it is in the current directory.")
            sys.exit(1)

    # Load libfxlink
    try:
        fxlink = ctypes.CDLL("./libfxlink.dll")
    except OSError:
        try:
            fxlink = ctypes.CDLL("libfxlink.dll")
        except OSError:
            print("Error: Could not load libfxlink.dll. Ensure it is in the current directory.")
            sys.exit(1)

    return libusb, fxlink

# --- Main Script ---

def main():
    print("Initializing fxlink python wrapper...")
    libusb, fxlink = load_libs()

    # Initialize libusb
    ctx = ctypes.POINTER(libusb_context)()
    rc = libusb.libusb_init(ctypes.byref(ctx))
    if rc != 0:
        print(f"Failed to initialize libusb: {rc}")
        sys.exit(1)

    # Setup filter to find a calculator with fxlink interface
    filt = fxlink_filter()
    filt.intf_fxlink = True

    # Configure function prototypes
    fxlink.fxlink_device_find.argtypes = [ctypes.POINTER(libusb_context), ctypes.POINTER(fxlink_filter)]
    fxlink.fxlink_device_find.restype = ctypes.POINTER(fxlink_device)

    fxlink.fxlink_device_claim_fxlink.argtypes = [ctypes.POINTER(fxlink_device)]
    fxlink.fxlink_device_claim_fxlink.restype = ctypes.c_bool

    fxlink.fxlink_device_id.argtypes = [ctypes.POINTER(fxlink_device)]
    fxlink.fxlink_device_id.restype = ctypes.c_char_p

    fxlink.fxlink_device_start_bulk_IN.argtypes = [ctypes.POINTER(fxlink_device)]
    fxlink.fxlink_device_start_bulk_IN.restype = ctypes.c_bool

    fxlink.fxlink_device_start_bulk_OUT.argtypes = [ctypes.POINTER(fxlink_device), ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_bool]
    fxlink.fxlink_device_start_bulk_OUT.restype = ctypes.c_bool

    fxlink.fxlink_device_finish_bulk_IN.argtypes = [ctypes.POINTER(fxlink_device)]
    fxlink.fxlink_device_finish_bulk_IN.restype = ctypes.POINTER(fxlink_message)

    fxlink.fxlink_message_free.argtypes = [ctypes.POINTER(fxlink_message), ctypes.c_bool]

    libusb.libusb_handle_events.argtypes = [ctypes.POINTER(libusb_context)]

    print("Waiting for calculator connection...")
    fdev = None
    while not fdev:
        # We manually loop to allow ctrl-c, although fxlink_device_find is non-blocking
        fdev = fxlink.fxlink_device_find(ctx, ctypes.byref(filt))
        if not fdev:
            # Handle events to let enumeration happen?
            # Actually fxlink_device_find does simplified enumeration which calls libusb functions directly.
            # But typically we might need to wait a bit or retry.
            libusb.libusb_handle_events(ctx)
            time.sleep(0.5)

    dev_id = fxlink.fxlink_device_id(fdev).decode('utf-8')
    print(f"Found device: {dev_id}")

    if not fxlink.fxlink_device_claim_fxlink(fdev):
        print("Failed to claim fxlink interface. Ensure drivers are installed (Zadig -> WinUSB).")
        sys.exit(1)

    print("Connected! Entering command loop.")
    print("Type '/echo Hello' to test sending. Press Ctrl+C to exit.")

    # Start listening
    fxlink.fxlink_device_start_bulk_IN(fdev)

    import threading
    import queue
    input_queue = queue.Queue()

    def input_thread():
        while True:
            try:
                cmd = input()
                input_queue.put(cmd)
            except EOFError:
                break

    t = threading.Thread(target=input_thread, daemon=True)
    t.start()

    try:
        while True:
            # Handle USB events
            libusb.libusb_handle_events(ctx)

            # Check incoming messages
            msg_ptr = fxlink.fxlink_device_finish_bulk_IN(fdev)
            if msg_ptr:
                msg = msg_ptr.contents
                app = msg.application.decode('utf-8', errors='ignore')
                typ = msg.type.decode('utf-8', errors='ignore')
                size = msg.size

                # Copy data
                data_buffer = (ctypes.c_char * size).from_address(msg.data)
                data_bytes = data_buffer.raw

                print(f"\n[RX] {app}:{typ} ({size} bytes)")
                if typ == "text":
                    print(f"     Text: {data_bytes.decode('utf-8', errors='ignore')}")
                elif typ == "image":
                    print(f"     Image received (saving not implemented in this script)")
                else:
                    print(f"     Data: {data_bytes[:20]}...")

                fxlink.fxlink_message_free(msg_ptr, True)
                # Restart listening
                fxlink.fxlink_device_start_bulk_IN(fdev)

            # Check user input
            try:
                cmd = input_queue.get_nowait()
                if cmd.startswith("/echo "):
                    payload = cmd[6:] + "\n" # Add newline as echo expects it usually? Or generic text.
                    # Send command "echo payload"
                    full_cmd = f"echo {payload}".encode('utf-8')
                    # Actually standard /echo sends app="fxlink" type="command" data="echo payload"
                    # Let's emulate that.

                    c_app = b"fxlink"
                    c_type = b"command"
                    c_data = ctypes.create_string_buffer(full_cmd)

                    # We need to allocate data for the library to own if we set own_data=True
                    # But ctypes buffers are managed by python.
                    # Let's set own_data=False and manage it ourselves (keep ref until sent?)
                    # Actually start_bulk_OUT copies if own_data=False usually? No wait.
                    # Helper: fxlink_device_start_bulk_OUT(..., bool own_data)
                    # "If `own_data` is set, the transfer will free(data) when it completes."
                    # We can't let C free python memory easily. So own_data=False.
                    # "If own_data is false, the caller must keep the data valid until completion."
                    # Since we are single threaded regarding the transfer start, and `start_bulk_OUT`
                    # queues the transfer... libusb handles it async.
                    # We should probably wait for it or just keep the buffer alive.
                    # For this simple script, let's just make a copy in C using malloc if needed?
                    # Or just hope `fxlink` copies it?
                    # Looking at `fxlink/src/fxlink/devices.c`:
                    # It creates a `fxlink_transfer_make_OUT`.
                    # `fxlink_transfer_make_OUT` copies data to `tr->msg.data` if `own_data` is false?
                    # No, it just assigns the pointer.
                    # Actually, we should check `fxlink_transfer_make_OUT` source.
                    # If I don't have it, safest is to pass `own_data=False` and keep the buffer in a global list until confirmed sent?
                    # Or simpler: For small commands, `fxlink_device_send_bulk_OUT` is blocking/sync.
                    # Let's use `fxlink_device_send_bulk_OUT` if available.

                    fxlink.fxlink_device_send_bulk_OUT.argtypes = [ctypes.POINTER(libusb_context), ctypes.POINTER(fxlink_device), ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_int]
                    fxlink.fxlink_device_send_bulk_OUT.restype = ctypes.c_bool

                    print(f"Sending: {full_cmd}")
                    fxlink.fxlink_device_send_bulk_OUT(ctx, fdev, c_app, c_type, c_data, len(full_cmd))

                else:
                    print("Unknown command. Try /echo Hello")
            except queue.Empty:
                pass

            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nExiting...")

if __name__ == "__main__":
    main()

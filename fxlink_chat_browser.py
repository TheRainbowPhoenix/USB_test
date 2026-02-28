import ctypes
import sys
import time
import queue
import threading
import urllib.request
import urllib.error

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
    try:
        libusb = ctypes.CDLL("./libusb-1.0.dll")
    except OSError:
        try:
            libusb = ctypes.CDLL("libusb-1.0.dll")
        except OSError:
            print("Error: Could not load libusb-1.0.dll.")
            sys.exit(1)

    try:
        fxlink = ctypes.CDLL("./libfxlink.dll")
    except OSError:
        try:
            fxlink = ctypes.CDLL("libfxlink.dll")
        except OSError:
            print("Error: Could not load libfxlink.dll.")
            sys.exit(1)

    return libusb, fxlink

# --- Web Fetcher ---
def fetch_webpage(url):
    print(f"Fetching URL: {url} ...")
    if not url.startswith("http"):
        url = "http://" + url
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=10) as response:
            html = response.read().decode('utf-8', errors='ignore')
            # Very basic strip to get some readable text
            # For a real app, use BeautifulSoup, but this is a demo.
            text = html.replace('\r', '')
            # Just send the first 2000 chars to avoid overwhelming the basic demo buffer
            return text[:2000]
    except urllib.error.URLError as e:
        return f"Error fetching {url}: {e}"
    except Exception as e:
        return f"Error: {e}"

# --- Main Script ---

def main():
    print("Initializing fxlink python wrapper (Chat & Browser)...")
    libusb, fxlink = load_libs()

    # Initialize libusb
    ctx = ctypes.POINTER(libusb_context)()
    rc = libusb.libusb_init(ctypes.byref(ctx))
    if rc != 0:
        print(f"Failed to initialize libusb: {rc}")
        sys.exit(1)

    # Setup filter
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
    fxlink.fxlink_device_send_bulk_OUT.argtypes = [ctypes.POINTER(libusb_context), ctypes.POINTER(fxlink_device), ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p, ctypes.c_int]
    fxlink.fxlink_device_send_bulk_OUT.restype = ctypes.c_bool
    fxlink.fxlink_device_finish_bulk_IN.argtypes = [ctypes.POINTER(fxlink_device)]
    fxlink.fxlink_device_finish_bulk_IN.restype = ctypes.POINTER(fxlink_message)
    fxlink.fxlink_message_free.argtypes = [ctypes.POINTER(fxlink_message), ctypes.c_bool]
    libusb.libusb_handle_events.argtypes = [ctypes.POINTER(libusb_context)]

    print("Waiting for calculator connection...")
    fdev = None
    while not fdev:
        fdev = fxlink.fxlink_device_find(ctx, ctypes.byref(filt))
        if not fdev:
            libusb.libusb_handle_events(ctx)
            time.sleep(0.5)

    dev_id = fxlink.fxlink_device_id(fdev).decode('utf-8')
    print(f"Connected to device: {dev_id}")

    if not fxlink.fxlink_device_claim_fxlink(fdev):
        print("Failed to claim fxlink interface.")
        sys.exit(1)

    print("\n--- Commands ---")
    print("  /get <url>  : Fetch a webpage and send to calc")
    print("  /quit       : Exit the script")
    print("  <text>      : Send chat message to calc")
    print("----------------\n")

    fxlink.fxlink_device_start_bulk_IN(fdev)

    input_queue = queue.Queue()

    def input_thread():
        while True:
            try:
                cmd = input("> ")
                input_queue.put(cmd)
            except EOFError:
                break

    t = threading.Thread(target=input_thread, daemon=True)
    t.start()

    def send_to_calc(app, type_str, payload):
        c_app = app.encode('utf-8')
        c_type = type_str.encode('utf-8')
        c_data = ctypes.create_string_buffer(payload.encode('utf-8'))
        fxlink.fxlink_device_send_bulk_OUT(ctx, fdev, c_app, c_type, c_data, len(payload))

    try:
        while True:
            libusb.libusb_handle_events(ctx)

            # Receive
            msg_ptr = fxlink.fxlink_device_finish_bulk_IN(fdev)
            if msg_ptr:
                msg = msg_ptr.contents
                app = msg.application.decode('utf-8', errors='ignore')
                typ = msg.type.decode('utf-8', errors='ignore')
                size = msg.size

                data_buffer = (ctypes.c_char * size).from_address(msg.data)
                data_bytes = data_buffer.raw

                if typ == "text":
                    print(f"\n[Calc]: {data_bytes.decode('utf-8', errors='ignore')}\n> ", end="")
                elif typ == "command":
                    cmd_str = data_bytes.decode('utf-8', errors='ignore').strip()
                    print(f"\n[Calc Command]: {cmd_str}\n> ", end="")
                    if cmd_str == "quit":
                        break
                else:
                    print(f"\n[Calc Data - {typ}]: {size} bytes\n> ", end="")

                fxlink.fxlink_message_free(msg_ptr, True)
                fxlink.fxlink_device_start_bulk_IN(fdev)

            # Send
            try:
                cmd = input_queue.get_nowait()
                if cmd.startswith("/get "):
                    url = cmd[5:].strip()
                    content = fetch_webpage(url)
                    send_to_calc("python", "text", content)
                    print("Webpage sent.")
                elif cmd == "/quit":
                    send_to_calc("fxlink", "command", "quit\n")
                    break
                else:
                    # Send as generic text
                    send_to_calc("python", "text", cmd)
            except queue.Empty:
                pass

            time.sleep(0.01)

    except KeyboardInterrupt:
        pass
    print("\nExiting...")

if __name__ == "__main__":
    main()

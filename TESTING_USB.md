# JustUI Template Add-in & USB Testing

This project serves as a template for JustUI-based add-ins on Casio calculators and includes a dedicated USB testing suite using `gint`.

## Features
- **Fullscreen Layout**: Pre-configured `jscene_create_fullscreen` layout.
- **Tabs**: Example of tabbed interface using `jlayout_set_stack`.
- **USB Testing**: A dedicated tab to test synchronous and asynchronous USB transfers with `fxlink`.

## Testing USB on Windows

To test the USB functionality, you need the `fxlink` tool running on your Windows PC.

### 1. Prerequisites
- **Artifacts**: Download the latest build from the [GitHub Actions artifacts](https://github.com/TheRainbowPhoenix/USB_test/actions). Extract the zip file. This contains `fxlink.exe`, `libfxlink.dll`, and necessary DLLs (`libusb-1.0.dll`, etc.).
- **Drivers**: Ensure you have `libusb` compatible drivers installed for your calculator. You can use [Zadig](https://zadig.akeo.ie/) to install `WinUSB` driver for the calculator device when connected.

### 2. Running fxlink (CLI & Win32 GUI)
Open a command prompt (cmd or PowerShell) in the folder where you extracted the artifacts.

To list connected devices:
```cmd
fxlink.exe -l
```

To start listening for data (Interactive Mode):
```cmd
fxlink.exe -i
```

To start the **Windows GUI** mode (recommended for Windows testing):
```cmd
fxlink.exe -t
```
*Note: This opens a native Windows window where you can view logs, save them via right-click context menu, and send commands like `/echo`, `/vram`, `/quit`.*

### 3. Running fxlink (Python Script - Chat & Browser)
For advanced testing, a Python script (`fxlink_chat_browser.py`) is provided. This acts as a basic chat client and web text fetcher.

1. Ensure you have Python installed.
2. Place `fxlink_chat_browser.py` in the same folder as the extracted artifacts (`libfxlink.dll`, `libusb-1.0.dll`, etc.).
3. Run the script:
   ```cmd
   python fxlink_chat_browser.py
   ```
4. The script will wait for a calculator connection. Once connected:
   - Type normal text to send a chat message to the calculator.
   - Type `/get example.com` to fetch the HTML of a webpage and send it to the calculator as text.
   - Type `/quit` to close.

### 4. Running the Test on Calculator
1.  Transfer the compiled `.g1a` or `.g3a` add-in to your calculator.
2.  Launch the add-in.
3.  Navigate to the **USB** tab (Tab 4) using the bottom navigation bar.
4.  Press **Open** or **Open Wait** to initialize the USB connection. You should see the calculator detected in `fxlink`.

### 5. Performing Tests

#### Sending Data (Calculator -> PC)
1.  On the calculator, press **WA TxtHead** (sends "text" header).
2.  Press **WA Text** (sends "Hello JustUI!").
3.  Press **Commit A** (commits the transfer asynchronously).
4.  Check `fxlink` (or python script) output. You should see "Hello JustUI!".

#### Receiving Data (PC -> Calculator)
1.  On the PC (in `fxlink -t` GUI or Python script), send a command:
    - In the Win32 GUI, type `/echo HelloCalc` and click Send.
    - Or use the Python script to send a message.
2.  On the calculator, press **Read Sync** (or **Read Async**).
3.  The status label on the calculator should update to show the received data (e.g., "Read X: ...").

#### VRAM & Quit Commands
- In the `fxlink -t` Win32 GUI, type `/vram` and click Send. If you have a custom script on the calculator designed to handle this (like the Python one mentioned in the docs), it will respond.
- Type `/quit` to gracefully exit the GUI and notify the calculator.

#### Synchronous Mode
You can repeat the write tests using the **WS** (Write Sync) buttons (`WS TxtHead`, `WS Text`, `Commit S`). The behavior should be identical, but the calculator UI might freeze briefly during transfer (blocking call).

### 6. Troubleshooting
- **Missing DLLs**: Ensure all `.dll` files included in the artifact zip are in the same folder as `fxlink.exe` / `fxlink_py.py`.
- **Device not found**: Check device manager and Zadig to ensure `WinUSB` driver is loaded. Re-plug the calculator.
- **-p flag fails**: The `-p` flag is for the official "Add-In Push" application protocol. This demo uses the `gint` bulk transfer protocol, so standard `-p` will not work. Use `-t` (interactive GUI) to communicate.

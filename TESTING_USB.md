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

### 2. Running fxlink (CLI)
Open a command prompt (cmd or PowerShell) in the folder where you extracted the artifacts.

To list connected devices:
```cmd
fxlink.exe -l
```

To start listening for data (Interactive Mode):
```cmd
fxlink.exe -i
```
Or for TUI mode (better visualization):
```cmd
fxlink.exe -t
```
*Note: If `fxlink.exe -t` or `-i` crashes or fails on Windows (common issue with TUI/Console handling), use the Python script method below.*

### 3. Running fxlink (Python Script)
If the CLI tool issues occur, you can use the provided Python script `fxlink_py.py` to interact with the calculator using `libfxlink.dll`.

1. Ensure you have Python installed.
2. Place `fxlink_py.py` in the same folder as the extracted artifacts (`libfxlink.dll`, `libusb-1.0.dll`, etc.).
3. Run the script:
   ```cmd
   python fxlink_py.py
   ```
4. The script will wait for a calculator connection. Once connected, it will print received messages and allow sending commands.

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

#### Screenshot/VRAM Test
1.  On the calculator, press **WA ImgHead** (sends "image" header with VRAM size).
2.  Press **WA VRAM** (sends the raw VRAM buffer).
3.  Press **Commit A**.
4.  `fxlink` should detect the image data. The Python script will report "Image received".

#### Receiving Data (PC -> Calculator)
1.  On the PC (in `fxlink -t` or `python fxlink_py.py`), type a command:
    ```
    /echo HelloCalc
    ```
2.  On the calculator, press **Read Sync** (or **Read Async**).
3.  The status label on the calculator should update to show the received data (e.g., "Read X: ...").
    *Note: The raw read will likely capture the `fxlink` header first. You might need to press Read multiple times or parse the protocol to see the payload "HelloCalc".*

### 6. Troubleshooting
- **Missing DLLs**: Ensure all `.dll` files included in the artifact zip are in the same folder as `fxlink.exe` / `fxlink_py.py`.
- **Device not found**: Check device manager and Zadig to ensure `WinUSB` driver is loaded. Re-plug the calculator.

# JustUI Template Add-in & USB Testing

This project serves as a template for JustUI-based add-ins on Casio calculators and includes a dedicated USB testing suite using `gint`.

## Features
- **Fullscreen Layout**: Pre-configured `jscene_create_fullscreen` layout.
- **Tabs**: Example of tabbed interface using `jlayout_set_stack`.
- **USB Testing**: A dedicated tab to test synchronous and asynchronous USB transfers with `fxlink`.

## Testing USB on Windows

To test the USB functionality, you need the `fxlink` tool running on your Windows PC.

### 1. Prerequisites
- **fxlink.exe**: Download the latest build from the [GitHub Actions artifacts](https://github.com/TheRainbowPhoenix/USB_test/actions). Extract the zip file.
- **Drivers**: Ensure you have `libusb` compatible drivers installed for your calculator. You can use [Zadig](https://zadig.akeo.ie/) to install `WinUSB` driver for the calculator device when connected.

### 2. Running fxlink
Open a command prompt (cmd or PowerShell) in the folder where you extracted `fxlink.exe`.

To list connected devices:
```cmd
fxlink.exe -l
```

To start listening for data and sending commands (Interactive TUI Mode):
```cmd
fxlink.exe -t
```
*Note: TUI mode allows you to see incoming messages and type commands to send.*

### 3. Running the Test on Calculator
1.  Transfer the compiled `.g1a` or `.g3a` add-in to your calculator.
2.  Launch the add-in.
3.  Navigate to the **USB** tab (Tab 4) using the bottom navigation bar.
4.  Press **Open** or **Open Wait** to initialize the USB connection. You should see the calculator detected in `fxlink`.

### 4. Performing Tests

#### Sending Data (Calculator -> PC)
1.  On the calculator, press **WA TxtHead** (sends "text" header).
2.  Press **WA Text** (sends "Hello JustUI!").
3.  Press **Commit A** (commits the transfer asynchronously).
4.  Check `fxlink` output. You should see "Hello JustUI!".

#### Screenshot/VRAM Test
1.  On the calculator, press **WA ImgHead** (sends "image" header with VRAM size).
2.  Press **WA VRAM** (sends the raw VRAM buffer).
3.  Press **Commit A**.
4.  `fxlink` should detect the image data and likely save it as a PNG file in the current directory or display it.

#### Receiving Data (PC -> Calculator)
1.  On the PC, in `fxlink -t` mode, type a command that sends data. For simple text testing, you can try using the echo command which sends a packet back to the calc:
    ```
    /echo HelloCalc
    ```
2.  On the calculator, press **Read Sync** (or **Read Async**).
3.  The status label on the calculator should update to show the received data (e.g., "Read X: ...").
    *Note: The raw read will likely capture the `fxlink` header first. You might need to press Read multiple times or parse the protocol to see the payload "HelloCalc".*

#### Synchronous Mode
You can repeat the write tests using the **WS** (Write Sync) buttons (`WS TxtHead`, `WS Text`, `Commit S`). The behavior should be identical, but the calculator UI might freeze briefly during transfer (blocking call).

### 5. Troubleshooting
- **Missing DLLs**: Ensure all `.dll` files included in the artifact zip are in the same folder as `fxlink.exe`.
- **Device not found**: Check device manager and Zadig to ensure `WinUSB` driver is loaded. Re-plug the calculator.
- **-p flag fails**: The `-p` flag is for the official "Add-In Push" application protocol. This demo uses the `gint` bulk transfer protocol, so standard `-p` will not work. Use `-t` (interactive) to communicate.

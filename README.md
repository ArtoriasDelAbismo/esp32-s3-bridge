# ESP32-S3 Serial Bridge for Claude Code

An ESP32-S3 that lets [Claude Code](https://claude.com/claude-code) inspect a computer that is **not on the network**. The board plugs into the target machine over USB, shows up there as a serial port, and relays that serial port to your network over WiFi.

The target here is an old laptop running Arch Linux, but any Linux machine that can run a login shell on a USB serial port works the same way.

```
Your PC (Claude Code) ──WiFi/TCP:2323──▶ ESP32-S3 ──USB (CDC serial)──▶ Target laptop
                      ◀── command output ─────────────────────────────── login shell
```

The target laptop needs **no network connection and no extra software**. To it, the board is just a USB serial device.

## Status

| Step | What it adds | Status |
|---|---|---|
| 1. Toolchain | PlatformIO, board detected | ✅ Done |
| 2. Blink test | Build → flash → serial monitor chain works | ✅ Done |
| 3. Native USB serial | The board appears as its own USB serial port (`USBSerial`) | ✅ Done |
| 4. WiFi + TCP bridge | Bytes forwarded both ways between USB and a TCP client | ✅ Done |
| 5. Shell on the target | `serial-getty@ttyACM0` gives a login prompt through the bridge | ⏭️ Next |
| 6. MCP server | Python tools for Claude Code: `run_command`, `read_file`, `system_info`, `send_keys` | Planned |
| 7. Keyboard mode | USB HID keyboard to type the one-time setup command on the target | Optional |
| 8. Hardening | Authentication, read-only mode, reconnect handling | Planned |

## Hardware

- **ESP32-S3 dev board** with two USB-C ports (e.g. ESP32-S3-DevKitC-1 or a clone)
  - **UART / COM port**: goes through a CH343 USB-to-serial chip. Used for flashing and debug output.
  - **USB port**: the S3's native USB. This is the one that plugs into the target.
- Onboard RGB LED on GPIO 48 (GPIO 38 on some board revisions)
- Two USB-C data cables (some cables only charge)
- A **2.4 GHz** WiFi network. The ESP32-S3 can't use 5 GHz.

## How it works

The firmware uses three serial channels:

| In code | Physical port | Role |
|---|---|---|
| `Serial` | UART/COM port (CH343) | Debug messages: boot, IP address, "Client connected" |
| `USBSerial` | Native USB port | The target machine's side (`/dev/ttyACM0` on Linux) |
| `client` | WiFi, TCP port 2323 | Your PC's side |

On boot, the board joins WiFi and prints its IP address on the debug port. It then runs a TCP server on port 2323. When a client connects, the LED turns blue, and every byte is forwarded **unchanged** in both directions:

```
client.available()    → client.read()    → USBSerial.write()   (network → target)
USBSerial.available() → USBSerial.read() → client.write()      (target → network)
```

The native USB runs in **TinyUSB mode** (`ARDUINO_USB_MODE=0`). That makes the board a composite USB device, so a keyboard can be added later alongside the serial port (step 7).

## Project layout

```
esp32/
├── README.md
└── firmware/                 PlatformIO project (Arduino framework)
    ├── platformio.ini        board, ports, USB mode flags
    ├── include/
    │   └── secrets.h         WiFi credentials (not in git, create it yourself)
    └── src/
        └── main.cpp          the bridge
```

A `mcp-bridge/` folder for the Python MCP server will be added in step 6.

## Setup

### 1. Install PlatformIO

Install the **PlatformIO IDE** extension in VS Code, or `pip install platformio`. To use `pio` from any terminal on Windows, add its folder to your PATH:

```powershell
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";$env:USERPROFILE\.platformio\penv\Scripts", "User")
```

### 2. Create the WiFi credentials file

Create `firmware/include/secrets.h`:

```cpp
#pragma once
#define WIFI_SSID "your-2.4GHz-network"
#define WIFI_PASS "your-password"
```

Note the format: a space and **no `=`** between the name and the value. Both values are case-sensitive. This file is listed in `.gitignore`.

### 3. Set your upload port

In `firmware/platformio.ini`, set `upload_port` to the COM port of the board's **UART** connector. Find it with:

```powershell
pio device list
```

It's the one described as `USB-Enhanced-SERIAL CH343` (VID `1A86`).

### 4. Build and flash

Plug the **UART** port into your PC, then:

```powershell
cd firmware
pio run              # build only, a quick way to check for errors
pio run -t upload    # build and flash
```

If the upload fails with `Failed to connect`: hold **BOOT**, tap **RST**, release BOOT, and upload again.

## Testing the bridge

Plug in **both** USB ports. The native USB port appears as a new COM port with VID `303A` (Espressif).

Open three terminals:

```powershell
# 1. Debug output: shows the board's IP address after it joins WiFi
pio device monitor -p COM6

# 2. The "target" side (native USB port)
pio device monitor -p COM9

# 3. The "network" side, over WiFi (use the IP from terminal 1)
pio device monitor -p socket://<board-ip>:2323
```

Use your own port numbers. Press **RST** after opening terminal 1 to see the startup messages from the beginning.

Expected result:
- Terminal 1 prints `Bridge ready`, a few dots, the IP, then `Client connected` when terminal 3 opens.
- The LED turns blue.
- Text typed in terminal 3 appears in terminal 2, and text typed in terminal 2 appears in terminal 3.

There is no echo, so what you type only appears on the **other** side.

To see the exact bytes that arrive, add `-f hexlify` to a monitor command.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| Endless dots on the debug port | Can't join WiFi. Check the name and password (case-sensitive), and make sure it's a **2.4 GHz** network. |
| `could not open port` / `Acceso denegado` (access denied) | Another program has the port open, often a monitor left running. Close it with Ctrl+C. Uploading also needs the UART port to be free. |
| No IP shown on the debug port | It's printed only once at startup. Press **RST** with the monitor open. |
| No `303A` port appears | Check the cable on the native USB port, press RST, or look in Device Manager under "Other devices". |
| LED never lights | Try GPIO 38 instead of 48. |
| Backspace shows as `␛[C` in the monitor | A display quirk of PlatformIO's monitor on Windows. The board receives a correct `08` byte (checked with `hexlify`). |

## Security

Once step 5 is done, anyone who can connect to port 2323 on your network reaches the target's login prompt, and after logging in has a **full shell**. With `sudo`, that includes deleting files or the whole OS. Until step 8 adds authentication:

- Only run the bridge on a network you trust.
- Log in as a normal user, not root.
- Keep Claude Code's command approval prompts on, so you see every command before it runs.
- Consider a restricted or read-only user on the target if you only need inspection.

Limitations by design: the target gets no internet access through the bridge (so `pacman` can't download packages), and Claude only sees command output, not the screen.

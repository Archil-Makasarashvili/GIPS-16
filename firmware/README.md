# GIPS-16 firmware

ESP-IDF v6.1.1 project for the ESP32-C5. `main/idf_component.yml` pins the SDK
version. `main/` is the checkout app. `drivers/board/gips_board.h` holds the
single-board pin map. The DS3231M RTC driver reads and sets the clock over I2C.
Display output is disabled; the button driver is a stub. Neither touches hardware.
Firmware-specific instructions and style are in `AGENTS.md` and `STYLE.md`.

## Checkout CLI

Connect to the USB serial port and use:

```text
help
dsp
dsp off
rtc
rtc set 2026-11-22 14:30:00
```

`dsp` requests one additional lit pixel on row 4, up to 111 columns.
`dsp off` clears the requested frame and resets the next column to 1. Both
commands report that display output is unsupported: no pixels are driven or
scanned yet. `rtc` reads the clock; `rtc set` writes a date and time without
timezone conversion. The RTC driver uses `struct tm`: `tm_year` is years since 1900 and
`tm_mon` is zero-based. It accepts years 2000-2099. An oscillator-stop flag
makes `rtc` report invalid time until the clock is set. I2C starts only when an
RTC command runs. No display GPIOs are configured or driven at boot.

The pin map in `drivers/board` is transcribed from the schematic. The current
PCB file shows the U6 display pads unconnected; confirm the actual routed PCB
and power stage before enabling display GPIOs. The updated schematic connects
U3 FB, but the physical power stage still requires verification. A verified
cathode scan sequence is also required before display output can be enabled.

## Build and debug on Windows

Open `firmware/` as the VS Code folder for the Espressif ESP-IDF extension.
The extension uses the opened folder as the project root and writes its build
files to `firmware/build/`. Use a native ESP-IDF v6.1.1 installation. From the
ESP-IDF terminal in `firmware/`:

```powershell
idf.py set-target esp32c5
idf.py build
```

Flash and monitor through the board's USB Serial/JTAG port with
`idf.py -p COMx flash monitor` (replace `COMx` with the detected port). For
breakpoints, use the extension's OpenOCD/GDB debugger with the ESP32-C5
built-in USB/JTAG configuration. USB/JTAG connectivity depends on the routed
PCB and Windows driver setup.

## Release artifacts

`release/gips16_checkout-flash.bin` combines the bootloader, partition table,
and app for the ESP32-C5; flash it at offset `0x0`. Replace `COMx` with the
board's port and run in an ESP-IDF terminal:

```powershell
python -m esptool --chip esp32c5 -p COMx write-flash 0x0 release/gips16_checkout-flash.bin
```

`release/gips16_checkout.elf` contains debugging symbols. The current binary has
display output disabled and is not a display checkout pass.

## Optional Docker build

From this directory in WSL with Docker available:

```sh
docker run --rm -v "$PWD:/project" -w /project espressif/idf:v6.1.1 idf.py -B build-wsl set-target esp32c5 build
```

This is a separate reproducible build check, not the Windows debug build. It
uses `build-wsl/` so Linux and Windows CMake files do not mix. The source
currently lives on `C:`, which WSL accesses under `/mnt/c`; that mount may
make container builds slower.

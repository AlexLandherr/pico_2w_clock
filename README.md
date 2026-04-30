# Pico 2 W Clock

A Raspberry Pi Pico 2 W / Pico 2 WH clock-display project using the official Raspberry Pi Pico C/C++ SDK.

Current status:

- Builds with Pico SDK, CMake, and Ninja.
- Flashes successfully with the Debian-packaged `picotool`.
- Drives the Waveshare Pico-8SEG-LED display.
- Current smoke-test firmware displays `12:34` and blinks the decimal-point indicator once per second.
- WIZnet Ethernet HAT hardware is installed/planned for later Ethernet/NTP work, but Ethernet is not yet enabled in the application firmware.

## Hardware list

### Main board

Raspberry Pi Pico 2 WH

The Pico 2 WH is equivalent to the Pico 2 W for this project, but with pre-soldered headers.

Official documentation and code:

- Official product page: https://www.raspberrypi.com/products/raspberry-pi-pico-2/
- Pico C/C++ SDK: https://github.com/raspberrypi/pico-sdk
- Pico SDK documentation: https://www.raspberrypi.com/documentation/pico-sdk/
- Pico SDK PDF documentation: https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf

### Display

Waveshare Pico-8SEG-LED

A 4-digit 7-segment display HAT for Raspberry Pi Pico-style headers. It uses 74HC595-style serial/shift-register control.

Pin use in this project:

- RCLK: GP9
- CLK: GP10
- DIN: GP11

Notes from hardware testing:

- The display driver in this project uses active-high/common-cathode-style segment values.
- An earlier active-low segment map produced incorrect output; the corrected mapping displays `12:34` as expected.
- The decimal-point bit is used as the current 1 Hz blinking indicator.

Official documentation and code:

- Official Waveshare wiki: https://www.waveshare.com/wiki/Pico-8SEG-LED
- Waveshare demo code download page is linked from the wiki.

### Ethernet

WIZnet Ethernet HAT for Raspberry Pi Pico

A W5100S-based Ethernet HAT. In this project it is used with stacking headers so the Waveshare display HAT can sit above it.

Official documentation and code:

- Official WIZnet documentation: https://docs.wiznet.io/Product/Chip/Chip_Related_modules/wiznet_ethernet_hat
- WIZnet-PICO-C examples/library repo: https://github.com/WIZnet-ioNIC/WIZnet-PICO-C
- WIZnet ioLibrary_Driver: https://github.com/Wiznet/ioLibrary_Driver

### Mechanical / stacking

Raspberry Pi Pico stacking header pair

One pair is soldered to the WIZnet Ethernet HAT.

Correct orientation:

- Female sockets face downward toward the Pico 2 WH.
- Long male pins face upward so the Waveshare display HAT can plug on top.
- No extra riser was needed after test-fitting.

## Setup on Debian-based Linux distro assuming none of the tools are present

These instructions assume a Debian-based Linux system such as Debian, Ubuntu, Raspberry Pi OS, Linux Mint, etc.

The actual tested setup for this project is Debian 13 on the V56 laptop.

### 1. Install base build tools

```bash
sudo apt update
sudo apt install -y git cmake ninja-build build-essential python3 wget pkg-config libusb-1.0-0-dev gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib picotool
```

Verify that `picotool` comes from the distribution package:

```bash
command -v picotool
picotool version
```

Expected path on a normal Debian package install:

```text
/usr/bin/picotool
```

### 2. Install the Raspberry Pi Pico SDK

This project currently expects `PICO_SDK_PATH` to point to a Pico SDK checkout.

Example layout:

```bash
mkdir -p "$HOME/.pico-sdk/sdk"
git clone --recurse-submodules https://github.com/raspberrypi/pico-sdk.git "$HOME/.pico-sdk/sdk/2.2.0"
```

Set the environment variable:

```bash
echo 'export PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.2.0"' >> ~/.bashrc
source ~/.bashrc
```

Check it:

```bash
echo "$PICO_SDK_PATH"
```

Expected example output:

```text
/home/alexl/.pico-sdk/sdk/2.2.0
```

### 3. Clone this project and fetch submodules

If cloning fresh from a remote:

```bash
git clone --recurse-submodules <repo-url>
cd pico_2w_clock
```

If the repo was cloned without submodules, or if working from an existing checkout:

```bash
git submodule update --init --recursive --depth 1
```

The WIZnet repo is currently included as:

```text
external/WIZnet-PICO-C
```

### 4. Configure the project

From the project root:

```bash
cmake -S . -B build -G Ninja -DPICO_BOARD=pico2_w
```

This creates the `build/` directory.

## Code edit, compile, build and flash workflow

### Edit

Open the project in VS Code or your preferred editor:

```bash
cd /mnt/SSD_Storage/RPi_Pico_Projects/pico_2w_clock
code .
```

Important files:

```text
CMakeLists.txt
src/main.c
src/pico_8seg.c
include/pico_8seg.h
external/WIZnet-PICO-C/
```

Current display-driver files:

- `include/pico_8seg.h`
- `src/pico_8seg.c`

Current application entry point:

- `src/main.c`

### Build

After editing code:

```bash
cmake --build build
```

The important output file is:

```text
build/pico_2w_clock.uf2
```

### Flash with picotool

Put the Pico 2 W / Pico 2 WH into BOOTSEL mode, then run:

```bash
picotool load -fx build/pico_2w_clock.uf2
```

Expected successful output resembles:

```text
Family ID 'rp2350-arm-s' can be downloaded in absolute space:
  00000000->02000000
Loading into Flash:   [==============================]  100%

The device was rebooted to start the application.
```

### Alternative flash method: copy UF2 manually

If `picotool` is not available, hold BOOTSEL while plugging the Pico into USB. It should mount as a removable drive.

Check mount path:

```bash
lsblk -o NAME,LABEL,MOUNTPOINTS
```

Example copy command:

```bash
cp build/pico_2w_clock.uf2 /media/$USER/RP2350/
```

The Pico should reboot automatically after the copy completes.

## Replacing a source-built picotool with the Debian package

If `picotool` was previously built and installed manually into `~/.local`, remove that install first:

```bash
if [ -f "$HOME/.local/src/picotool/build/install_manifest.txt" ]; then xargs -r rm -v < "$HOME/.local/src/picotool/build/install_manifest.txt"; fi
rm -rf "$HOME/.local/src/picotool"
hash -r
```

Then install the packaged version:

```bash
sudo apt update
sudo apt install -y picotool
```

Verify:

```bash
hash -r
command -v picotool
picotool version
```

The expected packaged path is:

```text
/usr/bin/picotool
```

## Licensing note

The original code in this project can be licensed under the GNU General Public License version 2.0.

Recommended SPDX choices:

- `GPL-2.0-only` if the project should be strictly GPL version 2 only.
- `GPL-2.0-or-later` if later GPL versions are also acceptable.

Third-party libraries and submodules keep their own licenses. Do not relicense third-party code merely because it is present as a submodule.

Current known dependency/license notes:

- Raspberry Pi Pico SDK: BSD-3-Clause.
- WIZnet ioLibrary_Driver: MIT.
- Mbed TLS: generally dual Apache-2.0 OR GPL-2.0-or-later unless specific files say otherwise.
- Apache-2.0-only code should not be combined into a GPLv2-only work without checking compatibility first.

For this repository, a safe practical approach is:

```text
Project-original source files: GPL-2.0-only or GPL-2.0-or-later
Third-party submodules: retain upstream licenses
```

Practical recommendation: use `GPL-2.0-or-later` unless there is a specific reason to forbid GPLv3 use. It keeps the project GPLv2-compatible while reducing future compatibility friction if more libraries are added later.

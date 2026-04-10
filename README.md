# MAX32664 Universal Firmware Tool

A hardware-agnostic, scriptable firmware update tool for MAX32664 sensor hubs — without requiring Maxim reference boards.

---

## 🚀 Why this exists

Updating firmware on the MAX32664 is unnecessarily difficult.

Official tools:
- Require specific hardware (e.g. MAX32630FTHR)
- Are not easily scriptable (e.g. generic Arduino style code with .h file instead of .msbl)
- Are difficult to integrate into custom workflows

This tool solves that.

> It enables reliable firmware updates on **custom hardware designs** using a clean and developer-friendly interface.

---

## ✨ Key Features

- ✅ Works with **custom boards** (no MAX32630FTHR required)
- ✅ Simple, scriptable interface (ideal for automation)
- ✅ Handles **bootloader transitions (MFIO / RST)**
- ✅ Abstracts low-level I2C flashing protocol
- ✅ Designed for **real-world embedded development workflows**
- ✅ Lightweight and easy to integrate

---

## 🧠 Who is this for

- Embedded engineers working with MAX32664
- Wearable / biosensing developers
- Researchers building custom health devices
- Teams frustrated with official flashing workflows

---

## 🔧 Supported Hardware

- MAX32664C (primary target)
- Compatible with:
  - MAX8614x and MAX86161 based systems
  - Custom I2C host MCUs (nRF52, STM32, etc.)
  - Arduino compatible (!! min flash size 512kB needed!! )

---

## 📦 Installation

Clone the repository:

```bash
git clone https://github.com/Harshonar/Max32664c_firmware_update_tool.git
cd Max32664c_firmware_update_tool
```
Note: Temorary only, update the buffersize to 8448, e.g. ringbuffer.h (nrf52 adafruit arduino BSP)
```
#ifndef SERIAL_BUFFER_SIZE
#define SERIAL_BUFFER_SIZE 8448
#endif
```
This library helps user to update the Max32664c over I2C with the provided .MSBL library derived header file, as the chip shipped with is usually not upto date with the current data sheet versions (30.13.xx+) .
The code is designed to be used for Arduino boards and is tested specifically with nRF52 based boards like Adafruit or Xiao
Expected output when Firmware is not updated (ex. FW: 30.2.2) is as follows:
On serial port: ->
```text
=== MAX32664C firmware loader ===
Current hub FW: 30.2.2
Starting firmware update...
Entering bootloader mode...
Bootloader version: 3.1.7
Bootloader page size: 8192
Firmware pages: 29
Erasing existing application...
Writing firmware pages...
Page 1 / 29
Page 2 / 29
Page 3 / 29
Page 4 / 29
Page 5 / 29
Page 6 / 29
Page 7 / 29
Page 8 / 29
Page 9 / 29
Page 10 / 29
Page 11 / 29
Page 12 / 29
Page 13 / 29
Page 14 / 29
Page 15 / 29
Page 16 / 29
Page 17 / 29
Page 18 / 29
Page 19 / 29
Page 20 / 29
Page 21 / 29
Page 22 / 29
Page 23 / 29
Page 24 / 29
Page 25 / 29
Page 26 / 29
Page 27 / 29
Page 28 / 29
Page 29 / 29
Firmware transfer complete
Firmware after update: 30.13.31
Firmware update completed successfully
Current hub FW: 30.13.31
```

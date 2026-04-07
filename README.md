This library helps user update the .MSBL derived header file based firmware update of the MAX32664c Chip which is usually not upto date with the current data sheet versions.
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

# MrM_MiThermoemeter

Minimal custom firmware for the Xiaomi Thermometer LYWSD03MMC - fork of [atc1441's
custom firmware](https://github.com/atc1441/ATC_MiThermometer/).

## ⚠️ WARNING ️⚠️

### This firmware **DISABLES OTA FLASHING OF THE DEVICE**.

This is deliberate. I don't like having devices that can be remotely
reprogrammed _by anyone_ without my knowledge.

### This firmware **DISABLES OTA SETTINGS CHANGES**.

Same as above. I find no use for changing the display settings on a device as
simple as this one, so to make the FW simpler I removed that functionality.

## Support the original creator

This is the link to the donations atc1441 had in their repo:

> https://paypal.me/hoverboard1

The original firmware a cool piece of hardware reverse engineering. Consider
kicking a buck to atc1441 if you find the code here valuable.

## Changes from the atc1441 version:

Most of the changes are made in the interest of making the FW less cumbersome
to inspect and modify.

- Added BTHome v2 advertisement format.
- Made all settings constant & compile-time.
- Added FW version screen (shown after boot).
- Removed OTA updates.
- Removed RxTx ATT service (settings update OTA).
- Removed the "smiley face" from LCD (cause it's janky).
- Better & cleaner Python flasher.
- BLEMonitor script.
- Codebase cleanup.

## Building the software

All you need `docker` and `make`. Go to the root of the repo, execute `make`
(the command).

If all goes well, the binary will be placed into the `src/` directory.

## Flashing via UART

To flash the firmware using a UART to USB dongle (CP2102 ones should work):

- Make sure the VCC pin of the bridge outputs 3.3 volts - some models can
  switch between 3.3V or 5V by changing a jumper position.
- Connect the UART pins to the board:
  - GND pin to the negative battery terminal.
  - VCC pin to the positive battery terminal.
  - TX pin to the P14 pad on the board.
- Run the flash utility: `python3 flash.py --file src/mrm_mi_fw.bin`


# ESP32-S2_NIXIE

This project build a ESP32-S2 based Wi-Fi connected Nixie clock. 

This repo includes all the stuff needed to build the clock included list of components, schematics and codes.

## Features

- IN12 Nixie direct-drived using SN74141/KD155ID1 IC.
- SPI controlled, daisy-chained shift register to control the digit. 
- Addressable RGB backlight.
- Web-app for configurations.
- USB powered (with external Nixie PSU, **NCH8200HV** ).
- NTP to sync internal RTC time.

### Possible additions

- Backup battery circuit to keep RTC time running (with deep sleep enabled) when unplugged.
- Wifi manager (via AP-mode) for provisioning when default wifi network unavailable.
- API for displaying custom numbers.

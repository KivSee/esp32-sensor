# esp32-motion
An ESP32 project to connect a motion sensor to a KivSee project

## Upload thing_info to filesystem
A `data` folder with a file named `thing_info` containing the `thing_name` for the sensor, up to 16 characters, must be uploaded before the sensor can connect to WIFI and MQTT services. \
The platformio command for the filesystem upload \
`pio run -t uploadfs`

## secrets.h
The secrets.h file needs to be updated with WIFI SSID and password to connect.

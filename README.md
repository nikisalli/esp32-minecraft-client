# esp32-minecraft-client
this is an esp32 based minecraft client!

# INFO
- the currently supported version is 1.16.5 (protocol version 754)
- it may sometimes crash due to the esp32 being quite slow when parsing packets
- this only works for online-mode-off servers
- chunks are completely ignored since parsing chunks implies decompressing quite big frames of data and esp32 is already struggling by just receiving them

# HOW TO USE
rename include/config_edit_me.h to config.h and edit it with your own data, compile, upload using platformio and enjoy!
It should start writing debug info on the serial port and join the server

# SAMPLES
![alt text](https://github.com/nikisalli/esp32-minecraft-client/raw/master/images/example1.jpg)

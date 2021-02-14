# esp32-minecraft-client
an esp32 based minecraft client

# INFO
this only works on minecraft 1.15.2 for now (protocol version 578) and it's really unstable.
the default sketch attempts to connect and then just keeps spinning its head.

# HOW TO USE
rename include/credentials_edit_me.h to credentials.h and edit it with your data, compile and upload.
It should write some debug info on the serial port and access the server.

![alt text](https://github.com/nikisalli/esp32-minecraft-client/blob/features/example1.jpg?raw=true)

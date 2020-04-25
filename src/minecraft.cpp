#include "minecraft.h"
#include <Arduino.h>

// public //

minecraft::minecraft(String _username, String _url, uint16_t _port){
    username = _username;
    server_url = _url;
    server_port = _port;
}

void minecraft::keepAlive(Stream& S, uint64_t id){
    if(compression_enabled){
        writeVarInt(S, 10);
        writeVarInt(S, 0); // empty data length
        writeVarInt(S, 0x0F);
    } else {
        writeVarInt(S, 9);
        writeVarInt(S, 0x0F);
    }
    writeLong(S, id);
}

void minecraft::request(Stream& S){
    writeVarInt(S, 1);
    writeVarInt(S, 0);
}

void minecraft::ping(Stream& S, uint64_t num){
    if(compression_enabled){
        writeVarInt(S, 10);  //packet lenght
        writeVarInt(S, 0);  // empty data length
        writeVarInt(S, 1);  //packet id
    } else {
        writeVarInt(S, 9);  //packet lenght
        writeVarInt(S, 1);  //packet id
    }
    writeLong(S, num);
}

void minecraft::loginStart(Stream& S){
    if(compression_enabled){
        writeVarInt(S, 3 + username.length());
        writeVarInt(S, 0);  // empty data length
        writeVarInt(S, 0);
    } else {
        writeVarInt(S, 2 + username.length());
        writeVarInt(S, 0); 
    }
    writeString(S, username);
}

void minecraft::writeChat(Stream& S, String text){
    if(compression_enabled){
        writeVarInt(S, 3 + text.length());
        writeVarInt(S, 0);  // empty data length
        writeVarInt(S, 3);
    } else {
        writeVarInt(S, 2 + text.length());
        writeVarInt(S, 3); 
    }
    writeString(S, text);
}

void minecraft::handShake(Stream& S, uint8_t state){
    if(compression_enabled){
        writeVarInt(S, 8 + server_url.length());
        writeVarInt(S, 0);  // empty data length
        writeVarInt(S, 0);
    } else {
        writeVarInt(S, 7 + server_url.length());
        writeVarInt(S, 0);
    }
    writeVarInt(S, 578);
    writeString(S, server_url);
    writeUnsignedShort(S, server_port);
    writeVarInt(S, state);
}

void minecraft::clientStatus(Stream& S, uint8_t state){
    if(compression_enabled){
        writeVarInt(S, 3);
        writeVarInt(S, 0);  // empty data length
        writeVarInt(S, 4);
    } else {
        writeVarInt(S, 2);
        writeVarInt(S, 4);
    }
    writeVarInt(S, 0);
}


String minecraft::readString(Stream& S){
    int length = readVarInt(S);
    Serial.println("[INFO] <- text length to read: " + String(length) + " bytes");
    String result;
    for(int i=0; i<length; i++){
        while (S.available() < 1);
        result.concat((char)S.read());
    }
    return result;
}

int minecraft::readVarInt(Stream& S) {
    int numRead = 0;
    int result = 0;
    byte read;
    do {
        while (S.available() < 1);
        read = S.read();
        int value = (read & 0b01111111);
        result |= (value << (7 * numRead));
        numRead++;
        if (numRead > 5) {
            Serial.println("[ERROR] VarInt too big");
        }
    } while ((read & 0b10000000) != 0);
    return result;
}

// private //
int minecraft::VarIntLength(int val) {
    if(val == 0){
        return 1;
    }
    return (int)floor(log(val) / log(128)) + 1;
}

void minecraft::writeVarInt(Stream& S, int16_t value) {
    do {
        byte temp = (byte)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        S.write(temp);
    } while (value != 0);
}

void minecraft::writeVarLong(Stream& S, int64_t value) {
    do {
        byte temp = (byte)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        S.write(temp);
    } while (value != 0);
}

void minecraft::writeString(Stream& S, String str){
    int length = str.length();
    byte buf[length + 1]; 
    str.getBytes(buf, length + 1);
    writeVarInt(S, length);
    for(int i=0; i<length; i++){
        S.write(buf[i]);
    }
}

void minecraft::writeLong(Stream& S, uint64_t num){
    for(int i=7; i>=0; i--){
        S.write((byte)((num >> (i*8)) & 0xff));
    }
}

void minecraft::writeUnsignedShort(Stream& S, uint16_t num){
    S.write((byte)((num >> 8) & 0xff));
    S.write((byte)(num & 0xff));
}

int lsr(int x, int n){
  return (int)((unsigned int)x >> n);
}
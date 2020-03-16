#include "minecraft.h"
#include <Arduino.h>

// public //

minecraft::minecraft(String _username, String _url, int _port){
    username = _username;
    server_url = _url;
    server_port = _port;
}

void minecraft::keepAlive(Stream* S, uint64_t id){
    writeVarInt(S, 9);
    writeVarInt(S, 0x0F);
    writeLong(S, id);
}

void minecraft::request(Stream* S){
    writeVarInt(S, 1);
    writeVarInt(S, 0);
}

void minecraft::ping(Stream* S, uint64_t num){
    writeVarInt(S, 9);  //packet lenght
    writeVarInt(S, 1);  //packet id
    writeLong(S, num);
}

void minecraft::loginStart(Stream* S){
    writeVarInt(S, 2 + username.length());
    writeVarInt(S, 0);
    writeString(S, username);
}

void minecraft::writeChat(Stream* S, String text){
    writeVarInt(S, 2 + text.length());
    writeVarInt(S, 3);
    writeString(S, text);
}

void minecraft::handShake(Stream* S, byte state){
    writeVarInt(S, 23);
    writeVarInt(S, 0);
    writeVarInt(S, 578);
    writeString(S, server_url);
    writeUnsignedShort(S, server_port);
    writeVarInt(S, state);
}


// private //

void writeVarInt(Stream* S, int value) {
    do {
        byte temp = (byte)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        S->write(temp);
    } while (value != 0);
}

void writeVarLong(Stream* S, long value) {
    do {
        byte temp = (byte)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        S->write(temp);
    } while (value != 0);
}

void writeString(Stream* S, String str){
    int length = str.length();
    byte buf[length + 1]; 
    str.getBytes(buf, length + 1);
    writeVarInt(S, length);
    for(int i=0; i<length; i++){
        S->write(buf[i]);
    }
}

void writeLong(Stream* S, uint64_t num){
    for(int i=7; i>=0; i--){
        S->write((byte)((num >> (i*8)) & 0xff));
    }
}

void writeUnsignedShort(Stream* S, uint16_t num){
    S->write((byte)((num >> 8) & 0xff));
    S->write((byte)(num & 0xff));
}

int lsr(int x, int n){
  return (int)((unsigned int)x >> n);
}
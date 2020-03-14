#include "serializers.h"
#include <Arduino.h>

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
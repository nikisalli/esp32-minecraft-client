#include "actions.h"
#include <Arduino.h>
#include "serializers.h"

void keepAlive(Stream* S, uint64_t id){
    writeVarInt(S, 9);
    writeVarInt(S, 0x0F);
    writeLong(S, id);
}

void request(Stream* S){
    writeVarInt(S, 1);
    writeVarInt(S, 0);
}

void ping(Stream* S, uint64_t num){
    writeVarInt(S, 9);  //packet lenght
    writeVarInt(S, 1);  //packet id
    writeLong(S, num);
}

void loginStart(Stream* S, String username){
    writeVarInt(S, 2 + username.length());
    writeVarInt(S, 0);
    writeString(S, username);
}

void writeChat(Stream* S, String text){
    writeVarInt(S, 2 + text.length());
    writeVarInt(S, 3);
    writeString(S, text);
}

void handShake(Stream* S, byte state, String url, int port){
    writeVarInt(S, 23);           // 1+2+1+16+2+1
    writeVarInt(S, 0);            // 1 byte
    writeVarInt(S, 578);          // 2 bytes
    writeString(S, url);
    writeUnsignedShort(S, port);
    writeVarInt(S, state);        // 1 byte
}
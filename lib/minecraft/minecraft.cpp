#include "minecraft.h"
#include <Arduino.h>

// public //

minecraft::minecraft(String _username, String _url, uint16_t _port, Stream* __S){
    username = _username;
    server_url = _url;
    server_port = _port;
    S = __S;
}

void minecraft::teleportConfirm(int id){
    while(writing);
    writing = true;
    if(compression_enabled){
        writeVarInt(2 + VarIntLength(id));
        writeVarInt(0); // empty data length
        writeVarInt(0x00);
    } else {
        writeVarInt(1 + VarIntLength(id));
        writeVarInt(0x00);
    }
    writeVarInt(id);
    writing = false;
}

void minecraft::setRotation(float yaw, float pitch, bool ground){
    while(writing);
    writing = true;
    if(compression_enabled){
        writeVarInt(11);
        writeVarInt(0); // empty data length
        writeVarInt(0x13);
    } else {
        writeVarInt(10);
        writeVarInt(0x13);
    }
    writeFloat(yaw);
    writeFloat(pitch);
    S->write(ground?1:0);
    writing = false;
}

void minecraft::keepAlive(uint64_t id){
    while(writing);
    writing = true;
    if(compression_enabled){
        writeVarInt(10);
        writeVarInt(0); // empty data length
        writeVarInt(0x0F);
    } else {
        writeVarInt(9);
        writeVarInt(0x0F);
    }
    writeLong(id);
    writing = false;
}

void minecraft::request(){
    writeVarInt(1);
    writeVarInt(0);
}

void minecraft::ping(uint64_t num){
    if(compression_enabled){
        writeVarInt(10);  //packet lenght
        writeVarInt(0);  // empty data length
        writeVarInt(1);  //packet id
    } else {
        writeVarInt(9);  //packet lenght
        writeVarInt(1);  //packet id
    }
    writeLong(num);
}

void minecraft::loginStart(){
    if(compression_enabled){
        writeVarInt(3 + username.length());
        writeVarInt(0);  // empty data length
        writeVarInt(0);
    } else {
        writeVarInt(2 + username.length());
        writeVarInt(0); 
    }
    writeString(username);
}

void minecraft::writeChat(String text){
    while(writing);
    writing = true;
    if(compression_enabled){
        writeVarInt(3 + text.length());
        writeVarInt(0);  // empty data length
        writeVarInt(3);
    } else {
        writeVarInt(2 + text.length());
        writeVarInt(3); 
    }
    writeString(text);
    writing = false;
}

void minecraft::handShake(uint8_t state){
    if(compression_enabled){
        writeVarInt(8 + server_url.length());
        writeVarInt(0);  // empty data length
        writeVarInt(0);
    } else {
        writeVarInt(7 + server_url.length());
        writeVarInt(0);
    }
    writeVarInt(578);
    writeString(server_url);
    writeUnsignedShort(server_port);
    writeVarInt(state);
}

void minecraft::clientStatus(uint8_t state){
    if(compression_enabled){
        writeVarInt(3);
        writeVarInt(0);  // empty data length
        writeVarInt(4);
    } else {
        writeVarInt(2);
        writeVarInt(4);
    }
    writeVarInt(0);
}

float minecraft::readFloat(){
    char b[4] = {};
    while(S->available() < 4);
    for(int i=3; i>=0; i--){
        b[i] = S->read();
    }
    float f = 0;
    memcpy(&f, b, sizeof(float));
    return f;
}

double minecraft::readDouble(){
    char b[8] = {};
    while(S->available() < 8);
    for(int i=7; i>=0; i--){
        b[i] = S->read();
    }
    double d = 0;
    memcpy(&d, b, sizeof(double));
    return d;
}

long minecraft::readLong(){
    char b[8] = {};
    while(S->available() < 8);
    for(int i=0; i<8; i++){
        b[i] = S->read();
    }
    long l = ((long) b[0] << 56)
       | ((long) b[1] & 0xff) << 48
       | ((long) b[2] & 0xff) << 40
       | ((long) b[3] & 0xff) << 32
       | ((long) b[4] & 0xff) << 24
       | ((long) b[5] & 0xff) << 16
       | ((long) b[6] & 0xff) << 8
       | ((long) b[7] & 0xff);
    return l;
}

String minecraft::readString(){
    int length = readVarInt();
    Serial.println("[INFO] <- text length to read: " + String(length) + " bytes");
    String result;
    for(int i=0; i<length; i++){
        while (S->available() < 1);
        result.concat((char)S->read());
    }
    return result;
}

int minecraft::readVarInt() {
    int numRead = 0;
    int result = 0;
    byte read;
    do {
        while (S->available() < 1);
        read = S->read();
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

void minecraft::writeDouble(double value){
    unsigned char * p = reinterpret_cast<unsigned char *>(&value);
    for(int i=7; i>=0; i--){
        S->write(p[i]);
    }
}

void minecraft::writeFloat(float value) {
    unsigned char * p = reinterpret_cast<unsigned char *>(&value);
    for(int i=3; i>=0; i--){
        S->write(p[i]);
    }
}

void minecraft::writeVarInt(int16_t value) {
    do {
        byte temp = (byte)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        S->write(temp);
    } while (value != 0);
}

void minecraft::writeVarLong(int64_t value) {
    do {
        byte temp = (byte)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        S->write(temp);
    } while (value != 0);
}

void minecraft::writeString(String str){
    int length = str.length();
    byte buf[length + 1]; 
    str.getBytes(buf, length + 1);
    writeVarInt(length);
    for(int i=0; i<length; i++){
        S->write(buf[i]);
    }
}

void minecraft::writeLong(uint64_t num){
    for(int i=7; i>=0; i--){
        S->write((byte)((num >> (i*8)) & 0xff));
    }
}

void minecraft::writeUnsignedShort(uint16_t num){
    S->write((byte)((num >> 8) & 0xff));
    S->write((byte)(num & 0xff));
}

void minecraft::handle(){
    while(1){
        int pack_length = readVarInt();

        if(compression_enabled){
            int data_length = readVarInt();

            if(data_length > compression_treshold){
                int len = pack_length - VarIntLength(data_length);
                uint8_t* buf;
                buf = new uint8_t[len];

                S->readBytes(buf, len);

                delete [] buf;

                // Serial.println("[INFO] <- packet received! pack_length: " + String(pack_length) + " data_length: " + String(data_length));
            } else {
                int id = readVarInt();

                // Serial.print("[INFO] <- pack len: " + String(pack_length) + "B id: 0x");
                // Serial.println(id, HEX);

                switch (id){
                    case 0x49:{
                        health = readFloat();
                        food = readVarInt();
                        food_sat = readFloat();

                        Serial.println("[INFO] <- player stats health: " + String(health)
                                        + " food: " + String(food)
                                        + " food_sat: " + String(food_sat));
                        
                        break;
                    }
                    case 0x36:{
                        x = readDouble();
                        y = readDouble();
                        z = readDouble();
                        yaw = readFloat();
                        pitch = readFloat();
                        uint8_t flags = S->read();
                        int id = readVarInt();
                        teleported = true;

                        Serial.println("[INFO] <- player pos and look X: " + String(x)
                                        + " Y: " + String(y)
                                        + " Z: " + String(z)
                                        + " yaw: " + String(yaw)
                                        + " pitch: " + String(pitch)
                                        + " flags: " + String(flags)
                                        + " id: " + String(id));

                        teleportConfirm(id);
                        break;
                    }
                    case 0x09:{
                        int ent_id = readVarInt();
                        long pos = readLong();
                        uint8_t stage = S->read();
                        Serial.println("[INFO] <- block destroy id: " + String(ent_id)
                                        + " pos: " + String(pos)
                                        + " stage: " + String(stage));
                        break;
                    }
                    case 0x21:{
                        long num = readLong();
                        keepAlive(num);
                        Serial.println("[INFO] <- received keepalive " + String(num));
                        break;
                    }
                    case 0x0F:{
                        String chat = readString();
                        uint8_t pos = S->read();
                        Serial.println("[INFO] <- received message: " + chat + " sender type: " + String(pos));
                    }
                    default:{
                        for(int i=0; i < pack_length - VarIntLength(id) - VarIntLength(data_length); i++ ){
                            while (S->available() < 1);
                            S->read();
                        }
                        break;
                    }
                }
            } 
        } else {
            int id = readVarInt();

            Serial.print("[INFO] <- Received packet length: " + String(pack_length) + " bytes packet id: 0x");
            Serial.println(id, HEX);

            switch (id){
                case 0x03:{
                    int tres = readVarInt();
                    compression_treshold = tres;
                    compression_enabled = true;

                    Serial.println("[INFO] * Compression treshold set to " + String(tres) + " bytes");

                    break;
                }
                case 0x00:{
                    String str = readString();
                    Serial.println("[INFO] <- text Received: " + str);
                    break;
                }
                default:{
                    for(int i=0; i<pack_length - 1; i++ ){
                        S->read();
                    }
                    break;
                }
            }
        }
    }
}

int lsr(int x, int n){
  return (int)((unsigned int)x >> n);
}
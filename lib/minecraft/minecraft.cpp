#include "minecraft.h"
#include <Arduino.h>

// PACKET
void packet::write(uint8_t val){
    buffer[index] = val;
    index ++;
}

void packet::write(uint8_t * buf, size_t size){
    memcpy(buffer + index, std::move(buf), size);
    index += size;
}

void packet::serverWriteVarInt(int32_t value){
    do {
        uint8_t temp = (uint8_t)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        S->write(temp);
    } while (value != 0);
}

void packet::writePacket(){
    (*mtx).lock();
    if(compression_enabled){ // TODO
        serverWriteVarInt(index + 1);
        serverWriteVarInt(0);
    } else {
        serverWriteVarInt(index);
    }
    S->write(buffer, index);
    (*mtx).unlock();
}

// MINECRAFT
minecraft::minecraft(String _username, String _url, uint16_t _port, Stream* __S){
    username = _username;
    server_url = _url;
    server_port = _port;
    S = __S;
    mtx = new std::mutex();
}

// CLIENTBOUND
void minecraft::readSpawnPlayer(){
    int id = readVarInt();
    uint64_t player_UUID = readUUID();
    double px = readDouble();
    double py = readDouble();
    double pz = readDouble();
    uint8_t yaw = readByte();
    uint8_t pitch = readByte();
    
    login("player id:" + String(id) + " found at X: " + String(px) + " Y: " + String(py) + " Z: " + String(pz));
}

void minecraft::readFoodAndSat(){
    health = readFloat();
    food = readVarInt();
    food_sat = readFloat();
    login("player stats health: " + String(health) + " food: " + String(food) + " food_sat: " + String(food_sat));
}

void minecraft::readPlayerPosAndLook(){
    x = readDouble();
    y = readDouble();
    z = readDouble();
    yaw = readFloat();
    pitch = readFloat();
    uint8_t flags = readByte();
    id = readVarInt();

    login("player pos and look X: " + String(x)
                    + " Y: " + String(y)
                    + " Z: " + String(z)
                    + " yaw: " + String(yaw)
                    + " pitch: " + String(pitch)
                    + " flags: " + String(flags)
                    + " id: " + String(id));

    writeTeleportConfirm(id);
}

void minecraft::readBlockDestroy(){
    int ent_id = readVarInt();
    long pos = readLong();
    uint8_t stage = readByte();
    login("block destroy id: " + String(ent_id)
                    + " pos: " + String(pos)
                    + " stage: " + String(stage));
}

void minecraft::readKeepAlive(){
    uint64_t num = readLong();
    writeKeepAlive(num);
    login("received keepalive");
}

void minecraft::readChat(){
    String chat = readString();
    uint8_t pos = readByte();
    uint64_t player_UUID = readUUID();
    login("received message: " + chat + " sender type: " + String(pos));
}

void minecraft::readSetCompressionThres(){
    int tres = readVarInt();
    compression_treshold = tres;
    compression_enabled = true;
    loginfo("Compression treshold set to " + String(tres) + " bytes");
}

void minecraft::readDisconnected(){
    login("disconnected reason: " + readString());
}

// SERVERBOUND
void minecraft::writeTeleportConfirm(int id){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x00);
    p.writeVarInt(id);
    p.writePacket();
}

void minecraft::writeSetRotation(float yaw, float pitch, bool ground){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x14);
    p.writeFloat(yaw);
    p.writeFloat(pitch);
    p.writeBoolean(ground?1:0);
    p.writePacket();
}

void minecraft::writeKeepAlive(uint64_t id){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x10);
    p.writeLong(id);
    p.writePacket();
    logout("keepalive sent");
}

void minecraft::writeRequest(){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x00);
    p.writePacket();
}

void minecraft::writePing(){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x01);
    p.writeLong(millis());
    p.writePacket();
}

void minecraft::writeLoginStart(){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x00);
    p.writeString(username);
    p.writePacket();
}

void minecraft::writeChat(String text){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x03);
    p.writeString(text);
    p.writePacket();
}

void minecraft::writeHandShake(uint8_t state){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x00);
    p.writeVarInt(754);
    p.writeString(server_url);
    p.writeUnsignedShort(server_port);
    p.writeVarInt(state);
    p.writePacket();
}

void minecraft::writeClientStatus(uint8_t state){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x04);
    p.writeVarInt(state);
    p.writePacket();
}

// READ TYPES
uint16_t minecraft::readUnsignedShort(){
    while(S->available() < 2);
    int ret = S->read();
    return (ret << 8) | S->read();
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

int64_t minecraft::readLong(){
    char b[8] = {};
    while(S->available() < 8);
    for(int i=0; i<8; i++){
        b[i] = S->read();
    }
    uint64_t l = ((uint64_t) b[0] << 56)
       | ((uint64_t) b[1] & 0xff) << 48
       | ((uint64_t) b[2] & 0xff) << 40
       | ((uint64_t) b[3] & 0xff) << 32
       | ((uint64_t) b[4] & 0xff) << 24
       | ((uint64_t) b[5] & 0xff) << 16
       | ((uint64_t) b[6] & 0xff) << 8
       | ((uint64_t) b[7] & 0xff);
    return l;
}

String minecraft::readString(){
    int length = readVarInt();
    String result;
    for(int i=0; i<length; i++){
        while (S->available() < 1);
        result.concat((char)S->read());
    }
    return result;
}

int32_t minecraft::readVarInt() {
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
            logerr("VarInt too big");
        }
    } while ((read & 0b10000000) != 0);
    return result;
}

uint8_t minecraft::readByte(){
    return S->read();
}

bool minecraft::readBool(){
    return S->read();
}

uint64_t minecraft::readUUID(){
    long UUIDmsb = readLong();
    long UUIDlsb = readLong();
    return (((uint64_t)UUIDmsb) << 32) + UUIDlsb;
}

// WRITE TYPES
void packet::writeDouble(double value){
    unsigned char * p = reinterpret_cast<unsigned char *>(&value);
    for(int i=7; i>=0; i--){
        write(p[i]);
    }
}

void packet::writeFloat(float value) {
    unsigned char * p = reinterpret_cast<unsigned char *>(&value);
    for(int i=3; i>=0; i--){
        write(p[i]);
    }
}

void packet::writeVarInt(int32_t value) {
    do {
        uint8_t temp = (uint8_t)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        write(temp);
    } while (value != 0);
}

void packet::writeVarLong(int64_t value) {
    do {
        byte temp = (byte)(value & 0b01111111);
        value = lsr(value,7);
        if (value != 0) {
            temp |= 0b10000000;
        }
        write(temp);
    } while (value != 0);
}

void packet::writeString(String str){
    int length = str.length();
    byte buf[length + 1]; 
    str.getBytes(buf, length + 1);
    writeVarInt(length);
    write(buf, length);
    /*for(int i=0; i<length; i++){
        write(buf[i]);
    }*/
}

void packet::writeLong(int64_t num){
    for(int i=7; i>=0; i--){
        write((uint8_t)((num >> (i*8)) & 0xff));
    }
}

void packet::writeUnsignedLong(uint64_t num){
    for(int i=7; i>=0; i--){
        write((uint8_t)((num >> (i*8)) & 0xff));
    }
}

void packet::writeUnsignedShort(uint16_t num){
    write((byte)((num >> 8) & 0xff));
    write((byte)(num & 0xff));
}

void packet::writeUnsignedByte(uint8_t num){
    write(num);
}

void packet::writeInt(int32_t num){
    write((byte)((num >> 24) & 0xff));
    write((byte)((num >> 16) & 0xff));
    write((byte)((num >> 8) & 0xff));
    write((byte)(num & 0xff));
}

void packet::writeShort(int16_t num){
    write((byte)((num >> 8) & 0xff));
    write((byte)(num & 0xff));
}

void packet::writeByte(int8_t num){
    write(num);
}

void packet::writeBoolean(uint8_t val){
    write(val);
}

void packet::writeUUID(int user_id){
    uint8_t b[15] = {0};
    write(b, 15);
    write(user_id);
}

void minecraft::writeLength(uint32_t length){
    do {
        uint8_t temp = (uint8_t)(length & 0b01111111);
        length = lsr(length,7);
        if (length != 0) {
            temp |= 0b10000000;
        }
        S->write(temp);
    } while (length != 0);
}


void minecraft::handle(){
    int pack_length = readVarInt();
    if(compression_enabled){
        int data_length = readVarInt();
        //login("cpr");
        //login("compr len: " + String(pack_length) + "B data len: " + String(data_length));
        if(data_length != 0){
            int len = pack_length - VarIntLength(data_length);
            S->readBytes(buf, len);
        } else {
            int id = readVarInt();
            //login("id " + String(id, HEX));
            switch (id){
                case 0x04:
                    readSpawnPlayer();
                    break;
                case 0x49:
                    readFoodAndSat();
                    break;
                case 0x34:
                    readPlayerPosAndLook();
                    break;
                case 0x08:
                    readBlockDestroy();
                    break;
                case 0x1F:
                    readKeepAlive();
                    break;
                case 0x0E:
                    readChat();
                    break;
                default:
                    int len = pack_length - VarIntLength(id) - VarIntLength(data_length);
                    S->readBytes(buf, len);
                    break;
            }
        } 
    } else {
        int id = readVarInt();
        login("pack len: " + String(pack_length) + "B id: 0x" + String(id, HEX));

        switch (id){
            case 0x03:
                readSetCompressionThres();
                break;
            case 0x00:
                readDisconnected();
                break;
            default:{
                for(int i=0; i<pack_length - 1; i++ ){
                    S->read();
                }
                break;
            }
        }
    }
}

// UTILITIES
void minecraft::loginfo(String msg){
    Serial.println( "[INFO] " + msg);
}

void minecraft::logerr(String msg){
    Serial.println( "[ERROR] " + msg);
}

void minecraft::login(String msg){
    Serial.println( "[INFO] <- " + msg);
}

void minecraft::logout(String msg){
    Serial.println( "[INFO] -> " + msg);
}

int32_t lsr(int32_t x, uint32_t n){
  return (int32_t)((uint32_t)x >> n);
}

uint32_t minecraft::VarIntLength(int32_t val) {
    if(val == 0){
        return 1;
    }
    return (int)floor(log(val) / log(128)) + 1;
}
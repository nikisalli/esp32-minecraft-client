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
    readUUID();
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

void minecraft::readServerDifficulty(){
    uint8_t diff = readByte();
    bool locked = readBool();
    login("Server difficulty: " + String(diff) + " locked: " + String(locked));
}

void minecraft::readHeldItem(){
    held_item = readByte();
    login("held item: " + String(held_item));
}

void minecraft::readExperience(){
    float bar = readFloat();
    int level = readVarInt();
    experience = readVarInt();
    login("exp bar: " + String(bar) + " level: " + String(level) + " total: " + String(experience));
}

void minecraft::readSpawnPoint(){
    uint64_t val = readUnsignedLong();
    login("world spawn point: " + String((uint32_t)(val >> 38)) + " " + 
                                  String((uint32_t)(val & 0xFFF)) + " " + 
                                  String((uint32_t)(val << 26 >> 38)));
}

void minecraft::readWindowItems(){
    uint8_t windowid = readByte();
    uint32_t elementnum = readShort();
    for(int i = 0; i < elementnum; i++){
        bool present = readByte();
        if(present){
            uint32_t itemid = readVarInt();
            uint32_t itemCount = readByte();
            uint8_t NBT_len = readVarInt();
            login("window id: " + String(windowid) + 
                  " itemid: " + String(itemid) + 
                  " count: " + String(itemCount) + 
                  " at slot: " + String(elementnum));
            if(NBT_len != 0){
                dumpBytes(NBT_len);
            }
        } else {
            continue;
        }
    }
}

void minecraft::readSetSlot(){
    uint8_t windowid = readByte();
    uint32_t elementnum = readShort();
    bool present = readByte();
    if(present){
        uint32_t itemid = readVarInt();
        uint32_t itemCount = readByte();
        uint8_t NBT_len = readVarInt();
        login("window id: " + String(windowid) + 
                " itemid: " + String(itemid) + 
                " count: " + String(itemCount) + 
                " at slot: " + String(elementnum));
        if(NBT_len != 0){
            dumpBytes(NBT_len);
        }
    }
}

void minecraft::readSpawnEntity(){
    if(game_state == 0){ // login success
        readUUID();
        readString();
        game_state = 1; // now the client is in play state
        login("login success");
    } else if(game_state == 1) { // spawn entity
        uint32_t eid = readVarInt();
        readUUID();
        uint32_t etype = readVarInt();
        double ex = readDouble();
        double ey = readDouble();
        double ez = readDouble();
        readByte(); // we don't need heading
        readByte();
        readByte();
        readShort(); // we don't need velocity
        readShort();
        readShort();
        login("entity id: " + String(eid) + " type: " + String(etype) +
              " coords: " + String(ex) + " " + String(ey) + " " + String(ez));
    }
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

void minecraft::writeInteract(uint32_t entityid, uint8_t hand, bool sneaking){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x0E);
    p.writeVarInt(entityid);
    p.writeVarInt(0);
    p.writeVarInt(hand);
    p.writeBoolean(sneaking);
    p.writePacket();
    logout("interact sent");
}

void minecraft::writeInteractAt(uint32_t entityid, uint8_t hand, bool sneaking, uint32_t x, uint32_t y, uint32_t z){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x0E);
    p.writeVarInt(entityid);
    p.writeVarInt(2);
    p.writeFloat(x);
    p.writeFloat(y);
    p.writeFloat(z);
    p.writeVarInt(hand);
    p.writeBoolean(sneaking);
    p.writePacket();
    logout("interact at sent");
}

void minecraft::writeAttack(uint32_t entityid, uint8_t hand, bool sneaking){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x0E);
    p.writeVarInt(entityid);
    p.writeVarInt(1);
    p.writeVarInt(hand);
    p.writeBoolean(sneaking);
    p.writePacket();
    logout("attack sent");
}

// READ TYPES
uint16_t minecraft::readUnsignedShort(){
    while(S->available() < 2);
    int ret = S->read();
    return (ret << 8) | S->read();
}

int16_t minecraft::readShort(){
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

uint64_t minecraft::readUnsignedLong(){
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
    return UUIDlsb;
}

void minecraft::dumpBytes(uint32_t len){
    uint8_t buf[1000];
    S->readBytes(buf, len);
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


void minecraft::readCompressed(){
    int pack_length = readVarInt();
    int data_length = readVarInt();
    //login("cpr");
    if(data_length != 0){
        int len = pack_length - VarIntLength(data_length);
        S->readBytes(buf, len);
        //login("compr len: " + String(pack_length) + "B data len: " + String(data_length));
    } else {
        int id = readVarInt();
        switch (id){
            case 0x04: readSpawnPlayer(); break;
            case 0x49: readFoodAndSat(); break;
            case 0x34: readPlayerPosAndLook(); break;
            case 0x08: readBlockDestroy(); break;
            case 0x1F: readKeepAlive(); break;
            case 0x0E: readChat(); break;
            case 0x0D: readServerDifficulty(); break;
            case 0x3F: readHeldItem(); break;
            case 0x48: readExperience(); break;
            case 0x42: readSpawnPoint(); break;
            case 0x13: readWindowItems(); break;
            case 0x15: readSetSlot(); break;
            case 0x02: readSpawnEntity(); break;
            default:
                for(auto b : blacklisted_packets){
                    if(id == b){
                        goto out;
                    }
                }
                login("id " + String(id, HEX));
                out:
                int len = pack_length - VarIntLength(id) - VarIntLength(data_length);
                S->readBytes(buf, len);
                break;
        }
    }
}

void minecraft::readUncompressed(){
    int pack_length = readVarInt();
    int id = readVarInt();
    switch (id){
        case 0x03: readSetCompressionThres(); break;
        case 0x00: readDisconnected(); break;
        default:{
            login("uncompressed pack len: " + String(pack_length) + "B id: 0x" + String(id, HEX));
            for(int i=0; i<pack_length - 1; i++ ){
                S->read();
            }
            break;
        }
    }
}

void minecraft::handle(){
    if(compression_enabled){
        readCompressed();
    } else {
        readUncompressed();
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
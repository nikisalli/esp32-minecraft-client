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
    last_keepalive = millis();
    login("received keepalive");
}

void minecraft::readChat(){
    String chat = readString();
    uint8_t pos = readByte();
    uint64_t player_UUID = readUUID();
    if(chat_enabled){
        login("received message: " + chat + " sender type: " + String(pos));
    }
    
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
    windowid = readByte();
    uint32_t elementnum = readShort();
    login("windowID: " + String(windowid) + " reading " + String(elementnum) + " window items...");
    for(int i = 0; i < elementnum; i++){
        slot temp = readSlot();
        if(temp.present){
            login(" |-window id: " + String(windowid) + 
                  " itemid: " + String(temp.id) + 
                  " count: " + String(temp.count) + 
                  " at slot: " + String(i));
        
        } else {
            continue;
        }
    }
}

void minecraft::readSetSlot(){
    readByte();
    uint32_t elementnum = readShort();
    slot temp = readSlot();
    if(temp.present){
        login("## window id: " + String(windowid) + 
                " itemid: " + String(temp.id) + 
                " count: " + String(temp.count) + 
                " at slot: " + String(elementnum));
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
        entity_map[eid] = entity{ex, ey, ez, etype};
        //login("entity id: " + String(eid) + " type: " + String(etype) +
        //      " coords: " + String(ex) + " " + String(ey) + " " + String(ez));
    }
}

void minecraft::readDestroyEntity(){
    uint32_t count = readVarInt();
    while(count){
        uint32_t entity_to_destroy = readVarInt();
        std::map<uint32_t, entity>::iterator entity_map_iterator;
        entity_map_iterator = entity_map.find('b');
        if (entity_map_iterator != entity_map.end())
            entity_map.erase (entity_map_iterator);
        count--;
        //login("removed entity id: " + String(entity_to_destroy));
    }
}

void minecraft::readTradeList(){
    windowid = readVarInt();
    uint8_t trade_num = readByte();
    login("windowID: " + String(windowid) + " reading trade list...");
    while(trade_num){
        slot input = readSlot();
        slot output = readSlot();
        slot opt;
        if(readByte()){
            opt = readSlot();
        }
        bool trade_disabled = readByte();
        int32_t trade_uses = readInt();
        int32_t max_trade_uses = readInt();
        int32_t xp = readInt();
        int32_t special_price = readInt();
        float price_multiplier = readFloat();
        int32_t demand = readInt();
        trade_num--;
        login(" |- input_id: " + String(input.id) +
              " output_id: " + String(output.id) +
              " used_trades: " + String(trade_uses) + "/" + String(max_trade_uses));
    }
    uint32_t villager_level = readVarInt();
    uint32_t villager_xp = readVarInt();
    bool regular_villager = readByte();
    bool can_restock = readByte();
    loginfo("### villager_exp: " + String(villager_xp) + " villager_level: " + String(villager_level));
}

void minecraft::readWindowConfirmation(){
    windowid = readByte();
    int16_t action_num = readShort();
    bool accepted = readByte();
    login("CONFIRMATION windowID: " + String(windowid) + " actionID: " + String(action_num) + " accepted: " + String(accepted));
    writeWindowConfirmation(windowid, action_num, accepted);
}

void minecraft::readSpawnObject(){
    if(game_state == 0){ // disconnect
        String reason = readString();
        login("disconnected reason: " + reason);
    } else if(game_state == 1) { // spawn entity
        uint32_t eid = readVarInt();
        readUUID();
        uint32_t etype = readVarInt();
        double ex = readDouble();
        double ey = readDouble();
        double ez = readDouble();
        readByte(); // we don't need heading
        readByte();
        readInt();
        readShort(); // we don't need velocity
        readShort();
        readShort();
        entity_map[eid] = entity{ex, ey, ez, etype};
        //login("object id: " + String(eid) + " type: " + String(etype) +
        //      " coords: " + String(ex) + " " + String(ey) + " " + String(ez));
    }
}

void minecraft::readOpenWindow(){
    windowid = readVarInt();
    uint32_t windowtype = readVarInt();
    String title = readString();
    login("opened window " + title + " id: " + String(windowid) + " type: " + String(windowtype));
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

void minecraft::writeClickWindow(uint8_t window_id, int16_t slot_id, uint8_t button, uint8_t mode, slot item){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x09);
    p.writeByte(window_id);
    p.writeShort(slot_id);
    p.writeByte(button);
    p.writeShort(action_id);
    p.writeVarInt(mode);
    p.writeSlot(item);
    p.writePacket();
    action_id++;
    logout("clicked windowID: " + String(windowid) + " slot id " + String(slot_id));
}

void minecraft::writeWindowConfirmation(uint8_t window_id, int16_t action_num, bool accepted){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x07);
    p.writeByte(window_id);
    p.writeShort(action_num);
    p.writeByte(accepted);
    p.writePacket();
    logout("windowID: " + String(window_id) + " actionID: " + String(action_num) + " confirmation sent");
}

void minecraft::writeCloseWindow(){
    if(windowid != 0){
        packet p(S, mtx, compression_enabled);
        p.writeVarInt(0x0A);
        p.writeByte(windowid);
        p.writePacket();
        logout("windowID: " + String(windowid) + " closed");
        windowid = 0;
    }
}

void minecraft::writeUseItem(uint8_t hand){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x2F);
    p.writeVarInt(hand);
    p.writePacket();
    logout("used item");
}

void minecraft::writePlayerBlockPlace(uint8_t hand, int64_t bx, int64_t by, int64_t bz, uint32_t face, float cx, float cy, float cz, bool inside){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x2E);
    p.writeVarInt(hand);
    uint64_t kek = ((bx & 0x3FFFFFF) << 38) | ((bz & 0x3FFFFFF) << 12) | (by & 0xFFF);
    p.writeUnsignedLong(kek);
    for(int i = 0; i < 8; i++){
        Serial.print((kek >> (7 - i) * 8) & 0xFF, BIN);
        Serial.print(" ");
    }
    Serial.println();
    p.writeVarInt(face);
    p.writeFloat(cx);
    p.writeFloat(cy);
    p.writeFloat(cz);
    p.writeByte(inside);
    p.writePacket();
    logout("block placed");
}

void minecraft::writeClientSettings(uint8_t view_distance, uint8_t chat_mode, bool chat_colors, uint8_t skin_parts, uint8_t main_hand){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x05);
    p.writeString("en_GB");
    p.writeByte(view_distance);
    p.writeVarInt(chat_mode);
    p.writeByte(chat_colors);
    p.writeByte(skin_parts);
    p.writeVarInt(main_hand);
    p.writePacket();
    logout("client settings packet");
}

void minecraft::writeRequestRecipe(uint8_t window_id, String recipe, bool all){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x19);
    p.writeByte(window_id);
    p.writeString(recipe);
    p.writeByte(all);
    p.writePacket();
    logout("recipe sent: " + recipe);
}

void minecraft::writeSelectTrade(uint32_t slotid){
    packet p(S, mtx, compression_enabled);
    p.writeVarInt(0x23);
    p.writeVarInt(slotid);
    p.writePacket();
    logout("selected trade: " + String(slotid));
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
    S->readBytes(b, 8);
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
    S->readBytes(b, 8);
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

slot minecraft::readSlot(){
    slot ret;
    ret.present = readByte();
    if(ret.present){
        ret.id = readVarInt();
        ret.count = readByte();
        readNBT();
    }
    return ret;
}

String minecraft::readNBTString(){
    int length = readUnsignedShort();
    String result;
    while(length){
        while (S->available() < 1);
        result.concat((char)S->read());
        length--;
    }
    return result;
}

void minecraft::readNBTCompound(){
    Serial.println("COMPOUND");
    uint8_t level = 1;
    while(level > 0){
        uint8_t type = readByte();
        switch(type){
            case 0x00: // END nameless
                Serial.print("END");
                level--;
                break;
            case 0x01: // BYTE
                Serial.print("BYTE: " + readNBTString());
                readByte();
                break;
            case 0x02: // SHORT
                Serial.print("SHORT: " + readNBTString());
                readShort();
                break;
            case 0x03: // INT
                Serial.print("INT: " + readNBTString());
                readInt();
                break;
            case 0x04: // LONG
                Serial.print("LONG: " + readNBTString());
                readLong();
                break;
            case 0x05: // FLOAT
                Serial.print("FLOAT: " + readNBTString());
                readFloat();
                break;
            case 0x06: // DOUBLE
                Serial.print("DOUBLE: " + readNBTString());
                readDouble();
                break;
            case 0x07:{// BYTE ARRAY
                    Serial.print("BYTE ARRAY: " + readNBTString());
                    readNBTString();
                    uint32_t num = readInt(); // array length
                    while(num){
                        readByte();
                        num--;
                    }
                    break;
                }
            case 0x08: // STRING
                Serial.print("STRING: " + readNBTString());
                readNBTString();
                break;
            case 0x09:{ // LIST
                    readNBTList();
                    break;
                }
            case 0x0A: // COMPOUND
                readNBTCompound();
                level++;
                break;
            case 0x0B:{ // INT ARRAY
                    Serial.print("INT ARRAY: " + readNBTString());
                    uint32_t num = readInt(); // array length
                    while(num){
                        readInt();
                        num--;
                    }
                    break;
                }
            case 0x0C:{ // LONG ARRAY
                    Serial.print("LONG ARRAY: " + readNBTString());
                    uint32_t num = readInt(); // array length
                    while(num){
                        readLong();
                        num--;
                    }
                    break;
                }
            default:
                Serial.print("UNSUPPORTED " + String(type, HEX));
                break;
        }
        Serial.println();
    }
}

void minecraft::readNBTList(){
    Serial.print("LIST: " + readNBTString());
    uint8_t ltype = readByte();
    uint8_t num = readInt();
    Serial.println(" type: " + String(ltype, HEX) + " len: " + String(num));
    while(num){
        switch(ltype){
            case 0x01: /*Serial.print("BYTE");*/ readByte();break;
            case 0x02: /*Serial.print("SHORT");*/ readShort(); break;
            case 0x03: /*Serial.print("INT");*/ readInt(); break;
            case 0x04: /*Serial.print("LONG");*/ readLong(); break;
            case 0x05: /*Serial.print("FLOAT");*/ readFloat(); break;
            case 0x06: /*Serial.print("DOUBLE");*/ readDouble(); break;
            case 0x08: /*Serial.print("STRING");*/ readNBTString(); break;
            case 0x09: /*Serial.println("LIST");*/ readNBTList(); break;
            case 0x0A: /*Serial.println("COMPOUND");*/ readNBTCompound(); break;
        }
        num--;

        //Serial.println();
    }
}

void minecraft::readNBT(){
    //loginfo("reading NBT");
    if(readByte() == 0x0A){
        readNBTString();
        //Serial.println("COMPOUND: " + readNBTString());
        readNBTCompound();
    } else {
        //Serial.println("END");
    }
    
}

int32_t minecraft::readInt(){
    char b[4] = {};
    S->readBytes(b, 4);
    uint32_t l = ((uint32_t) b[0] << 24)
       | ((uint32_t) b[1] & 0xff) << 16
       | ((uint32_t) b[2] & 0xff) << 8
       | ((uint32_t) b[3] & 0xff);
    return l;
}

uint32_t minecraft::readUnsignedInt(){
    char b[4] = {};
    S->readBytes(b, 4);
    uint32_t l = ((uint32_t) b[0] << 24)
       | ((uint32_t) b[1] & 0xff) << 16
       | ((uint32_t) b[2] & 0xff) << 8
       | ((uint32_t) b[3] & 0xff);
    return l;
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

void packet::writeSlot(slot item){
    writeByte(item.present);
    if(item.present){
        writeVarInt(item.id);
        writeByte(item.count);
        writeByte(0); // no NBT data
    }
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
            case 0x36: readDestroyEntity(); break;
            case 0x26: readTradeList(); break;
            case 0x11: readWindowConfirmation(); break;
            case 0x19: readDisconnected(); break;
            case 0x00: readSpawnObject(); break;
            case 0x2D: readOpenWindow(); break;
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
            //login("uncompressed pack len: " + String(pack_length) + "B id: 0x" + String(id, HEX));
            for(int i=0; i<pack_length - 1; i++ ){
                S->read();
            }
            break;
        }
    }
}

void minecraft::handle(){
    packet_count++;
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
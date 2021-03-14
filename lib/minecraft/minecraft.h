#ifndef MINECRAFT_H
#define MINECRAFT_H

#include <Arduino.h>
#include <mutex>
#include <map>
//#include "rom/miniz.h"

struct entity{
    float x;
    float y;
    float z;
    int32_t type;
};

struct slot{
    bool present;
    uint32_t id;
    uint8_t count;
};

class packet{
    public:
    uint8_t buffer[256];
    uint32_t index = 0;
    bool compression_enabled;
    Stream* S;
    std::mutex * mtx;

    packet(Stream* __S, std::mutex * _mtx, bool _compression_enabled){
        S = __S;
        compression_enabled = _compression_enabled;
        mtx = _mtx;
    }

    void write(uint8_t val);
    void write(uint8_t * buf, size_t size);
    void writePacket();

    // write to buf
    void writeDouble        (double value);
    void writeFloat         (float value);
    void writeVarInt        (int32_t value);
    void writeVarLong       (int64_t value);
    void writeString        (String str);
    void writeUnsignedLong  (uint64_t num);
    void writeUnsignedShort (uint16_t num);
    void writeUnsignedByte  (uint8_t num);
    void writeLong          (int64_t num);
    void writeInt           (int32_t num);
    void writeShort         (int16_t num);
    void writeByte          (int8_t num);
    void writeBoolean       (uint8_t val);
    void writeUUID          (int user_id);
    void writeSlot          (slot item);

    // write to server
    void serverWriteVarInt  (int32_t value);
};

class minecraft{
    public:
    minecraft(String _username, String _url, uint16_t _port, Stream* __S);

    String username;
    String server_url;
    uint32_t server_port;
    std::mutex * mtx;
    Stream* S;
    std::map<uint32_t, entity> entity_map;

    uint8_t game_state = 0; // 0: login 1: play
    int id;
    bool compression_enabled = 0;
    int compression_treshold = 0;
    bool writing = 0;
    double x = 0;
    double y = 0;
    double z = 0;
    float yaw = 0;
    float pitch = 0;
    int yaw_i = 0;
    int pitch_i = 0;
    bool onGround = true;
    float health = 0;
    int food = 0;
    float food_sat = 0;
    uint8_t buf[50000];
    uint8_t held_item = 0;
    uint32_t experience = 0;
    uint8_t windowid = 0;
    uint32_t packet_count = 0;
    bool chat_enabled = true;
    uint32_t last_keepalive = 0;

    // these won't show in logs
    uint8_t blacklisted_packets[29] = {0x55, 0x17, 0x30, 0x1A, 0x35, 0x32, 0x40, 0x3A, 0x27, 0x44,
                                       0x46, 0x28, 0x56, 0x58, 0x4E, 0x36, 0x47, 0x00, 0x29, 0x0b,
                                       0x21, 0x4B, 0x3D, 0x51, 0x3B, 0x22, 0x05, 0x1c, 0x0A};

    int timeout = 100;

    void handle();
    void readUncompressed();
    void readCompressed();

    // clientbound
    void readSpawnPlayer         ();
    void readFoodAndSat          ();
    void readPlayerPosAndLook    ();
    void readBlockDestroy        ();
    void readKeepAlive           ();
    void readChat                ();
    void readSetCompressionThres ();
    void readDisconnected        ();
    void readServerDifficulty    ();
    void readHeldItem            ();
    void readExperience          ();
    void readSpawnPoint          ();
    void readWindowItems         ();
    void readSetSlot             ();
    void readSpawnEntity         ();
    void readDestroyEntity       ();
    void readTradeList           ();
    void readWindowConfirmation  ();
    void readSpawnObject         ();
    void readOpenWindow          ();

    // serverbound
    void writeHandle             ();  // this stream is the logging port not the web socket!!
    void writeTeleportConfirm    (int id);
    void writeSetRotation        (float yaw, float pitch, bool ground);
    void writeKeepAlive          (uint64_t id);
    void writeRequest            ();
    void writePing               ();
    void writeLoginStart         ();
    void writeChat               (String text);
    void writeHandShake          (uint8_t state);
    void writeClientStatus       (uint8_t state);
    void writeInteract           (uint32_t entityid, uint8_t hand, bool sneaking);
    void writeInteractAt         (uint32_t entityid, uint8_t hand, bool sneaking, uint32_t x, uint32_t y, uint32_t z);
    void writeAttack             (uint32_t entityid, uint8_t hand, bool sneaking);
    void writeClickWindow        (uint8_t window_id, int16_t slot_id, uint8_t button, int16_t action_id, uint8_t mode, slot item);
    void writeWindowConfirmation (uint8_t window_id, int16_t action_num, bool accepted);
    void writeCloseWindow        ();
    void writeUseItem            (uint8_t hand);
    void writePlayerBlockPlace   (uint8_t hand, int64_t bx, int64_t by, int64_t bz, uint32_t face, float cx, float cy, float cz, bool inside);
    void writeClientSettings     (uint8_t view_distance, uint8_t chat_mode, bool chat_colors, uint8_t skin_parts, uint8_t main_hand);

    void loginfo            (String msg);
    void logerr             (String msg);
    void login              (String msg);
    void logout             (String msg);

    float readFloat         ();
    double readDouble       ();
    int32_t readVarInt      ();
    String readString       ();
    int64_t readLong        ();
    uint64_t readUnsignedLong();
    int16_t readShort       ();
    uint16_t readUnsignedShort();
    int32_t readInt         ();
    uint32_t readUnsignedInt();
    uint32_t VarIntLength   (int32_t val);
    uint8_t readByte        ();
    bool readBool           ();
    uint64_t readUUID       ();
    void dumpBytes          (uint32_t len);
    slot readSlot           ();
    void readNBT            ();
    String readNBTString    ();
    void readNBTList        ();
    void readNBTCompound    ();

    void writeLength        (uint32_t length);
};

int32_t lsr(int32_t x, uint32_t n);
float fmap(float x, float in_min, float in_max, float out_min, float out_max);

#endif
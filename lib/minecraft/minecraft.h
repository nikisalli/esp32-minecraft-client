#ifndef MINECRAFT_H
#define MINECRAFT_H

#include <Arduino.h>
#include <mutex>

class packet{
    public:
    uint8_t buffer[6000];
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
    
    int timeout = 100;

    void handle();

    // clientbound
    void readSpawnPlayer         ();
    void readFoodAndSat          ();
    void readPlayerPosAndLook    ();
    void readBlockDestroy        ();
    void readKeepAlive           ();
    void readChat                ();
    void readSetCompressionThres ();
    void readDisconnected        ();

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

    void loginfo            (String msg);
    void logerr             (String msg);
    void login              (String msg);
    void logout             (String msg);

    float readFloat         ();
    double readDouble       ();
    int32_t readVarInt      ();
    String readString       ();
    int64_t readLong        ();
    uint16_t readUnsignedShort();
    uint32_t VarIntLength   (int32_t val);
    uint8_t readByte        ();
    bool readBool           ();
    uint64_t readUUID       ();

    void writeLength        (uint32_t length);
};

int32_t lsr(int32_t x, uint32_t n);
float fmap(float x, float in_min, float in_max, float out_min, float out_max);

#endif
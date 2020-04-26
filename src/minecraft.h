#ifndef MINECRAFT_H
#define MINECRAFT_H

#include <Arduino.h>

class minecraft{
    String username;
    String server_url;
    int server_port;
    

    public:
        minecraft(String _username, String _url, uint16_t _port);

        bool compression_enabled = 0;
        int compression_treshold = 0;
        bool writing = 0;
        double x = 0;
        double y = 0;
        double z = 0;
        float yaw = 0;
        float pitch = 0;
        bool onGround = true;
        bool teleported = false;
        float health = 0;
        int food = 0;
        float food_sat = 0;
        
        void teleportConfirm    (Stream& S, int id);
        void setRotation        (Stream& S, float yaw, float pitch, bool ground);
        void keepAlive          (Stream& S, uint64_t id);
        void request            (Stream& S);
        void ping               (Stream& S, uint64_t num);
        void loginStart         (Stream& S);
        void writeChat          (Stream& S, String text);
        void handShake          (Stream& S, uint8_t state);
        void clientStatus       (Stream& S, uint8_t state);

        float readFloat         (Stream& S);
        double readDouble       (Stream& S);
        int readVarInt          (Stream& S);
        String readString       (Stream& S);
        long readLong           (Stream& S);
        int VarIntLength        (int val);
    private:
        void writeDouble        (Stream& S, double value);
        void writeFloat         (Stream& S, float value);
        void writeVarInt        (Stream& S, int16_t value);
        void writeVarLong       (Stream& S, int64_t value);
        void writeString        (Stream& S, String str);
        void writeLong          (Stream& S, uint64_t num);
        void writeUnsignedShort (Stream& S, uint16_t num);
};

int lsr(int x, int n);

#endif
#ifndef MINECRAFT_H
#define MINECRAFT_H

#include <Arduino.h>

class minecraft{
    String username;
    String server_url;
    int server_port;
    

    public:
        minecraft(String _username, String _url, uint16_t _port, Stream* __S);

        Stream* S;
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
        
        int timeout = 100;

        void handle             ();  // this stream is the logging port not the web socket!!
        void teleportConfirm    (int id);
        void setRotation        (float yaw, float pitch, bool ground);
        void keepAlive          (uint64_t id);
        void request            ();
        void ping               (uint64_t num);
        void loginStart         ();
        void writeChat          (String text);
        void handShake          (uint8_t state);
        void clientStatus       (uint8_t state);

        float readFloat         ();
        double readDouble       ();
        int readVarInt          ();
        String readString       ();
        long readLong           ();
        int VarIntLength        (int val);
    private:
        void writeDouble        (double value);
        void writeFloat         (float value);
        void writeVarInt        (int16_t value);
        void writeVarLong       (int64_t value);
        void writeString        (String str);
        void writeLong          (uint64_t num);
        void writeUnsignedShort (uint16_t num);
};

int lsr(int x, int n);

#endif
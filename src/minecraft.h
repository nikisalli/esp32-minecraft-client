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

        void keepAlive          (Stream& S, uint64_t id);
        void request            (Stream& S);
        void ping               (Stream& S, uint64_t num);
        void loginStart         (Stream& S);
        void writeChat          (Stream& S, String text);
        void handShake          (Stream& S, uint8_t state);
        void clientStatus       (Stream& S, uint8_t state);
        int readVarInt          (Stream& S);
        String readString       (Stream& S);
        int VarIntLength        (int val);
    private:
        void writeVarInt        (Stream& S, int16_t value);
        void writeVarLong       (Stream& S, int64_t value);
        void writeString        (Stream& S, String str);
        void writeLong          (Stream& S, uint64_t num);
        void writeUnsignedShort (Stream& S, uint16_t num);
};

int lsr(int x, int n);

#endif
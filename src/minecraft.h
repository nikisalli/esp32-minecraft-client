#ifndef minecraft_h
#define minecraft_h

#include <Arduino.h>

class minecraft{
    String username;
    String server_url;
    int server_port;

    public:
        minecraft(String, String, uint16_t);

        void keepAlive          (Stream& S, const uint64_t id);
        void request            (Stream& S);
        void ping               (Stream& S, const uint64_t num);
        void loginStart         (Stream& S);
        void writeChat          (Stream& S, const String text);
        void handShake          (Stream& S, const uint8_t state);
    
    private:
        void writeVarInt        (Stream& S, const int16_t value);
        void writeVarLong       (Stream& S, const int64_t value);
        void writeString        (Stream& S, const String str);
        void writeLong          (Stream& S, const uint64_t num);
        void writeUnsignedShort (Stream& S, const uint16_t num);
};



#endif
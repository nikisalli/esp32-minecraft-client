#ifndef minecraft_h
#define minecraft_h

#include <Arduino.h>

class minecraft{
    String username;
    String server_url;
    int server_port;

    public:
        minecraft(String, String, int);

        void keepAlive          (Stream* S, uint64_t id);
        void request            (Stream* S);
        void ping               (Stream* S, uint64_t num);
        void loginStart         (Stream* S);
        void writeChat          (Stream* S, String text);
        void handShake          (Stream* S, byte state);
    
    private:
        void writeVarInt        (Stream* S, int value);
        void writeVarLong       (Stream* S, long value);
        void writeString        (Stream* S, String str);
        void writeLong          (Stream* S, uint64_t num);
        void writeUnsignedShort (Stream* S, uint16_t num);
};



#endif
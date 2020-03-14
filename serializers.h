#ifndef serializers.h
#define serializers.h

void writeVarInt        (Stream* S, int value);
void writeVarLong       (Stream* S, long value);
void writeString        (Stream* S, String str);
void writeLong          (Stream* S, uint64_t num);
void writeUnsignedShort (Stream* S, uint16_t num);

int lsr(int x, int n);

#endif
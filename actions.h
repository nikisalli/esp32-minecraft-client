#ifndef actions_h
#define actions_h

void keepAlive  (Stream* S, uint64_t id);
void request    (Stream* S);
void ping       (Stream* S, uint64_t num);
void loginStart (Stream* S, String username);
void writeChat  (Stream* S, String text);
void handShake  (Stream* S, byte state, String url, int port);

#endif
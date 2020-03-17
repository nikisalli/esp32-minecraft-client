#include <WiFi.h>
#include <minecraft.h>
//#include "/home/nik/Desktop/roba esp32/esp32-minecraft-client/src/minecraft.h"

String username = "username";
String server_url = "url";
int server_port = 25565;

const char* ssid = "ssid";
const char* password =  "password";

minecraft mc (username, server_url, server_port);

void setup() {
  Serial.begin(115200);
  delay(100);
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());

  delay(1000);
}

void loop(){
    WiFiClient client;
    minecraft mc (username, server_url, server_port);

    const char* buf;
    buf = server_url.c_str();
    if (!client.connect(buf, server_port)) {
        Serial.println("Connection to host failed");
        delay(1000);
        return;
    }

    mc.handShake(client, 2);
    //mc.ping(client, 1);
    mc.loginStart(client);
    delay(100);
    //mc.writeChat(client, "test");

    while(1){
        if(client.available() > 0){
            while(client.available() > 0){
                Serial.print(client.read(), HEX);
                Serial.print(" ");
            }
            Serial.println();
        }
    }
    /*while(1){                        //keep alive work in progress (needs compression)
        if(client.available() > 0)
        while(client.available() > 0){
            if(client.read() == 0x09){
                if(client.read() == 0x21){
                    uint64_t temp;
                    Serial.println("keep alive received!");
                    for(int i=7; i>=0; i--){
                        temp += (client.read() << (i*8));
                        Serial.print((int)(temp/2^32));
                    }
                    mc.keepAlive(&client, temp);
                }
            }
        }
    }*/
}
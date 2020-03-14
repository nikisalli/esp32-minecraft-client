#include <WiFi.h>
#include "actions.h"

#define SERVER_ADDRESS "server_url"
#define PORT 25565

const char* ssid = "ssid";
const char* password =  "password";

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
    
    if (!client.connect(SERVER_ADDRESS, PORT)) {
        Serial.println("Connection to host failed");
        delay(1000);
        return;
    }

    handShake(&client, 2, SERVER_ADDRESS, PORT);
    //ping(&client, 1);
    loginStart(&client, "username");
    delay(100);
    //writeChat(&client, "test");

    while(1){
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
                    keepAlive(&client, temp);
                }
            }
        }
    }
}
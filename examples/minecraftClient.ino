#include <WiFi.h>
#include <minecraft.h>
#include "/home/nik/Desktop/roba esp32/esp32-minecraft-client/src/miniz.h"
//#include "/home/nik/Desktop/roba esp32/esp32-minecraft-client/src/minecraft.h"

String username = "nikbot";
String server_url = "mc.ignuranza.net";
int server_port = 25565;

const char* ssid = "Vodafone-A48216342";
const char* password =  "12344321";

TaskHandle_t listener;
minecraft mc (username, server_url, server_port);
WiFiClient client;

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

    xTaskCreatePinnedToCore(listener_fun, "Task1", 10000, NULL, 0, &listener, 0);

    delay(5000);
}

void loop(){
    const char* buf;
    buf = server_url.c_str();

    if (!client.connect(buf, server_port)) {
        Serial.println("Connection to host failed");
        delay(1000);
        return;
    }

    mc.handShake(client, 2);
    Serial.println("[INFO] -> handShake packet sent");
    delay(2000);

    //mc.ping(client, 1);

    mc.loginStart(client);
    Serial.println("[INFO] -> login_start packet sent");
    delay(2000);

    /*mc.clientStatus(client, 0);
    Serial.println("[INFO] -> client_status packet sent");
    delay(2000);*/

    mc.writeChat(client, "/login nikk");
    Serial.println("[INFO] -> logging in as nikbot");
    delay(200);

    /*mc.writeChat(client, "test");
    Serial.println("[INFO] -> writing to chat");
    delay(200);*/

    while(1){
        delay(1000);
    }
}

void listener_fun( void * parameter) {
    while(1){
        int pack_length = mc.readVarInt(client);

        if(mc.compression_enabled && pack_length > mc.compression_treshold){
            int data_length = mc.readVarInt(client);

            for(int i=0; i<pack_length - mc.VarIntLength(data_length); i++){
                client.read();
            }

            Serial.println("[INFO] <- packet received! pack_lenght: " + String(pack_length) + " data_length: " + String(data_length));
        } 
            
        else if (mc.compression_enabled && pack_length < mc.compression_treshold) {
            mc.readVarInt(client); // unused because always zero
            int id = mc.readVarInt(client);

            Serial.print("[INFO] <- Received packet length: " + String(pack_length) + " bytes packet id: 0x");
            Serial.println(id, HEX);

            switch (id){
                case 0x00:{
                    String str = mc.readString(client);
                    Serial.println("[INFO] <- text Received: " + str);
                    break;
                }
                default:{
                    for(int i=0; i<pack_length - 2; i++ ){
                        client.read();
                    }
                    break;
                }
            }
        } 
            
        else if (!mc.compression_enabled) {
            int id = mc.readVarInt(client);

            Serial.print("[INFO] <- Received packet length: " + String(pack_length) + " bytes packet id: 0x");
            Serial.println(id, HEX);

            switch (id){
                case 0x03:{
                    int tres = mc.readVarInt(client);
                    mc.compression_treshold = tres;
                    mc.compression_enabled = true;

                    Serial.println("[INFO] * Compression treshold set to " + String(tres) + " bytes");

                    break;
                }
                case 0x00:{
                    String str = mc.readString(client);
                    Serial.println("[INFO] <- text Received: " + str);
                    break;
                }
                default:{
                    for(int i=0; i<pack_length - 1; i++ ){
                        client.read();
                    }
                    break;
                }
            }
        }
    }
}
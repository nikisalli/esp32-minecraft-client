#include <WiFi.h>
#include <minecraft.h>
#include "/home/nik/Desktop/roba esp32/esp32-minecraft-client/src/miniz.h"
//#include "/home/nik/Desktop/roba esp32/esp32-minecraft-client/src/minecraft.h"

String username = "nikbot";
String server_url = "192.168.1.12";
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

    delay(1000);

    const char* buf;
    buf = server_url.c_str();

    if (!client.connect(buf, server_port)) {
        Serial.println("Connection to host failed");
        delay(1000);
        return;
    }
    delay(500);
    xTaskCreatePinnedToCore(listener_fun, "listener", 100000, NULL, 1, &listener, 0);
 
    mc.handShake(client, 2);
    Serial.println("[INFO] -> handShake packet sent");
    vTaskDelay(pdMS_TO_TICKS(1000));
    //mc.ping(client, 1);

    mc.loginStart(client);
    Serial.println("[INFO] -> login_start packet sent");
    vTaskDelay(pdMS_TO_TICKS(2000));

    /*mc.clientStatus(client, 0);
    Serial.println("[INFO] -> client_status packet sent");
    delay(2000);*/

    /*mc.writeChat(client, "/login nikk");
    Serial.println("[INFO] -> logging in as nikbot");
    delay(200);*/

    mc.writeChat(client, "test");
    Serial.println("[INFO] -> writing to chat");
    vTaskDelay(pdMS_TO_TICKS(1000));
    disableCore0WDT();
    disableCore1WDT();
}

void loop(){
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}

void listener_fun( void * parameter ){
    while(1){
        int pack_length = mc.readVarInt(client);

        if(mc.compression_enabled && pack_length > mc.compression_treshold){
            int data_length = mc.readVarInt(client);

            for(int i=0; i<pack_length - mc.VarIntLength(data_length); i++){
                while (client.available() < 1);
                client.read();
            }

            Serial.println("[INFO] <- packet received! pack_lenght: " + String(pack_length) + " data_length: " + String(data_length));
        } 
            
        else if (mc.compression_enabled && pack_length < mc.compression_treshold) {
            int data_length = mc.readVarInt(client); // unused because always zero
            int id = mc.readVarInt(client);

            Serial.print("[INFO] <- Received packet length: " + String(pack_length) + " bytes packet id: 0x");
            Serial.print(id, HEX);
            Serial.print(" data length: " + String(data_length));

            switch (id){
                /*case 0x00:{
                    String str = mc.readString(client);
                    Serial.println("[INFO] <- text Received: " + str);
                    break;
                }*/
                case 0x1C:{
                    for(int i=0; i < pack_length - mc.VarIntLength(id) - mc.VarIntLength(data_length); i++ ){
                        while (client.available() < 1);
                        client.read();
                        //Serial.print(" ");
                        //Serial.print(client.read(), HEX);
                    }
                    Serial.println();
                    break;
                }
                default:{
                    for(int i=0; i < pack_length - mc.VarIntLength(id) - mc.VarIntLength(data_length); i++ ){
                        while (client.available() < 1);
                        client.read();
                    }
                    Serial.println();
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
        Serial.println("[INFO] # bytes in buf: " + String(client.available()) + " bytes");
    }
}
#include <Arduino.h>
#include <WiFi.h>
#include <minecraft.h>
#include <config.h>
//#include "../src/miniz.h"

TaskHandle_t listener;
WiFiClient client;
minecraft mc (username, server_url, server_port, &client);

void update( void * pvParameters ){
    mc.handle();
}

void setup() {
    disableCore0WDT();
    disableCore1WDT();
    disableLoopWDT();

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

    client.setNoDelay(true);
    client.setTimeout(2);

    xTaskCreatePinnedToCore(update, "listener", 100000, NULL, 2, &listener, 0);

    delay(1000);
    mc.handShake(2);
    Serial.println("[INFO] -> handShake packet sent");
    delay(1000);
    //mc.ping(client, 1);

    mc.loginStart();
    Serial.println("[INFO] -> login_start packet sent");
    delay(1000);

    //mc.clientStatus(0);
    //Serial.println("[INFO] -> client_status packet sent");
    //delay(100);

    /*mc.writeChat("/login nikk");
    Serial.println("[INFO] -> logging in as nikbot");
    delay(200);*/

    mc.writeChat("test");
    Serial.println("[INFO] -> writing to chat");
    // vTaskDelay(pdMS_TO_TICKS(1000));
}

void loop(){
    mc.yaw+=5;
    if(mc.yaw > 360){
        mc.yaw = 0;
    }
    mc.setRotation(mc.yaw, mc.pitch, mc.onGround);
    delay(50);
    Serial.println("[INFO] # buf: " + String(client.available()/1024.0) + "kB Heap: " + String(ESP.getFreeHeap()/1024.0) + "kB");
    //mc.writeChat(client, "[nik INFO] Heap: " + String(ESP.getFreeHeap()/1024.0) + "kB || buf: " + String(client.available()/1024.0) + "kB");
}

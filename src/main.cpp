#include <Arduino.h>
#include <WiFi.h>
#include <minecraft.h>
#include <config.h>

TaskHandle_t listener;
WiFiClient client;
minecraft mc(username, server_url, server_port, &client);

void update( void * pvParameters ){
    while(true){
        mc.handle();
    }
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

    const char* buf;
    buf = server_url.c_str();

    if (!client.connect(buf, server_port)) {
        Serial.println("Connection to host failed");
        delay(1000);
        return;
    }

    xTaskCreatePinnedToCore(update, "listener", 100000, NULL, 2, &listener, 0);

    mc.writeHandShake(2);
    vTaskDelay(pdMS_TO_TICKS(500));
    mc.writeLoginStart();
    vTaskDelay(pdMS_TO_TICKS(500));

    mc.writeChat("test");
    Serial.println("[INFO] -> writing to chat");
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void loop(){
    if(mc.health < 0.01){
        mc.writeClientStatus(0);
    }
    // mc.writeInteractAt(0, 0, 0, -129, 65, -62);
    vTaskDelay(pdMS_TO_TICKS(1000));

    //Serial.println("[INFO] # buf: " + String(client.available()/1024.0) + "kB Heap: " + String(ESP.getFreeHeap()/1024.0) + "kB");
    //mc.writeChat(client, "[nik INFO] Heap: " + String(ESP.getFreeHeap()/1024.0) + "kB || buf: " + String(client.available()/1024.0) + "kB");
}

#include <Arduino.h>
#include <WiFi.h>
#include <minecraft.h>
#include <credentials.h>
//#include "../src/miniz.h"

TaskHandle_t listener;
WiFiClient client;
minecraft mc (username, server_url, server_port, &client);

void update( void * pvParameters ){
    mc.handle();
}

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
    xTaskCreatePinnedToCore(update, "listener", 100000, NULL, 2, &listener, 0);

    mc.handShake(2);
    Serial.println("[INFO] -> handShake packet sent");
    vTaskDelay(pdMS_TO_TICKS(1000));
    //mc.ping(client, 1);

    mc.loginStart();
    Serial.println("[INFO] -> login_start packet sent");
    vTaskDelay(pdMS_TO_TICKS(2000));

    /*mc.clientStatus(client, 0);
    Serial.println("[INFO] -> client_status packet sent");
    delay(2000);*/

    /*mc.writeChat(client, "/login nikk");
    Serial.println("[INFO] -> logging in as nikbot");
    delay(200);*/

    // mc.writeChat("test");
    // Serial.println("[INFO] -> writing to chat");
    // vTaskDelay(pdMS_TO_TICKS(1000));
    disableCore0WDT();
    disableCore1WDT();
}

void loop(){
    Serial.println("[INFO] # buf: " + String(client.available()/1024.0) + "kB");
    vTaskDelay(pdMS_TO_TICKS(2000));
    //mc.writeChat(client, "[nik INFO] Heap: " + String(ESP.getFreeHeap()/1024.0) + "kB || buf: " + String(client.available()/1024.0) + "kB");
}

void printHex(int num, int precision) {
    char tmp[16];
    char format[128];
    sprintf(format, "%%.%dX", precision);
    sprintf(tmp, format, num);
    Serial.print(tmp);
}

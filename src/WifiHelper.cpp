#include "WifiHelper.h"
#include <Arduino.h>
#include "wifi_config.h"

#ifndef HOSTNAME
#define HOSTNAME "esp32cam"
#endif

void WifiHelper::Connect()
{
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(HOSTNAME);
    
    WiFi.begin(WIFI_SSID, WIFI_KEY);
    delay(500);
    if (!WiFi.isConnected())
    {
        delay(500);
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_KEY);

        while (!WiFi.isConnected())
        {
            delay(1000);
            Serial.println("retry.");
        }
    }

    Serial.println("WiFi connected");
    Serial.print("http://");
    Serial.println(WiFi.localIP());
}
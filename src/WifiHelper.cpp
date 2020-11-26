#include "WifiHelper.h"
#include <Arduino.h>
#include "wifi_config.h"

void WifiHelper::Connect()
{
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
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
            Serial.print("retry.");
        }
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("http://");
    Serial.println(WiFi.localIP());
}
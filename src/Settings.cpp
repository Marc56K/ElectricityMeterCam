#include "Settings.h"
#include <ArduinoJson.h>

#define PREF_NAMESPACE "mc"
#define DIGIT_BBOX "bb"

Settings::Settings()
{
    _lock = xSemaphoreCreateMutex();
}

Settings::~Settings()
{
}

void Settings::Load()
{
    xSemaphoreTakeRecursive(_lock, portMAX_DELAY);
    {
        _preferences.begin(PREF_NAMESPACE, true);
        const uint32_t left = 19;
        const uint32_t stepSize = 37;
        for (uint32_t i = 0; i < NUM_DIGITS; i++)
        {
            String name(DIGIT_BBOX);
            name += i;

            _digitBBoxes[i].x = _preferences.getUInt((name + "_x").c_str(), left + i * stepSize);
            _digitBBoxes[i].y = _preferences.getUInt((name + "_y").c_str(), 112);
            _digitBBoxes[i].w = _preferences.getUInt((name + "_w").c_str(), 30);
            _digitBBoxes[i].h = _preferences.getUInt((name + "_h").c_str(), 42);
        }
        _preferences.end();
    }
    xSemaphoreGiveRecursive(_lock);
}

void Settings::Save()
{
    xSemaphoreTakeRecursive(_lock, portMAX_DELAY);
    {
        _preferences.begin(PREF_NAMESPACE, false);
        for (uint32_t i = 0; i < NUM_DIGITS; i++)
        {
            String name(DIGIT_BBOX);
            name += i;
            _preferences.putUInt((name + "_x").c_str(), _digitBBoxes[i].x);
            _preferences.putUInt((name + "_y").c_str(), _digitBBoxes[i].y);
            _preferences.putUInt((name + "_w").c_str(), _digitBBoxes[i].w);
            _preferences.putUInt((name + "_h").c_str(), _digitBBoxes[i].h);
        }
        _preferences.end();
    }
    xSemaphoreGiveRecursive(_lock);
}

DigitBBox Settings::GetDigitBBox(uint8_t idx)
{
    DigitBBox result;
    xSemaphoreTakeRecursive(_lock, portMAX_DELAY);
    {
        result = _digitBBoxes[idx % NUM_DIGITS];
    }
    xSemaphoreGiveRecursive(_lock);
    return result;
}

String Settings::GetDigitBBoxesAsJson()
{
    char buffer[1024];
    xSemaphoreTakeRecursive(_lock, portMAX_DELAY);
    {
        StaticJsonDocument<1024> json;
        JsonArray array = json.to<JsonArray>();
        for (uint32_t i = 0; i < NUM_DIGITS; i++)
        {
            JsonObject obj = array.createNestedObject();
            obj["x"] = _digitBBoxes[i].x;
            obj["y"] = _digitBBoxes[i].y;
            obj["w"] = _digitBBoxes[i].w;
            obj["h"] = _digitBBoxes[i].h;
        }
        serializeJson(array, buffer);
    }
    xSemaphoreGiveRecursive(_lock);
    return buffer;
}

bool Settings::SetDigitBBoxesFromJson(const char *jsonStr)
{
    bool result = false;
    xSemaphoreTakeRecursive(_lock, portMAX_DELAY);
    {
        StaticJsonDocument<1024> json;
        if (deserializeJson(json, jsonStr) == DeserializationError::Code::Ok)
        {
            if (json.is<JsonArray>())
            {
                JsonArray array = json.as<JsonArray>();
                for (uint8_t i = 0; i < std::min<uint32_t>(NUM_DIGITS, array.size()); i++)
                {
                    if (array[i].is<JsonObject>())
                    {
                        JsonObject obj = array[i].as<JsonObject>();
                        if (obj.containsKey("x"))
                        {
                            _digitBBoxes[i].x = obj["x"].as<uint32_t>();
                        }
                        if (obj.containsKey("y"))
                        {
                            _digitBBoxes[i].y = obj["y"].as<uint32_t>();
                        }
                        if (obj.containsKey("w"))
                        {
                            _digitBBoxes[i].w = obj["w"].as<uint32_t>();
                        }
                        if (obj.containsKey("h"))
                        {
                            _digitBBoxes[i].h = obj["h"].as<uint32_t>();
                        }
                    }
                }
                result = true;
            }
        }
    }
    xSemaphoreGiveRecursive(_lock);
    return result;
}
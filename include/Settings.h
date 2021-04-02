#pragma once
#include <Arduino.h>
#include <Preferences.h>

#define NUM_DIGITS 7

struct DigitBBox
{
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;

    DigitBBox() :
        x(0), y(0), w(30), h(42)
    {
    }
};

class Settings
{
public:
    Settings();
    ~Settings();

    void Load();
    void Save();

    DigitBBox GetDigitBBox(uint8_t idx);

    String GetDigitBBoxesAsJson();
    bool SetDigitBBoxesFromJson(const char* json);
    
private:
    DigitBBox _digitBBoxes[NUM_DIGITS];
    Preferences _preferences;
    SemaphoreHandle_t _lock;
};
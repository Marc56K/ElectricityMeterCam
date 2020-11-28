#pragma once
#include <Arduino.h>

class SDCard
{
public:
    SDCard();
    ~SDCard();

    bool Init();
    bool IsAvailable();
    bool WriteToFile(const String& filePath, const String& line, const bool append = true);

private:
    bool _available;
};
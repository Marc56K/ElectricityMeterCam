#pragma once
#include <Arduino.h>

class SDCard
{
public:
    SDCard();
    ~SDCard();

    bool Init();
    bool IsAvailable();
    bool AppendToFile(const String& filePath, const String& line);

private:
    bool _available;
};
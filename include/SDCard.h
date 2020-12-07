#pragma once
#include <Arduino.h>
#include "FS.h"
#include "SD_MMC.h"

class SDCard
{
public:
    SDCard();
    ~SDCard();

    bool Mount();
    void Unmount();
    bool IsMounted();
    bool IsWritable();
    uint64_t GetFreeSpaceInBytes();
    bool WriteToFile(const String& filePath, const String& line, const bool append = true);
    bool CreateNextFile(const String& dir, const String& name, File& file);

private:
    bool _mounted;
};
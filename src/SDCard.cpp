#include "SDCard.h"
#include "FS.h"
#include "SD_MMC.h"

SDCard::SDCard() :
    _available(false)
{    
}
    
SDCard::~SDCard()
{    
}

bool SDCard::Init()
{
    _available = false;

    if(!SD_MMC.begin("/sdcard", true)) // using slow mode1bit to allow use of LED on pin 4
    {
        Serial.println("SD Card Mount Failed");
        return false;
    }

    switch (SD_MMC.cardType())
    {
        case CARD_MMC:
            Serial.println("Found MMC-Card");
            break;
        case CARD_SD:
            Serial.println("Found SD-Card");
            break;
        case CARD_SDHC:
            Serial.println("Found SDHC-Card");
            break;
        case CARD_UNKNOWN:
            Serial.println("Found Unknown-Card");
            break;
        default:
            Serial.println("No SD Card attached");
            return false;
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024 * 1024);
    Serial.printf("SD Card Size: %lluGB\n", cardSize);

    _available = true;

    return true;
}

bool SDCard::IsAvailable()
{
    return _available;
}

bool SDCard::WriteToFile(const String& filePath, const String& line, const bool append)
{
    if (IsAvailable())
    {
        Serial.println(String("Writing '") + filePath + "' " + line);
        File file = SD_MMC.open(filePath, append ? FILE_APPEND : FILE_WRITE);
        if(!file)
        {
            return false;
        }

        file.write((uint8_t*)line.c_str(), line.length());
        file.write('\n');

        file.close();
        return true;
    }

    return false;
}
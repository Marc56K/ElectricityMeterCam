#include "SDCard.h"

SDCard::SDCard()
    : _mounted(false)
{    
}
    
SDCard::~SDCard()
{
    Unmount();
}

bool SDCard::Mount()
{
    if (IsMounted())
    {
        return true;
    }

    if(!SD_MMC.begin("/sdcard", true)) // using slow mode1bit to allow use of LED on pin 4
    {
        return false;
    }

    switch (SD_MMC.cardType())
    {
        case CARD_MMC:
            Serial.print("Mounted MMC-Card");
            break;
        case CARD_SD:
            Serial.print("Mounted SD-Card");
            break;
        case CARD_SDHC:
            Serial.print("Mounted SDHC-Card");
            break;
        case CARD_UNKNOWN:
            Serial.print("Mounted Unknown-Card");
            break;
        default:
            return false;
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf(" (%lluMB)\n", cardSize);

    _mounted = true;

    return true;
}

void SDCard::Unmount()
{
    _mounted = false;
    SD_MMC.end();
}

bool SDCard::IsMounted()
{
    return _mounted;
}

bool SDCard::IsWritable()
{
    if (IsMounted())
    {
        String filePath = "/.w";
        File file = SD_MMC.open(filePath, FILE_WRITE);
        if (file)
        {
            file.write(42);
            file.close();
        }
        else
        {
            return false;
        }

        file = SD_MMC.open(filePath, FILE_READ);
        if (file)
        {  
            bool result = file.read() == 42;
            file.close();
            SD_MMC.remove("/.w");
            return result;
        }
        return false;
    }
    return false;
}

uint64_t SDCard::GetFreeSpaceInBytes()
{
    if (IsMounted())
    {
        return SD_MMC.totalBytes() - SD_MMC.usedBytes();
    }
    return 0;
}

bool SDCard::WriteToFile(const String& filePath, const String& line, const bool append)
{
    if (IsMounted())
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

bool SDCard::CreateNextFile(const String& dir, const String& name, File& file)
{
    if (IsMounted())
    {
        String idxFilePath = dir + "/" + name + ".idx";
        uint32_t nextIdx = 0;
        if (SD_MMC.exists(idxFilePath))
        {
            File idxFile = SD_MMC.open(idxFilePath, FILE_READ);
            if (idxFile)
            {
                idxFile.read((uint8_t*)&nextIdx, sizeof(nextIdx));
                idxFile.close();
            }
        }

        String filePath = dir + "/" + nextIdx + "_" + millis() + "_" + name;
        Serial.println(String("Writing '") + filePath + "'");
        file = SD_MMC.open(filePath, FILE_WRITE);
        
        // write next index to *.idx file
        if (file)
        {
            nextIdx++;
            File idxFile = SD_MMC.open(idxFilePath, FILE_WRITE);
            if (idxFile)
            {
                idxFile.write((uint8_t*)&nextIdx, sizeof(nextIdx));
                idxFile.close();
            }
            return true;
        }
    }
    return false;
}
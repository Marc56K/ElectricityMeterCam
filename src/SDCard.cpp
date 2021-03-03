/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2021 Marc Roßbach
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

bool SDCard::OpenFileForWriting(const String& filePath, File& file)
{
    if (IsMounted())
    {
        Serial.println(String("Writing '") + filePath + "'");
        file = SD_MMC.open(filePath, FILE_WRITE);
        
        if (file)
        {
            return true;
        }
    }
    return false;
}
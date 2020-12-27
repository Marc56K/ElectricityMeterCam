#include <Arduino.h>
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems

#include "NTPClient.h"
#include "SDCard.h"
#include "CameraServer.h"
#include "WifiHelper.h"
#include "OCR.h"

#define LED_PIN 4
#define MIN_CONFIDENCE 0.4f

SDCard sdCard;
//OCR ocr(ocr_model_28x28_tflite, 28, 28, 10);
OCR ocr(ocr_model_28x28_c11_tflite, 28, 28, 11);
CameraServer camServer;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600);

void setup()
{
    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

    Serial.begin(115200);
    Serial.println("starting ...");

    sdCard.Mount();

    WifiHelper::Connect();

    if (camServer.InitCamera(false))
    {
        camServer.StartServer();

        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LOW);

        Serial.println("started");
    }
    timeClient.begin();
}

int DetectDigit(dl_matrix3du_t* frame, const int x, const int y, const int width, const int height, float* confidence)
{
    int digit = ocr.PredictDigit(frame, x, y, width, height, confidence);
    uint32_t color = ImageUtils::GetColorFromConfidence(*confidence, MIN_CONFIDENCE, 1.0f);
    ImageUtils::DrawRect(x, y, width, height, color, frame);
    ImageUtils::DrawFillRect(x, y - 4, width * (*confidence), 4, color, frame);
    ImageUtils::DrawText(x + width / 5, y + height, color, String(digit), frame);
    return digit;
}

void warten(unsigned long milisec)
{
    vTaskDelay(milisec * portTICK_PERIOD_MS);
}

void updateConnections()
{
    if (!WiFi.isConnected())
    {
        WifiHelper::Connect();
    }

    if (sdCard.IsMounted() && !sdCard.IsWritable())
    {
        Serial.println("SD card is readonly or disconnected.");
        sdCard.Unmount();
    }

    timeClient.update();
}

void loop()
{
    updateConnections();

    KwhInfo info = {};    
    info.unixtime = timeClient.getEpochTime();
    const String time = timeClient.getFormattedTime();

    int digit = 0;
    Serial.println("LEDs an");
    digitalWrite(LED_PIN, HIGH);
    warten(1000);
    Serial.println("Bild holen");
    auto* frame = camServer.CaptureFrame(info.unixtime, &sdCard);    
    Serial.println("LEDs aus");
    digitalWrite(LED_PIN, LOW);
    
    if (frame != nullptr)
    {
        Serial.println("Auswertung");
        int left = 17;
        int stepSize = 37;
        
        info.kwh = 0;
        info.confidence = 1.0;
        float conf = 0;
        for (int i = 0; i < 7; i++)
        {
            switch(i){
            case 0 ... 1:
                digit = DetectDigit(frame, left + stepSize * i, 132, 30, 42, &conf);
                break;        
            case 2 ... 3:
                digit = DetectDigit(frame, left + (stepSize+1) * i, 134, 30, 42, &conf);        
                break;
            case 4 ... 6:
                digit = DetectDigit(frame, left + (stepSize+1) * i, 135, 30, 42, &conf);
            default:
                break;
            }
            info.confidence = std::min(conf, info.confidence);
            info.kwh += pow(10, 5 - i) * digit;
        }
        
        uint32_t color = ImageUtils::GetColorFromConfidence(info.confidence, MIN_CONFIDENCE, 1.0f);
        ImageUtils::DrawText(120, 5, color, String("") +  (int)(info.confidence * 100) + "%", frame);
        ImageUtils::DrawText(190, 5, COLOR_TURQUOISE, String("") + time, frame);
        
        // send result to http://esp32cam/kwh/ endpoint
        camServer.SetLatestKwh(info);

        // send frame to http://esp32cam/ endpoint
        camServer.SwapBuffers();
        Serial.println(String("Time: ") + time + String(" VALUE: ") + info.kwh + " kWh (" + (info.confidence * 100) + "%)");
        sdCard.WriteToFile("/kwh.csv", String("") + info.unixtime + "\t" + info.kwh + "\t" + info.confidence);
    }

    if (millis() < 300000) // more frequent updates in first 5 minutes
    {
        warten(500);
    }
    else
    {
        warten(60000);
    }    

    if (millis() > 24 * 60 * 60 * 1000) // restart esp after 24 hours
    {
        Serial.println("Restart");
        Serial.flush();
        ESP.restart();
    }
}
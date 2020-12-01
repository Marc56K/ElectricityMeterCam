#include <Arduino.h>
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems

#include "SDCard.h"
#include "CameraServer.h"
#include "WifiHelper.h"
#include "OCR.h"

#define LED_PIN 4

//OCR ocr(ocr_model_22x32_tflite, 22, 32);
SDCard sdCard;
OCR ocr(ocr_model_28x28_tflite, 28, 28);
CameraServer camServer;

void setup()
{
    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

    Serial.begin(115200);
    Serial.println("starting ...");

    if (sdCard.Init())
    {
        Serial.println("SD-Card connected");
    }

    WifiHelper::Connect();

    if (camServer.InitCamera(false))
    {
        camServer.StartServer();

        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LOW);

        Serial.println("started");
    }
}

int DetectDigit(dl_matrix3du_t* frame, const int x, const int y, const int width, const int height, float* confidence)
{
    int digit = ocr.PredictDigit(frame, x, y, width, height, confidence);
    uint32_t color = ImageUtils::GetColorFromConfidence(*confidence, 0.4f, 1.0f);
    ImageUtils::DrawRect(x, y, width, height, color, frame);
    ImageUtils::DrawText(x + width / 5, y + height, color, String(digit), frame);
    return digit;
}

void warten(unsigned long milisec)
{
    vTaskDelay(milisec * portTICK_PERIOD_MS);
}

void loop()
{
    int digit = 0;
    Serial.println("LEDs an");
    digitalWrite(LED_PIN, HIGH);
    warten(1000);
    Serial.println("Bild holen");
    auto* frame = camServer.CaptureFrame(&sdCard);    
    Serial.println("LEDs aus");
    digitalWrite(LED_PIN, LOW);
    //Serial.println("Auswertung");
    
    if (frame != nullptr)
    {
        Serial.println("Auswertung");
        int left = 18;
        int stepSize = 37;
        float minConf = 1.0;
        float kwh = 0;
        float confidence = 0;
        for (int i = 0; i < 7; i++)
        {
            switch(i){
            case 1 ... 2:
                digit = DetectDigit(frame, left + stepSize * i, 132, 30, 42, &confidence);        
            case 3:
                digit = DetectDigit(frame, left + (stepSize+1) * i, 133, 30, 42, &confidence);        
            case 4 ... 7:
                digit = DetectDigit(frame, left + (stepSize+2) * i, 134, 30, 42, &confidence);        
            }
            minConf = std::min(confidence, minConf);
            kwh += pow(10, 5 - i) * digit;
        }

        camServer.SetLatestKwh(kwh);

        Serial.println(String("VALUE: ") + kwh + " kWh (" + (minConf * 100) + "%)");
        sdCard.WriteToFile("/kwh.csv", String("") + millis() + "\t" + kwh + "\t" + minConf);
    }
    //Serial.println("warte 60 Sekunden");
    warten(60000);

}
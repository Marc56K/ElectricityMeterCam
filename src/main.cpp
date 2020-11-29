#include <Arduino.h>
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems

#include "CameraServer.h"
#include "WifiHelper.h"
#include "OCR.h"

#define LED_PIN 4

//OCR ocr(ocr_model_22x32_tflite, 22, 32);
OCR ocr(ocr_model_28x28_tflite, 28, 28);
CameraServer camServer;
unsigned long time_now = 0;

void setup()
{
    //disable brownout detector
    //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

    Serial.begin(115200);
    Serial.println("starting ...");

    WifiHelper::Connect();

    if (camServer.InitCamera(false))
    {
        camServer.StartServer();

        // switch on the lights
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LOW);            

        Serial.println("started");
    }
}

int DetectDigit(dl_matrix3du_t* frame, const int x, const int y, const int width, const int height, float* confidence)
{
    int digit = ocr.PredictDigit(frame, x, y, width, height, confidence);
    //Serial.println(String("PREDICTION: ") + digit);
    ImageUtils::DrawRect(x, y, width, height, COLOR_RED, frame);
    ImageUtils::DrawText(x + width / 5, y + height, COLOR_RED, String(digit), frame);
    return digit;
}

void warten(unsigned long milisec)
{
    //time_now = millis();
    //while(millis() < (time_now + milisec)){}
    vTaskDelay(milisec * portTICK_PERIOD_MS);
}


void loop()
{
    Serial.println("Bitte lächeln");
    digitalWrite(LED_PIN, HIGH);
    warten(2000);
    auto* frame = camServer.CaptureFrame();    
    digitalWrite(LED_PIN, LOW);
    Serial.println("Auswertung");
    if (frame != nullptr)
    {
        int left = 18;
        int stepSize = 37;
        float minConf = 1.0;
        float result = 0;
        float confidence = 0;
        for (int i = 0; i < 7; i++)
        {
            int digit = DetectDigit(frame, left + stepSize * i, 132, 30, 42, &confidence);
            minConf = std::min(confidence, minConf);
            result += pow(10, 5 - i) * digit;
        }
        Serial.println(String("VALUE: ") + result + " kWh (" + (minConf * 100) + "%)");
    }
    //Serial.println("warte 30 Sekunden");
    warten(3000);
}
#include <Arduino.h>
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems

#include "CameraServer.h"
#include "WifiHelper.h"
#include "OCR.h"

//OCR ocr(ocr_model_22x32_tflite, 22, 32);
OCR ocr(ocr_model_28x28_tflite, 28, 28);
CameraServer camServer;

void setup()
{
    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

    Serial.begin(115200);
    Serial.println("starting ...");

    WifiHelper::Connect();

    if (camServer.InitCamera(false))
    {
        camServer.StartServer();
        Serial.println("started");
    }
}

int DetectDigit(dl_matrix3du_t* frame, const int x, const int y, const int width, const int height)
{
    int digit = ocr.PredictDigit(frame, x, y, width, height);
    //Serial.println(String("PREDICTION: ") + digit);
    ImageUtils::DrawRect(x, y, width, height, COLOR_RED, frame);
    ImageUtils::DrawText(x + width / 5, y + height, COLOR_RED, String(digit), frame);
    return digit;
}

void loop()
{
    auto* frame = camServer.CaptureFrame();    
    if (frame != nullptr)
    {
        int left = 24;
        int stepSize = 40;
        float result = 0;
        for (int i = 0; i < 7; i++)
        {
            int digit = DetectDigit(frame, left + stepSize * i, 98, 32, 42);
            result += pow(10, 5 - i) * digit;
        }
        Serial.println(String("VALUE: ") + result);
    }
    delay(100);
}
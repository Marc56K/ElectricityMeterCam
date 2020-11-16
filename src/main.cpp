#include <Arduino.h>
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems

#include "CameraServer.h"
#include "WifiHelper.h"

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

void loop()
{
    delay(1);
}
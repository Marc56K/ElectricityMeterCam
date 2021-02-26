#include <Arduino.h>
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems
#include "NTPClient.h"
#include "SDCard.h"
#include "CameraServer.h"
#include "WifiHelper.h"
#include "OCR.h"
#include "PubSubClient.h"
#include "wifi_config.h"

#define LED_PIN 4
#define MIN_CONFIDENCE 0.4f

WiFiClient espClient;
PubSubClient client(espClient);

SDCard sdCard;
//OCR ocr(ocr_model_28x28_tflite, 28, 28, 10);
OCR ocr(ocr_model_28x28_c11_tflite, 28, 28, 11);
CameraServer camServer;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600);

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

void mqttconnect() {
  if (!client.connected()){
  // Loop until we're reconnected
    //bboot = true;
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("metercam",USER,PASS)) {
        Serial.println("connected");
        // Subscribe
        //client.subscribe("metercam/");
        //Serial.println("subscribed to topic </>");
        //warten(100);
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        warten(5000);
      }
    }
  }
}

void callback(char* topic, byte* message, unsigned int length)
{
  Serial.print("Message arrived on topic: <");
  Serial.print(topic);
  Serial.print("> Message: ");
  String messageTemp = "";
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
 
  if (String(topic) == "metercam/") {
    if (messageTemp == "value"){
      //client.publish("");
      Serial.println("doing something");
    }
  }
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
    mqttconnect();
    client.loop();
}

// C program for implementation of ftoa() 
#include <math.h> 
#include <stdio.h> 

// Reverses a string 'str' of length 'len' 
void reverse(char* str, int len) 
{ 
	int i = 0, j = len - 1, temp; 
	while (i < j) { 
		temp = str[i]; 
		str[i] = str[j]; 
		str[j] = temp; 
		i++; 
		j--; 
	} 
} 

// Converts a given integer x to string str[]. 
// d is the number of digits required in the output. 
// If d is more than the number of digits in x, 
// then 0s are added at the beginning. 
int intToStr(int x, char str[], int d) 
{ 
	int i = 0; 
	while (x) { 
		str[i++] = (x % 10) + '0'; 
		x = x / 10; 
	} 

	// If number of digits required is more, then 
	// add 0s at the beginning 
	while (i < d) 
		str[i++] = '0'; 

	reverse(str, i); 
	str[i] = '\0'; 
	return i; 
} 

// Converts a floating-point/double number to a string. 
void ftoa(float n, char* res, int afterpoint) 
{ 
	// Extract integer part 
	int ipart = (int)n; 

	// Extract floating part 
	float fpart = n - (float)ipart; 

	// convert integer part to string 
	int i = intToStr(ipart, res, 0); 

	// check for display option after point 
	if (afterpoint != 0) { 
		res[i] = '.'; // add dot 

		// Get the value of fraction part upto given no. 
		// of points after dot. The third parameter 
		// is needed to handle cases like 233.007 
		fpart = fpart * pow(10, afterpoint); 

		intToStr((int)fpart, res + i + 1, afterpoint); 
	} 
} 

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
    client.setServer(CLOUD, 1883);
    client.setCallback(callback);
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
        int left = 19;
        int stepSize = 37;
        
        info.kwh = 0;
        info.confidence = 1.0;
        float conf = 0;
        for (int i = 0; i < 7; i++)
        {
            switch(i){
            case 0 ... 1:
                digit = DetectDigit(frame, left + stepSize * i, 112, 30, 42, &conf);
                break;        
            case 2 ... 3:
                digit = DetectDigit(frame, left + (stepSize+2) * i, 111, 30, 42, &conf);        
                break;
            case 4 ... 6:
                digit = DetectDigit(frame, left + (stepSize+1) * i, 108, 30, 42, &conf);
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

    char tempString[20];
    itoa(info.confidence * 100, tempString,10);
    client.publish("metercam/confidence", tempString);
    if (info.confidence > 0.9){
    ftoa(info.kwh, tempString, 2);
    client.publish("metercam/metervalue", tempString);
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
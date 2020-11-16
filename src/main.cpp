#include <Arduino.h>
#include "soc/soc.h"          //disable brownout problems
#include "soc/rtc_cntl_reg.h" //disable brownout problems

#include <math.h>
#include "tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h"
#include "tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/experimental/micro/micro_interpreter.h"
#include "ocr_model.h"

#include "CameraServer.h"
#include "WifiHelper.h"
#include "ImageUtils.h"

CameraServer camServer;

// Create a memory pool for the nodes in the network
constexpr int tensor_pool_size = 64 * 1024;
uint8_t* tensor_pool = nullptr;

// Define the model to be used
const tflite::Model* ocr_model;

// Define the interpreter
tflite::MicroInterpreter* interpreter;

// Input/Output nodes for the network
TfLiteTensor* input;
TfLiteTensor* output;

void reportFreeHeap()
{
    size_t heapsize = heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024;
    Serial.println(String("Unused heap in KB: ") + heapsize);    
}

void initCNN()
{
    // Load the model
	Serial.println("Loading Tensorflow model....");
	ocr_model = tflite::GetModel(ocr_model_tflite);
	Serial.println("model loaded!");

	// Define ops resolver and error reporting
	static tflite::ops::micro::AllOpsResolver resolver;

	static tflite::ErrorReporter* error_reporter;
	static tflite::MicroErrorReporter micro_error;
	error_reporter = &micro_error;

	// Instantiate the interpreter 
    Serial.println("Allocating memory pool");
    tensor_pool = (uint8_t*)malloc(tensor_pool_size);
    if (tensor_pool == nullptr)
    {
        Serial.print("failed to allocate tensor pool");
        return;
    }

    Serial.println("Creating MicroInterpreter");
	static tflite::MicroInterpreter static_interpreter(
		ocr_model, resolver, tensor_pool, tensor_pool_size, error_reporter
	);

	interpreter = &static_interpreter;

	// Allocate the the model's tensors in the memory pool that was created.
	Serial.println("Allocating tensors to memory pool");
	if(interpreter->AllocateTensors() != kTfLiteOk) {
		Serial.println("There was an error allocating the memory...ooof");
		return;
	}

	// Define input and output nodes
	input = interpreter->input(0);
	output = interpreter->output(0);
	Serial.println("Starting inferences... ! ");
}

int predict(const dl_matrix3du_t* frame, const int x, const int y)
{
    ImageUtils::GetNormalizedPixels(frame, x, y, 28, 28, input->data.f);

    // Run inference on the input data
    if(interpreter->Invoke() != kTfLiteOk)
    {
        Serial.println("There was an error invoking the interpreter!");
        return -1;
    }

    float bestConv = 0.0;
    int bestMatch = -1;
    for (int i = 0; i < 10; i++)
    {        
        Serial.println(String(i) + " -> " + output->data.f[i] * 100);
        if (output->data.f[i] > bestConv)
        {
            bestConv = output->data.f[i];
            bestMatch = i;
        }
    }
    return bestMatch;
}

void DetectDigit(dl_matrix3du_t* frame, const int x, const int y)
{
    int digit = predict(frame, x, y);
    Serial.println(String("PREDICTION: ") + digit);
    ImageUtils::DrawRect(x, y, 28, 28, COLOR_RED, frame);
    ImageUtils::DrawText(x + 5, y + 28, COLOR_RED, String(digit), frame);
}

void setup()
{
    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

    Serial.begin(115200);
    Serial.println("starting ...");

    initCNN();

    WifiHelper::Connect();

    if (camServer.InitCamera(false))
    {
        camServer.StartServer();
        Serial.println("started");
    }
}

void loop()
{
    auto* frame = camServer.CaptureFrame();    
    if (frame != nullptr)
    {
        DetectDigit(frame, 146, 106);
    }
    delay(100);
}
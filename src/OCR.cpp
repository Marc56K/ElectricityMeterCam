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

#include "OCR.h"
#include <Arduino.h>

constexpr int tensor_pool_size = 64 * 1024;

OCR::OCR(const void* model, const int inputWidth, const int inputHeight, const int outputClasses) :
    _inputWidth(inputWidth),
    _inputHeight(inputHeight),
    _outputClasses(outputClasses),
    _interpreter(nullptr),
    _input(nullptr),
    _output(nullptr)
{
    // Load the model
	Serial.println("Loading Tensorflow model....");
	_model = tflite::GetModel(model);
	Serial.println("model loaded!");

	// Define ops resolver and error reporting
	static tflite::ops::micro::AllOpsResolver resolver;

	static tflite::ErrorReporter* error_reporter;
	static tflite::MicroErrorReporter micro_error;
	error_reporter = &micro_error;

	// Instantiate the interpreter 
    Serial.println("Allocating memory pool");
    _tensorMemoryPool = (uint8_t*)malloc(tensor_pool_size);
    if (_tensorMemoryPool == nullptr)
    {
        Serial.print("failed to allocate tensor pool");
        return;
    }

	static tflite::MicroInterpreter static_interpreter(
		_model, resolver, _tensorMemoryPool, tensor_pool_size, error_reporter
	);

	_interpreter = &static_interpreter;

	// Allocate the the model's tensors in the memory pool that was created.
	Serial.println("Allocating tensors to memory pool");
	if(_interpreter->AllocateTensors() != kTfLiteOk) {
		Serial.println("There was an error allocating the memory...ooof");
		return;
	}

	// Define input and output nodes
	_input = _interpreter->input(0);
	_output = _interpreter->output(0);
	Serial.println("Starting inferences... ! ");
}

OCR::~OCR()
{
    if (_tensorMemoryPool != nullptr)
    {
        free(_tensorMemoryPool);
    }  
}

int OCR::PredictDigit(const dl_matrix3du_t* frame, const int rectX, const int rectY, const int rectWidth, const int rectHeight, float* confidence)
{
    auto start = millis();
    ImageUtils::GetNormalizedPixels(
        frame, 
        rectX,
        rectY,
        rectWidth,
        rectHeight,
        _input->data.f,
        _inputWidth,
        _inputHeight);

    auto end = millis();
    Serial.println(String("Conversion: ") + (end - start) + "ms");
    start = end;

    // Run inference on the input data
    if(_interpreter->Invoke() != kTfLiteOk)
    {
        Serial.println("There was an error invoking the interpreter!");
        return -1;
    }

    float bestConf = 0.0;
    int bestMatch = -1;
    for (int i = 0; i < _outputClasses; i++)
    {        
        Serial.print(String("[") + i + "](" + (int)round(_output->data.f[i] * 100) + "%) ");
        if (i < 10 && _output->data.f[i] > bestConf)
        {
            bestConf = _output->data.f[i];
            bestMatch = i;
        }
    }

    Serial.println();

    if (confidence != nullptr)
    {
        *confidence = bestConf;
    }
    
    end = millis();
    Serial.println(String("Inference: ") + (end - start) + "ms");

    return bestMatch;
}
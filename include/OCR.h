#pragma once
#include "ImageUtils.h"
#include <math.h>
#include "tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h"
#include "tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/experimental/micro/micro_interpreter.h"
#include "ocr_models.h"

class OCR
{
public:
    OCR(const void* model, const int inputWidth, const int inputHeight, const int outputClasses);
    ~OCR();

    int PredictDigit(
        const dl_matrix3du_t* frame, 
        const int rectX, 
        const int rectY, 
        const int rectWidth, 
        const int rectHeight,
        float* confidence = nullptr);

private:
    int _inputWidth;
    int _inputHeight;
    int _outputClasses;
    uint8_t* _tensorMemoryPool = nullptr;
    const tflite::Model* _model;
    tflite::MicroInterpreter* _interpreter;
    TfLiteTensor* _input;
    TfLiteTensor* _output;
};
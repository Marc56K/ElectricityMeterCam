#pragma once
#include <Arduino.h>
#include "fb_gfx.h"
#include "fr_forward.h"

#define COLOR_WHITE     0x00FFFFFF
#define COLOR_BLACK     0x00000000
#define COLOR_RED       0x000000FF
#define COLOR_GREEN     0x0000FF00
#define COLOR_BLUE      0x00FF0000
#define COLOR_YELLOW    0x0000FFFF
#define COLOR_TURQUOISE 0x00D0E040

class ImageUtils
{
public:
    static void DrawRect(const int x, const int y, const int w, const int h, const uint32_t color, dl_matrix3du_t* dst);
    static void DrawFillRect(const int x, const int y, const int w, const int h, const uint32_t color, dl_matrix3du_t* dst);
    static void DrawText(const int x, const int y, const uint32_t color, const String& txt, dl_matrix3du_t* dst);

    static void GetNormalizedPixels(
        const dl_matrix3du_t* src, 
        const int srcRectX, 
        const int srcRectY, 
        const int srcRectWidth,
        const int srcRectHeight, 
        float* dst,
        const int dstWidth,
        const int dstHeight);

    static uint32_t GetColorFromConfidence(const float confidence, const float min, const float max); 
};
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


#pragma once
#include <Arduino.h>
#include "fb_gfx.h"
#include "fr_forward.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

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
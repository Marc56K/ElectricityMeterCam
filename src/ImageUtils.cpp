#include "ImageUtils.h"

void ImageUtils::DrawRect(const int x, const int y, const int w, const int h, const uint32_t color, dl_matrix3du_t* dst)
{
    fb_data_t fb;
    fb.width = dst->w;
    fb.height = dst->h;
    fb.data = dst->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;

    fb_gfx_drawFastHLine(&fb, x, y, w, color);
    fb_gfx_drawFastHLine(&fb, x, y+h-1, w, color);
    fb_gfx_drawFastVLine(&fb, x, y, h, color);
    fb_gfx_drawFastVLine(&fb, x+w-1, y, h, color);
}

void ImageUtils::DrawFillRect(const int x, const int y, const int w, const int h, const uint32_t color, dl_matrix3du_t* dst)
{
    fb_data_t fb;
    fb.width = dst->w;
    fb.height = dst->h;
    fb.data = dst->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;

    fb_gfx_fillRect(&fb, x, y, w, h, color);
}

void ImageUtils::DrawText(const int x, const int y, const uint32_t color, const String& txt, dl_matrix3du_t* dst)
{
    fb_data_t fb;
    fb.width = dst->w;
    fb.height = dst->h;
    fb.data = dst->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;

    fb_gfx_print(&fb, x, y, color, txt.c_str());
}

void ImageUtils::GetNormalizedPixels(
    const dl_matrix3du_t* src, 
    const int srcRectX, 
    const int srcRectY, 
    const int srcRectWidth,
    const int srcRectHeight, 
    float* dst,
    const int dstWidth,
    const int dstHeight)
{
    auto getGrayScaledSrcPixel = [&](const int idx) -> float
    {
        const float r = src->item[idx + 0];
        const float g = src->item[idx + 1];
        const float b = src->item[idx + 2];
        return (r / 255.0f + g / 255.0f + b / 255.0f) / 3.0f;
    };

    if (srcRectWidth == dstWidth && srcRectHeight == dstHeight)
    {
        int dstIdx = 0;
        for (int _y = srcRectY; _y < srcRectY + srcRectHeight; _y++)
        {
            int offset = _y * src->w;
            for (int _x = srcRectX; _x < srcRectX + srcRectWidth; _x++)
            {
                const int idx = (offset + _x) * 3;
                dst[dstIdx++] = getGrayScaledSrcPixel(idx);
            }
        }
    }
    else // nearest neighbor scaling 
    {
        auto getSrcRectPixelByUV = [&](const float u, const float v) -> float
        {
            const int srcX = srcRectX + (int)roundf(u * srcRectWidth);
            const int srcY = srcRectY + (int)roundf(v * srcRectHeight);

            const int idx = (srcY * src->w + srcX) * 3;
            return getGrayScaledSrcPixel(idx);
        };

        for (int y = 0; y < dstHeight; y++)
        {
            for (int x = 0; x < dstWidth; x++)
            {
                dst[y * dstWidth + x] = getSrcRectPixelByUV((float)x / dstWidth, (float)y / dstHeight);
            }
        }
    }
}

uint32_t ImageUtils::GetColorFromConfidence(const float confidence)
{
    float bf = 0.0f;
    uint32_t colorA = 0;
    uint32_t colorB = 0;
    if (confidence < 0.5f)
    {
        bf = confidence * 2.0f;
        colorA = COLOR_RED;
        colorB = COLOR_YELLOW;
    }
    else
    {
        bf = (confidence - 0.5f) * 2.0f;
        colorA = COLOR_YELLOW;
        colorB = COLOR_GREEN;
    }

    uint8_t* a = (uint8_t*)&colorA;
    uint8_t* b = (uint8_t*)&colorB;
    uint8_t result[4];
    for (uint8_t i = 0; i < 4; ++i)
    {
        result[i] = (uint8_t)((1.0f - bf) * a[i] + bf * b[i]);
    }

    return *((uint32_t*)result);
}
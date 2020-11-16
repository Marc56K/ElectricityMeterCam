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

void ImageUtils::GetNormalizedPixels(const dl_matrix3du_t* src, const int x, const int y, const int w, const int h, float* dst)
{
    int dstIdx = 0;
    for (int _y = y; _y < y + h; _y++)
    {
        int offset = _y * src->w;
        for (int _x = x; _x < x + w; _x++)
        {
            int idx = (offset + _x) * 3;
            float r = src->item[idx + 0];
            float g = src->item[idx + 1];
            float b = src->item[idx + 2];
            dst[dstIdx++] = (r / 255.0f + g / 255.0f + b / 255.0f) / 3.0f;
        }
    }
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
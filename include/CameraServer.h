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
#include "SDCard.h"
#include "esp_camera.h"
#include "fb_gfx.h"
#include "fr_forward.h"
#include <Arduino.h>
#include "ImageUtils.h"
#include "esp_http_server.h"
#include "camera_pins.h"
#include <sstream>
#include <functional>
#include "Settings.h"

struct KwhInfo
{
    float kwh;
    float confidence;
    unsigned long unixtime;

    KwhInfo() :
        kwh(0), confidence(0), unixtime(0)
    {
    }
};

class CameraServer
{
public:
    CameraServer(Settings& settings);
    ~CameraServer();

    bool StartServer();
    bool InitCamera(const bool flipImage);

    dl_matrix3du_t* CaptureFrame(const unsigned long timestamp, SDCard* sdCard = nullptr);
    void SwapBuffers();
    
    void SetLatestKwh(const KwhInfo& info);

    esp_err_t HttpGetIndex(httpd_req_t *req);
    esp_err_t HttpGetLive(httpd_req_t *req);
    esp_err_t HttpGetImage(httpd_req_t *req);
    esp_err_t HttpGetKwh(httpd_req_t *req);
    esp_err_t HttpGetBBoxes(httpd_req_t *req);
    esp_err_t HttpPostBBoxes(httpd_req_t *req);

    bool UserConnected();

private:
    Settings& _settings;
    dl_matrix3du_t* _frontRgbBuffer;
    dl_matrix3du_t* _backRgbBuffer;
    uint32_t _numCapturedFrames;
    uint32_t _numStoredFrames;
    httpd_handle_t _httpServer;
    SemaphoreHandle_t _httpSemaphore;
    dl_matrix3du_t* _httpFrontRgbBuffer;
    KwhInfo _latestKwhInfo;
    unsigned long _lastUserInteraction;
};
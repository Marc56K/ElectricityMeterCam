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

#include "CameraServer.h"
#include "index.h.html"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

CameraServer::CameraServer(Settings& settings) :
    _settings(settings),
    _frontRgbBuffer(nullptr),
    _backRgbBuffer(nullptr),
    _numCapturedFrames(0),
    _numStoredFrames(0),
    _httpServer(nullptr),
    _httpFrontRgbBuffer(nullptr),
    _lastUserInteraction(0)
{
    _httpSemaphore = xSemaphoreCreateMutex();
}

CameraServer::~CameraServer()
{
    if (_httpServer != nullptr)
        httpd_stop(_httpServer);

    if (_frontRgbBuffer != nullptr)
        dl_matrix3du_free(_frontRgbBuffer);

    if (_backRgbBuffer != nullptr)
        dl_matrix3du_free(_backRgbBuffer);
}

bool CameraServer::StartServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = 1;
    config.server_port = 80;

    Serial.printf("Starting http server on port: '%d'\n", config.server_port);
    if (httpd_start(&_httpServer, &config) == ESP_OK)
    {
        httpd_uri_t indexUri = 
        {
            .uri = "/",
            .method = HTTP_GET,
            .handler = [](httpd_req_t *req) -> esp_err_t { return ((CameraServer*)req->user_ctx)->HttpGetIndex(req); },
            .user_ctx = this
        };
        httpd_register_uri_handler(_httpServer, &indexUri);

        httpd_uri_t imageUri = 
        {
            .uri = "/image",
            .method = HTTP_GET,
            .handler = [](httpd_req_t *req) -> esp_err_t { return ((CameraServer*)req->user_ctx)->HttpGetImage(req); },
            .user_ctx = this
        };
        httpd_register_uri_handler(_httpServer, &imageUri);

        httpd_uri_t liveUri = 
        {
            .uri = "/live",
            .method = HTTP_GET,
            .handler = [](httpd_req_t *req) -> esp_err_t { return ((CameraServer*)req->user_ctx)->HttpGetLive(req); },
            .user_ctx = this
        };
        httpd_register_uri_handler(_httpServer, &liveUri);

        httpd_uri_t kwhUri = 
        {
            .uri = "/kwh",
            .method = HTTP_GET,
            .handler = [](httpd_req_t *req) -> esp_err_t { return ((CameraServer*)req->user_ctx)->HttpGetKwh(req); },
            .user_ctx = this
        };
        httpd_register_uri_handler(_httpServer, &kwhUri);

        httpd_uri_t getbboxesUri = 
        {
            .uri = "/getbboxes",
            .method = HTTP_GET,
            .handler = [](httpd_req_t *req) -> esp_err_t { return ((CameraServer*)req->user_ctx)->HttpGetBBoxes(req); },
            .user_ctx = this
        };
        httpd_register_uri_handler(_httpServer, &getbboxesUri);

        httpd_uri_t setbboxesUri = 
        {
            .uri = "/setbboxes",
            .method = HTTP_POST,
            .handler = [](httpd_req_t *req) -> esp_err_t { return ((CameraServer*)req->user_ctx)->HttpPostBBoxes(req); },
            .user_ctx = this
        };
        httpd_register_uri_handler(_httpServer, &setbboxesUri);

        return true;
    }

    return false;
}

bool CameraServer::InitCamera(const bool flipImage)
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 10000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 8;
    config.fb_count = 1;

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        Serial.println();
        return false;
    }

    if (flipImage)
    {
        sensor_t *s = esp_camera_sensor_get();
        s->set_vflip(s, 1);   //flip vertically
        s->set_hmirror(s, 1); //flip horizontally
    }

    return true;
}

dl_matrix3du_t* CameraServer::CaptureFrame(const unsigned long timestamp, SDCard* sdCard)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == nullptr)
    {
        Serial.println("Camera capture failed");
        return nullptr;
    }

    _numCapturedFrames++;

    if (sdCard != nullptr && sdCard->IsMounted())
    {
        File file;
        if (sdCard->OpenFileForWriting(String("/") + timestamp + ".jpg", file))
        {
            file.write(fb->buf, fb->len);
            file.close();
            _numStoredFrames++;
        }
    }

    if (_backRgbBuffer == nullptr)
    {
        _backRgbBuffer = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
        memset(_backRgbBuffer->item, 255, _backRgbBuffer->stride * _backRgbBuffer->h);

        _frontRgbBuffer = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
        memset(_frontRgbBuffer->item, 255, _frontRgbBuffer->stride * _frontRgbBuffer->h);        
        ImageUtils::DrawText(5, 5, COLOR_RED, String("stay tuned ..."), _frontRgbBuffer);
        xSemaphoreTakeRecursive(_httpSemaphore, portMAX_DELAY);
        {
            _httpFrontRgbBuffer = _frontRgbBuffer;
        }
        xSemaphoreGiveRecursive(_httpSemaphore);
    }

    bool rgbValid = true;
    if (!fmt2rgb888(fb->buf, fb->len, fb->format, _backRgbBuffer->item))
    {
        rgbValid = false;
        Serial.println("fmt2rgb888 failed");
    }
    else
    {
        // frame number in upper left corner
        ImageUtils::DrawText(5, 5, COLOR_RED, String("") + _numCapturedFrames, _backRgbBuffer);
        
        // sd card infos on the bottom
        if (sdCard != nullptr && sdCard->IsMounted())
        {
            ImageUtils::DrawText(160, 210, COLOR_TURQUOISE, String("SD:") + (int)(sdCard->GetFreeSpaceInBytes() / 1024 / 1024) + "MB", _backRgbBuffer);
        }

        if (_numStoredFrames > 0)
        {
            ImageUtils::DrawText(5, 210, COLOR_TURQUOISE, String("#imgs:") + _numStoredFrames, _backRgbBuffer);
        }
    }

    if (fb != nullptr)
    {
        esp_camera_fb_return(fb);
    }

    if (!rgbValid)
    {
        return nullptr;
    }

    return _backRgbBuffer;
}

void CameraServer::SwapBuffers()
{
    xSemaphoreTakeRecursive(_httpSemaphore, portMAX_DELAY);
    {
        std::swap(_frontRgbBuffer, _backRgbBuffer);
        _httpFrontRgbBuffer = _frontRgbBuffer;
    }
    xSemaphoreGiveRecursive(_httpSemaphore);
}

void CameraServer::SetLatestKwh(const KwhInfo& info)
{
    xSemaphoreTakeRecursive(_httpSemaphore, portMAX_DELAY);
    {
        _latestKwhInfo = info;
    }
    xSemaphoreGiveRecursive(_httpSemaphore);
}

esp_err_t CameraServer::HttpGetIndex(httpd_req_t *req)
{
    _lastUserInteraction = millis();
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

esp_err_t CameraServer::HttpGetLive(httpd_req_t *req)
{
    _lastUserInteraction = millis();
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = nullptr;
    char *part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
    {
        return res;
    }

    while (true)
    {
        if (_httpFrontRgbBuffer == nullptr)
        {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        }
        else
        {
            xSemaphoreTakeRecursive(_httpSemaphore, portMAX_DELAY);
            {
                Serial.println("Compressing RGB to JPEG");
                static uint32_t imgIdx = 0;
                imgIdx++;

                ImageUtils::DrawRect(
                    0, 0, _httpFrontRgbBuffer->w, _httpFrontRgbBuffer->h,
                    imgIdx % 2 == 0 ? COLOR_RED : COLOR_BLACK, _httpFrontRgbBuffer);

                if (!fmt2jpg(
                        _httpFrontRgbBuffer->item, 
                        _httpFrontRgbBuffer->stride * _httpFrontRgbBuffer->h,
                        _httpFrontRgbBuffer->w,
                        _httpFrontRgbBuffer->h,
                        PIXFORMAT_RGB888,
                        80, &_jpg_buf, &_jpg_buf_len))
                {
                    Serial.println("JPEG compression failed");
                    res = ESP_FAIL;
                }
            }
            xSemaphoreGiveRecursive(_httpSemaphore);
        }

        if (res == ESP_OK)
        {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }

        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }

        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        if (_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = nullptr;
        }

        if (res != ESP_OK)
        {
            break;
        }

        //Serial.printf("MJPG: %uB\n", (uint32_t)(_jpg_buf_len));
    }

    return res;
}

esp_err_t CameraServer::HttpGetImage(httpd_req_t *req)
{
    _lastUserInteraction = millis();
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = nullptr;

    res = httpd_resp_set_type(req, "image/jpeg");
    if (res != ESP_OK)
    {
        return res;
    }

    if (_httpFrontRgbBuffer == nullptr)
    {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        res = ESP_FAIL;
    }
    else
    {
        xSemaphoreTakeRecursive(_httpSemaphore, portMAX_DELAY);
        {
            Serial.println("Compressing RGB to JPEG");

            if (!fmt2jpg(
                    _httpFrontRgbBuffer->item, 
                    _httpFrontRgbBuffer->stride * _httpFrontRgbBuffer->h,
                    _httpFrontRgbBuffer->w,
                    _httpFrontRgbBuffer->h,
                    PIXFORMAT_RGB888,
                    80, &_jpg_buf, &_jpg_buf_len))
            {
                Serial.println("JPEG compression failed");
                res = ESP_FAIL;
                httpd_resp_send_500(req);
            }
        }
        xSemaphoreGiveRecursive(_httpSemaphore);
    }

    if (res == ESP_OK)
    {
        res = httpd_resp_send(req, (const char *)_jpg_buf, _jpg_buf_len);
    }

    if (_jpg_buf)
    {
        free(_jpg_buf);
        _jpg_buf = nullptr;
    }

    return res;
}

esp_err_t CameraServer::HttpGetKwh(httpd_req_t *req)
{
    String str;
    xSemaphoreTakeRecursive(_httpSemaphore, portMAX_DELAY);
    {
        str += _latestKwhInfo.unixtime;
        str += " ";
        str += String(_latestKwhInfo.kwh, 1);
        str += " ";
        str += _latestKwhInfo.confidence;
    }
    xSemaphoreGiveRecursive(_httpSemaphore);

    httpd_resp_send(req, str.c_str(), str.length());

    return ESP_OK;
}

esp_err_t CameraServer::HttpGetBBoxes(httpd_req_t *req)
{
    String str = _settings.GetDigitBBoxesAsJson();
    httpd_resp_set_type(req, HTTPD_TYPE_JSON);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, str.c_str(), str.length());
    return ESP_OK;
}

esp_err_t CameraServer::HttpPostBBoxes(httpd_req_t *req)
{
    _lastUserInteraction = millis();
    httpd_resp_set_type(req, HTTPD_TYPE_TEXT);

    std::vector<char> buffer(req->content_len);
    if (buffer.size() >= req->content_len)
    {
        httpd_req_recv(req, buffer.data(), req->content_len);
        if (_settings.SetDigitBBoxesFromJson(buffer.data()))
        {
            _settings.Save();
            httpd_resp_send(req, "", 0);
            return ESP_OK;
        }
    }
    httpd_resp_send_500(req);
    return ESP_FAIL;
}

bool CameraServer::UserConnected()
{
    return millis() - _lastUserInteraction < 300000;
}
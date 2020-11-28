#pragma once
#include "SDCard.h"
#include "esp_camera.h"
#include "fb_gfx.h"
#include "fr_forward.h"

class CameraServer
{
public:
    CameraServer();
    ~CameraServer();

    bool StartServer();
    bool InitCamera(const bool flipImage);
    dl_matrix3du_t* CaptureFrame(SDCard* sdCard = nullptr);

    void SetLatestKwh(float kwh);

private:
    dl_matrix3du_t* _frontRgbBuffer;
    dl_matrix3du_t* _backRgbBuffer;
};
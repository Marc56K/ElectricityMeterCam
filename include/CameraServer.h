#pragma once
#include "SDCard.h"
#include "esp_camera.h"
#include "fb_gfx.h"
#include "fr_forward.h"

struct KwhInfo
{
    float kwh;
    float confidence;
};

class CameraServer
{
public:
    CameraServer();
    ~CameraServer();

    bool StartServer();
    bool InitCamera(const bool flipImage);

    dl_matrix3du_t* CaptureFrame(SDCard* sdCard = nullptr);
    void SwapBuffers();
    
    void SetLatestKwh(const KwhInfo& info);

private:
    dl_matrix3du_t* _frontRgbBuffer;
    dl_matrix3du_t* _backRgbBuffer;
    uint32_t _numCapturedFrames;
    uint32_t _numStoredFrames;
};
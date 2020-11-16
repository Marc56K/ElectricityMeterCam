#pragma once
#include "esp_camera.h"

class CameraServer
{
public:
    CameraServer();
    ~CameraServer();

    bool InitCamera(const bool flipImage);
    bool StartServer();
};
#pragma once

#include <windows.h>

struct LightSettings
{
    float PosX = 2.0f;
    float PosY = 4.0f;
    float PosZ = -2.0f;

    float Ambient = 0.2f;
    float Diffuse = 1.0f;
    float Specular = 0.5f;
    float Shininess = 32.0f;
};

bool ShowLightDialog(HWND parentHwnd, LightSettings& settings);
#pragma once
#include <cstdint>
#include <windows.h>

#include <engine/engine_export.hpp>

namespace engine {


using WindowResizedCallback = void(int newWidth, int newHeight);

//Some shitty window init, rewrite
//only one window could be handled
ENGINE_API HWND makeWindow(std::uint32_t screenWidth, uint32_t screenHeight, WindowResizedCallback * resizedCallback);

}

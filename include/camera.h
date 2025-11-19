#pragma once
#include "lib.h"
#include "global.h"

void startCamera(std::shared_ptr<websocket::stream<tcp::socket>> ws_ptr);
void stopCamera();

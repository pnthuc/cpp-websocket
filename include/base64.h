// base64.h
#pragma once
#include <string>

std::string base64_encode(const unsigned char* data, size_t len);
std::string base64_encode(const char* data, size_t len);

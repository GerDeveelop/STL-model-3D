#pragma once
#include <map>
#include <span>
#include <stop_token>
#include <string>
#include <vector>
namespace stlcut {
// Selected 3MF XML/config parts, read in memory with CRC and size validation.
std::map<std::string,std::vector<unsigned char>> readAppearanceParts(std::span<const unsigned char> data,std::stop_token stop = {});
}

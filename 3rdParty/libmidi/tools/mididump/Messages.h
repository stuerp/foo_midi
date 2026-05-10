
/** $VER: Messages.h (2026.05.10) P. Stuer **/

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

extern std::string DescribeDrumSet(uint8_t drumKit, bool isGS) noexcept;
extern std::string DescribeDrum(uint8_t drumKit, uint8_t programNumber, bool isGS) noexcept;
extern std::string DescribeControlChange(uint8_t d1, uint8_t d2) noexcept;

extern const std::vector<const char *> Instruments;

#pragma once

#include <string>

#include <MIDIProcessor.h>

extern std::string DescribeControlChange(uint8_t d1, uint8_t d2) noexcept;

extern const std::vector<const char *> Instruments;
extern const std::unordered_map<int, const char *> Percussions;

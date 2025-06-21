
/** $VER: Channels.cpp (2025.06.21) **/

#include "framework.h"

#include "Channels.h"

const uint64_t DefEnabledChannels[32] = { ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull, ~0ull };

cfg_var_modern::cfg_blob CfgEnabledChannels({ 0x813ffb7a,0x59fc,0x4e19,{0x9b,0x8d,0x7a,0x4e,0xeb,0x2d,0x8b,0xca } }, DefEnabledChannels, sizeof(DefEnabledChannels));

channels_t CfgChannels;

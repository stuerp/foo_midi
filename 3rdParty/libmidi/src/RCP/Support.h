
/** $VER: Support.h (2025.03.21) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#pragma once

#include "pch.h"

#include "MIDI.h"

namespace rcp
{

#pragma warning(disable: 4820) // x bytes padding added after data member 'y'

uint16_t GetTrimmedLength(const char * data, uint16_t size, char trimChar, bool leaveLast);
uint32_t BPM2Ticks(uint16_t bpm, uint8_t scale);

const wchar_t * GetFileName(const wchar_t * filePath);

struct buffer_t
{
    uint8_t * Data;
    uint32_t Size;

    buffer_t() : Data(), Size() { }

    buffer_t(uint32_t size)
    {
        Data = (uint8_t *) ::malloc(size);
        Size = size;
    }

    buffer_t & operator =(const buffer_t & other)
    {
        Copy(other.Data, other.Size);

        return *this;
    }

    virtual ~buffer_t()
    {
        Reset();
    }

    void Copy(const void * data, size_t size)
    {
        if ((data == nullptr) || (size == 0))
            return;

        if (Data != nullptr)
            ::free(Data);

        Data = (uint8_t *) ::malloc(size);
        Size = (uint32_t) size;

        if (Data != nullptr)
            ::memcpy(Data, data, Size);
    }

    void Grow(uint32_t size)
    {
        void * NewData = ::realloc(Data, size);

        if (NewData != nullptr)
        {
            Data = (uint8_t *) NewData;
            Size = size;
        }
    }

    void ReadFile(const wchar_t * filePath)
    {
        FILE * fp = nullptr;

        if (::_wfopen_s(&fp, filePath, L"rb") != 0)
            throw std::runtime_error(std::format("Failed to open \"{}\" for reading: error {}", msc::WideToUTF8(filePath).c_str(), errno));

        if (fp != nullptr)
        {
            ::fseek(fp, 0, SEEK_END);

            Size = (uint32_t) std::min(::ftell(fp), (long) 0x100000);

            ::fseek(fp, 0, SEEK_SET);

            Data = (uint8_t *) ::malloc(Size);

            if (Data != nullptr)
                ::fread(Data, 1, Size, fp);

            ::fclose(fp);
        }
    }

    void WriteFile(const wchar_t * filePath) const
    {
        FILE * fp = nullptr;

        if (::_wfopen_s(&fp, filePath, L"wb") != 0)
            throw std::runtime_error(std::format("Failed to open \"{}\" for writing: error {}", msc::WideToUTF8(filePath).c_str(), errno));

        if (fp != nullptr)
        {
            ::fwrite(Data, 1, Size, fp);

            ::fclose(fp);
        }
    }

 private:
    void Reset()
    {
        if (Data != nullptr)
        {
            ::free(Data);

            Data = nullptr;
            Size = 0;
        }
    }
};

/// <summary>
/// 
/// </summary>
inline uint32_t MulDivCeil(uint32_t val, uint32_t mul, uint32_t div)
{
    return (uint32_t) (((uint64_t) val * mul + div - 1) / div);
}

/// <summary>
/// 
/// </summary>
inline uint32_t MulDivRound(uint32_t val, uint32_t mul, uint32_t div)
{
    return (uint32_t) (((uint64_t) val * mul + div / 2) / div);
}

/// <summary>
/// 
/// </summary>
inline uint16_t ReadLE16(const uint8_t * data)
{
    return (uint16_t) ((data[1] << 8) | (data[0] << 0));
}

/// <summary>
/// 
/// </summary>
inline uint32_t ReadLE32(const uint8_t * data)
{
    return (uint32_t) ((data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0] << 0));
}

}

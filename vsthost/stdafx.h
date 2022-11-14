// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#pragma once

#pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

// TODO: reference additional headers your program requires here

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <commctrl.h>

#include <stdint.h>

#include <fcntl.h>
#include <io.h>

#include <vector>

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

template <typename T>
static void append_be(std::vector<uint8_t> & out, const T & value)
{
    union
    {
        T original;
        uint8_t raw[sizeof(T)];
    } carriage;

    carriage.original = value;

    for (unsigned i = 0; i < sizeof(T); ++i)
    {
        out.push_back(carriage.raw[sizeof(T) - 1 - i]);
    }
}

template <typename T>
static void retrieve_be(T & out, const uint8_t *& in, unsigned & size)
{
    if (size < sizeof(T))
        return;

    size -= sizeof(T);

    union
    {
        T original;
        uint8_t raw[sizeof(T)];
    } carriage;

    carriage.raw[0] = 0;

    for (unsigned i = 0; i < sizeof(T); ++i)
    {
        carriage.raw[sizeof(T) - 1 - i] = *in++;
    }

    out = carriage.original;
}

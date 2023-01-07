/* vim: set et ts=3 sw=3 sts=3 ft=c:
 *
 * Copyright (C) 2016 Christopher Snowhill.  All rights reserved.
 * https://github.com/kode54/sflist
 * https://gist.github.com/kode54/a7bb01a0db3f2e996145b77f0ca510d5
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 4625 4626 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <json-builder.h>

#include "SFList.h"

#define json_compare_invalid -1000

static bool AreValuesEqual(const json_value * a, const json_value * b);

static bool AreArrayEqual(const json_value * a, const json_value * b)
{
    if (a->u.array.length != b->u.array.length)
        return false;

    for (unsigned i = 0; i < a->u.array.length; ++i)
    {
        if (!AreValuesEqual(a->u.array.values[i], b->u.array.values[i]))
            return false;
    }

    return true;
}

static bool AreObjectsEqual(const json_value * a, const json_value * b)
{
    if (a->u.object.length != b->u.object.length)
        return false;

    for (unsigned int i = 0; i < a->u.object.length; ++i)
    {
        if (::strcmp(a->u.object.values[i].name, b->u.object.values[i].name) != 0)
            return false;

        if (!AreValuesEqual(a->u.object.values[i].value, b->u.object.values[i].value))
            return false;
    }

    return true;
}

static bool AreValuesEqual(const json_value * a, const json_value * b)
{
    if (a->type != b->type)
        return false;

    switch (a->type)
    {
        case json_none:
        case json_null:
            return true;

        case json_integer:
            return a->u.integer == b->u.integer;

        case json_double:
            return a->u.dbl == b->u.dbl;

        case json_boolean:
            return ~a->u.boolean == ~b->u.boolean;

        case json_string:
            return ::strcmp(a->u.string.ptr, b->u.string.ptr) == 0;

        case json_array:
            return AreArrayEqual(a, b);

        case json_object:
            return AreObjectsEqual(a, b);
    }

    return 0;
}

static int CompareSignum(double val)
{
    return (val > 0.0) - (val < 0.0);
}

static bool ArrayContains(const json_value * array, const json_value * value)
{
    for (unsigned i = 0; i < array->u.array.length; ++i)
        if (AreValuesEqual(array->u.array.values[i], value))
            return true;

    return false;
}

json_value * MergeArray(json_value * a, json_value * b)
{
    if (a->type != json_array || b->type != json_array)
        return nullptr;

    for (unsigned int i = 0; i < b->u.array.length; ++i)
    {
        if (!ArrayContains(a, b->u.array.values[i]))
            ::json_array_push(a, b->u.array.values[i]);
    }

    ::json_builder_free(b);

    return a;
}

static int CompareValues(const json_value * a, const json_value * b)
{
    if (a->type != b->type)
        return json_compare_invalid;

    switch (a->type)
    {
        case json_none:
        case json_null:
            return 0;

        case json_integer:
            return (int) (a->u.integer - b->u.integer);

        case json_double:
            return CompareSignum(a->u.dbl - b->u.dbl);

        case json_boolean:
            return !!a->u.boolean - !!b->u.boolean;

        case json_string:
            return ::strcmp(a->u.string.ptr, b->u.string.ptr);

        case json_array:
        case json_object:
            return json_compare_invalid;
    }

    return json_compare_invalid;
}

static int CompareValues(const void * a, const void * b) noexcept
{
    const json_value * x = (const json_value *) a;
    const json_value * y = (const json_value *) b;

    return CompareValues(x, y);
}

json_value * SortArray(json_value * array)
{
    if (array->type != json_array)
        return nullptr;

    if (array->u.array.length < 2)
        return array;

    // Verify that all values have the same type.
    json_type ValueType = array->u.array.values[0]->type;

    for (unsigned int i = 1; i < array->u.array.length; ++i)
    {
        if (array->u.array.values[i]->type != ValueType)
            return 0;
    }

    ::qsort(array->u.array.values, array->u.array.length, sizeof(json_value *), CompareValues);

    return array;
}

static size_t GetInteger(const char * data, const char ** tail)
{
    if ((data == nullptr) || (tail == nullptr))
        return 0;

    unsigned int Value = 0;

    while (data < *tail)
    {
        if (::isdigit(*data))
            Value = (Value * 10) + (*data - '0');
        else
            break;

        ++data;
    }

    *tail = data;

    return Value;
}

static double GetDouble(const char * data, const char ** tail)
{
    if ((data == nullptr) || (tail == nullptr))
        return 0.0f;

    const char * Tail = *tail;
    const char * Curr = data;

    double Sign = 1.0f;

    if (*Curr == '-')
    {
        ++Curr;
        Sign = -1.0f;
    }

    size_t Whole = GetInteger(Curr, tail);

    if (*tail == Curr || ((**tail != '.') && (*tail < Tail)))
    {
        *tail = data;
        return 0.0;;
    }

    size_t Decimal = 0;
    size_t DecimalPlaces = 0;

    if (*tail < Tail)
    {
        Curr = *tail + 1;
        *tail = Tail;

        Decimal = GetInteger(Curr, tail);

        if (*tail == Curr || *tail < Tail)
        {
            *tail = data;

            return 0.0f;
        }

        DecimalPlaces = (size_t)(*tail - Curr);
    }

    return (((double) Whole) + (((double) Decimal) / pow(10.0, (double) DecimalPlaces))) * Sign;
}

static json_value * SFListLoadv1(const char * data, size_t size, char * errorMessage)
{
    json_value * Array = ::json_array_new(0);

    json_value * Channels = nullptr;
    json_value * PatchMappings = nullptr;
    double Gain = 0.0;

    const char * Curr = data;
    const char * Tail = data + size;

    unsigned int LineCount = 0;

    while (Curr < Tail)
    {
        const char * LineHead = Curr;
        const char * LineTail = Curr;

        const char * Path = nullptr;
        const char * Pipe = nullptr;

        ++LineCount;

        while ((LineTail < Tail) && *LineTail && (*LineTail != '\r') && (*LineTail != '\n'))
        {
            if (*LineTail == '|')
                Pipe = LineTail;

            ++LineTail;
        }

        if (Pipe)
            Path = Pipe + 1;
        else
            Path = Curr;

        if (Pipe)
        {
            while (Curr < Pipe)
            {
                const char * fend = Curr;
                const char * vend;

                while (fend < Pipe && *fend != '&')
                    ++fend;

                vend = fend;

                switch (char c = *Curr++)
                {
                    case '&':
                        continue;

                    case 'c':
                    {
                        json_value * this_channels;

                        size_t channel_low = GetInteger(Curr, &vend);
                        size_t channel_high = 0;

                        size_t i;

                        if (vend == Curr || (*vend != '-' && *vend != '&' && *vend != '|'))
                        {
                            ::sprintf_s(errorMessage, sflist_max_error, "Invalid channel number (%u:%u)", LineCount, (unsigned int) (vend - LineHead + 1));
                            goto error;
                        }

                        if (*vend != '-')
                            channel_high = channel_low;
                        else
                        {
                            Curr = vend + 1;
                            vend = fend;

                            channel_high = GetInteger(Curr, &vend);

                            if (vend == Curr || (*vend != '&' && *vend != '|'))
                            {
                                ::sprintf_s(errorMessage, sflist_max_error, "Invalid channel range end value (%u:%u)", LineCount, (unsigned int) (vend - LineHead + 1));
                                goto error;
                            }
                        }

                        if (!Channels)
                            Channels = ::json_array_new(0);

                        this_channels = ::json_array_new(0);

                        for (i = channel_low; i <= channel_high; ++i)
                            ::json_array_push(this_channels, ::json_integer_new((__int64)i));

                        Channels = MergeArray(Channels, this_channels);
                        Curr = fend;
                    }
                    break;

                    case 'p':
                    {
                        json_value * mapping = nullptr;
                        json_value * mapping_destination = nullptr;
                        json_value * mapping_source = nullptr;

                        long source_bank = -1;
                        long source_program = -1;
                        long dest_bank = -1;
                        long dest_program = -1;

                        size_t val = GetInteger(Curr, &vend);

                        if (vend == Curr || (*vend != '=' && *vend != ',' && *vend != '|'))
                        {
                            ::sprintf_s(errorMessage, sflist_max_error, "Invalid preset number (%u:%u)", LineCount, (unsigned int)(vend - LineHead + 1));
                            goto error;
                        }

                        dest_program = (long)val;

                        if (*vend == ',')
                        {
                            dest_bank = (long)val;
                            Curr = vend + 1;
                            vend = fend;
                            val = GetInteger(Curr, &vend);

                            if (vend == Curr || (*vend != '=' && *vend != '|'))
                            {
                                ::sprintf_s(errorMessage, sflist_max_error, "Invalid preset number (%u:%u)", LineCount, (unsigned int) (vend - LineHead + 1));
                                goto error;
                            }

                            dest_program = (long)val;
                        }

                        if (*vend == '=')
                        {
                            Curr = vend + 1;
                            vend = fend;
                            val = GetInteger(Curr, &vend);

                            if (vend == Curr || (*vend != ',' && *vend != '|'))
                            {
                                ::sprintf_s(errorMessage, sflist_max_error, "Invalid preset number (%u:%u)", LineCount, (unsigned int) (vend - LineHead + 1));
                                goto error;
                            }

                            source_program = (long)val;

                            if (*vend == ',')
                            {
                                source_bank = (long)val;
                                Curr = vend + 1;
                                vend = fend;
                                val = GetInteger(Curr, &vend);

                                if (vend == Curr || (*vend != '&' && *vend != '|'))
                                {
                                    ::sprintf_s(errorMessage, sflist_max_error, "Invalid preset number (%u:%u)", LineCount, (unsigned int) (vend - LineHead + 1));
                                    goto error;
                                }
                                source_program = (long)val;
                            }
                        }

                        if (PatchMappings == nullptr)
                            PatchMappings = ::json_array_new(0);

                        mapping = ::json_object_new(0);
                        mapping_destination = ::json_object_new(0);

                        if (dest_bank != -1)
                            ::json_object_push(mapping_destination, "bank", ::json_integer_new(dest_bank));

                        ::json_object_push(mapping_destination, "program", ::json_integer_new(dest_program));
                        ::json_object_push(mapping, "destination", mapping_destination);

                        if (source_program != -1)
                        {
                            mapping_source = json_object_new(0);

                            if (source_bank != -1)
                            {
                                json_object_push(mapping_source, "bank", json_integer_new(source_bank));
                            }

                            ::json_object_push(mapping_source, "program", ::json_integer_new(source_program));
                            ::json_object_push(mapping, "source", mapping_source);
                        }

                        ::json_array_push(PatchMappings, mapping);

                        Curr = fend;
                    }
                    break;

                    case 'g':
                    {
                        double val = GetDouble(Curr, &vend);

                        if (vend == Curr || vend < fend)
                        {
                            ::sprintf_s(errorMessage, sflist_max_error, "Invalid gain value (%u:%u)", LineCount, (unsigned int)(vend - LineHead + 1));
                            goto error;
                        }

                        Gain = val;

                        Curr = fend;
                    }
                    break;

                    default:
                        ::sprintf_s(errorMessage, sflist_max_error, "Invalid character in preset '%c' (%u:%u)", c, LineCount, (unsigned int)(Curr - LineHead));
                        goto error;
                }
            }
        }

        {
            json_value * Object = ::json_object_new(0);

            ::json_object_push(Object, "fileName", json_string_new_length((unsigned int)(LineTail - Path), Path));

            {
                if (Gain != 0.0)
                {
                    ::json_object_push(Object, "gain", ::json_double_new(Gain));
                    Gain = 0.0;
                }
            }

            if (Channels)
            {
                Channels = SortArray(Channels);

                ::json_object_push(Object, "channels", Channels);

                Channels = nullptr;
            }

            if (PatchMappings)
            {
                json_object_push(Object, "patchMappings", PatchMappings);

                PatchMappings = nullptr;
            }

            ::json_array_push(Array, Object);
        }

        Curr = LineTail;

        while (Curr < Tail && (*Curr == '\n' || *Curr == '\r'))
            ++Curr;
    }

    json_value * rval = json_object_new(1);

    json_object_push(rval, "soundFonts", Array);

    return rval;

error:
    if (Channels)
        ::json_builder_free(Channels);

    if (PatchMappings)
        ::json_builder_free(PatchMappings);

    if (Array)
        ::json_builder_free(Array);

    return nullptr;
}

static json_value * SFListLoadv2(const char * data, size_t size, char * errorMessage)
{
    json_settings Settings = { 0 };

    Settings.value_extra = json_builder_extra;

    json_value * Values = json_parse_ex(&Settings, data, size, errorMessage);

    return Values;
}

static const json_value * GetItem(const json_value * object, const char * name)
{
    if (object->type != json_object)
        return &json_value_none;

    for (unsigned i = 0;  i < object->u.object.length; ++i)
    {
        if (::strcmp(object->u.object.values[i].name, name) == 0)
            return object->u.object.values[i].value;
    }

    return &json_value_none;
}

static void ProcessPatchMappings(BASS_MIDI_FONTEX * out, BASS_MIDI_FONTEX * fontex, const json_value * patchMappings, unsigned int channel)
{
    for (unsigned int i = 0; i < patchMappings->u.array.length; ++i)
    {
        json_value * Preset = patchMappings->u.array.values[i];

        const json_value * Src = GetItem(Preset, "source");

        const json_value * SrcBank = GetItem(Src, "bank");
        const json_value * SrcProgram = GetItem(Src, "program");

        const json_value * Dst = GetItem(Preset, "destination");

        const json_value * DstBank = GetItem(Dst, "bank");
        const json_value * DstProgram = GetItem(Dst, "program");

        fontex->spreset = (SrcProgram->type == json_none) ? -1 : (int) SrcProgram->u.integer;
        fontex->sbank = (SrcBank->type == json_none) ? -1 : (int) SrcBank->u.integer;

        fontex->dpreset = (DstProgram->type == json_none) ? -1 : (int) DstProgram->u.integer;
        fontex->dbank = (DstBank->type == json_none) ? 0 : (int) DstBank->u.integer;

        fontex->dbanklsb = (int) channel;

        *out++ = *fontex;
    }
}

static PresetArray * CreatePresetArray(const json_value * rootItem, const char * basePath, char * errorMessage)
{
    HSOUNDFONT hfont = 0;

    PresetArray * presetArray = (PresetArray *)calloc(1, sizeof(PresetArray));

    if (presetArray == nullptr)
    {
        ::strcpy_s(errorMessage, sflist_max_error, "Out of memory");
        goto error;
    }

    if (rootItem->type != json_object)
    {
        ::strcpy_s(errorMessage, sflist_max_error, "Base JSON item is not an object");
        goto error;
    }

    if (rootItem->u.object.length != 1)
    {
        ::sprintf_s(errorMessage, sflist_max_error, "Base JSON object contains unexpected number of items (wanted 1, got %u)", rootItem->u.object.length);
        goto error;
    }

    if (::strcmp(rootItem->u.object.values[0].name, "soundFonts") != 0)
    {
        ::sprintf_s(errorMessage, sflist_max_error, "Base JSON object contains '%s' object instead of 'soundFonts'", rootItem->u.object.values[0].name);

        goto error;
    }

    const json_value * RootValue = rootItem->u.object.values[0].value;

    if (RootValue->type != json_array)
    {
        ::strcpy_s(errorMessage, sflist_max_error, "JSON 'soundFonts' object is not an array");
        goto error;
    }

    unsigned int RequiredPresetCount = 0;

    for (unsigned int i = 0; i < RootValue->u.array.length; ++i)
    {
        const json_value * Value = RootValue->u.array.values[i];

        if (Value->type != json_object)
        {
            ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u is not an object", i + 1);
            goto error;
        }

        {
            const json_value * path = GetItem(Value, "fileName");

            if (path->type == json_none)
            {
                ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u has no 'fileName'", i + 1);
                goto error;
            }

            if (path->type != json_string)
            {
                ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'fileName' is not a string", i + 1);
                goto error;
            }
        }

        {
            const json_value * gain = GetItem(Value, "gain");

            if (gain->type != json_none && gain->type != json_integer && gain->type != json_double)
            {
                ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u has an invalid gain value", i + 1);
                goto error;
            }
        }

        unsigned int NeededPatchCount = 1;

        {
            const json_value * channels = GetItem(Value, "channels");

            if (channels->type != json_none)
            {
                if (channels->type != json_array)
                {
                    ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'channels' is not an array", i + 1);
                    goto error;
                }

                unsigned int l = channels->u.array.length;

                for (unsigned int k = 0; k < l; ++k)
                {
                    json_value * channel = channels->u.array.values[k];

                    if (channel->type != json_integer)
                    {
                        ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'channels' #%u is not an integer", i + 1, k + 1);
                        goto error;
                    }

                    if (channel->u.integer < 1 || channel->u.integer > 48)
                    {
                        ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'channels' #%u is out of range (wanted 1-48, got %I64d)", i + 1, k + 1, channel->u.integer);
                        goto error;
                    }
                }

                NeededPatchCount = l;
            }
        }

        {
            const json_value * patchMappings = GetItem(Value, "patchMappings");

            if (patchMappings->type != json_none)
            {
                if (patchMappings->type != json_array)
                {
                    ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' is not an array", i + 1);
                    goto error;
                }

                unsigned int l = patchMappings->u.array.length;

                for (unsigned int k = 0; k < l; ++k)
                {
                    unsigned int m, n;
                    unsigned int source_found = 0;
                    unsigned int destination_found = 0;
                    json_value * mapping = patchMappings->u.array.values[k];

                    if (mapping->type != json_object)
                    {
                        ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u is not an object", i + 1, k + 1);
                        goto error;
                    }

                    for (m = 0, n = mapping->u.object.length; m < n; ++m)
                    {
                        unsigned int o, p;
                        json_value * item = mapping->u.object.values[m].value;
                        const char * name = mapping->u.object.values[m].name;
                        unsigned int bank_found = 0;
                        unsigned int program_found = 0;

                        if (strcmp(name, "source") && strcmp(name, "destination"))
                        {
                            ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u contains an invalid '%s' field", i + 1, k + 1, name);
                            goto error;
                        }

                        if (item->type != json_object)
                        {
                            ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u '%s' is not an object", i + 1, k + 1, name);
                            goto error;
                        }

                        if (!strcmp(name, "source"))
                            ++source_found;
                        else
                            ++destination_found;

                        for (o = 0, p = item->u.object.length; o < p; ++o)
                        {
                            int range_min = 0;
                            int range_max = 128;
                            json_value * item2 = item->u.object.values[o].value;
                            const char * name2 = item->u.object.values[o].name;

                            if (strcmp(name2, "bank") && strcmp(name2, "program"))
                            {
                                ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u '%s' contains an invalid '%s' field", i + 1, k + 1, name, name2);
                                goto error;
                            }

                            if (item2->type != json_integer)
                            {
                                ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u '%s' '%s' is not an integer", i + 1, k + 1, name, name2);
                            }

                            if (!strcmp(name2, "program"))
                            {
                                if (!strcmp(name, "destination"))
                                    range_max = 65535;
                                else
                                    range_max = 127;
                            }

                            if (item2->u.integer < range_min || item2->u.integer > range_max)
                            {
                                ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u '%s' '%s' is out of range (expected %d-%d, got %I64d)", i + 1, k + 1, name, name2, range_min, range_max, item->u.integer);
                                goto error;
                            }

                            if (!strcmp(name2, "bank"))
                                ++bank_found;
                            else
                                ++program_found;
                        }

                        if (!bank_found && !program_found)
                        {
                            ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u '%s' contains no 'bank' or 'program'", i + 1, k + 1, name);
                            goto error;
                        }

                        if (bank_found > 1)
                        {
                            ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u '%s' contains more than one 'bank'", i + 1, k + 1, name);
                            goto error;
                        }

                        if (program_found > 1)
                        {
                            ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u '%s' contains more than one 'program'", i + 1, k + 1, name);
                            goto error;
                        }
                    }

                    if (!destination_found)
                    {
                        ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u is missing 'destination'", i + 1, k + 1);
                        goto error;
                    }

                    if (destination_found > 1)
                    {
                        ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u contains more than one 'destination'", i + 1, k + 1);
                        goto error;
                    }

                    if (source_found > 1)
                    {
                        ::sprintf_s(errorMessage, sflist_max_error, "soundFont item #%u 'patchMappings' #%u contains more than one 'source'", i + 1, k + 1);
                    }
                }

                NeededPatchCount *= l;
            }
        }

        RequiredPresetCount += NeededPatchCount;
    }

    // Convert the values to presets.
    {
        presetArray->Count = RequiredPresetCount;
        presetArray->Items = (BASS_MIDI_FONTEX *)::calloc(sizeof(BASS_MIDI_FONTEX), presetArray->Count);

        if (presetArray->Items == nullptr)
        {
            ::strcpy_s(errorMessage, sflist_max_error, "Out of memory");
            goto error;
        }

        {
            unsigned int PresetIndex = 0;

            const char * BasePathTail = basePath + ::strlen(basePath) - 1;

            char PathName[MAX_PATH];

            for (size_t ItemIndex = RootValue->u.array.length, j = 0; ItemIndex--; ++j)
            {
                const json_value * Object = RootValue->u.array.values[ItemIndex];
        
                {
                    const json_value * FileNameItem = GetItem(Object, "fileName");

                    const char * FileNameValue = FileNameItem->u.string.ptr;

                    if (!(::isalpha(*FileNameValue) && FileNameValue[1] == ':'))
                    {
                        if (::strlen(FileNameValue) + (BasePathTail - basePath + 2) > 32767)
                        {
                            ::strcpy_s(errorMessage, sflist_max_error, "Base path plus SoundFont relative path is longer than 32767 characters");
                            goto error;
                        }

                        ::strcpy_s(PathName, _countof(PathName), basePath);

                        if (*BasePathTail != '\\' && *BasePathTail != '/')
                            ::strcat_s(PathName, _countof(PathName), "\\");

                        ::strcat_s(PathName, _countof(PathName), FileNameValue);

                        FileNameValue = PathName;
                    }

                    {
                        wchar_t PathNameW[MAX_PATH];

                        ::MultiByteToWideChar(CP_UTF8, 0, FileNameValue, -1, PathNameW, _countof(PathNameW));

                        PathNameW[MAX_PATH - 1] = '\0';

                        const void * BASSPathName = (void *) PathNameW;
                        unsigned int BASSFlags = BASS_UNICODE;

                        hfont = ::BASS_MIDI_FontInit(BASSPathName, BASSFlags);

                        if (hfont == 0)
                        {
                            int error_code = BASS_ErrorGetCode();

                            if (error_code == BASS_ERROR_FILEOPEN)
                            {
                                ::sprintf_s(errorMessage, sflist_max_error, "Could not open SoundFont bank '%s'", FileNameItem->u.string.ptr);
                                goto error;
                            }
                            else if (error_code == BASS_ERROR_FILEFORM)
                            {
                                ::sprintf_s(errorMessage, sflist_max_error, "SoundFont bank '%s' is not a supported format", FileNameItem->u.string.ptr);
                                goto error;
                            }
                            else
                            {
                                ::sprintf_s(errorMessage, sflist_max_error, "SoundFont bank '%s' failed to load with error #%u", FileNameItem->u.string.ptr, (unsigned int)error_code);
                                goto error;
                            }
                        }
                    }
                }

                {
                    const json_value * GainItem = GetItem(Object, "gain");

                    if (GainItem->type != json_none)
                    {
                        double d = 0.0f;

                        if (GainItem->type == json_integer)
                            d = (double) GainItem->u.integer;
                        else
                        if (GainItem->type == json_double)
                            d = GainItem->u.dbl;

                        d = ::pow(10.0f, d / 20.0f);

                        ::BASS_MIDI_FontSetVolume(hfont, (float)d);
                    }
                }

                {
                    BASS_MIDI_FONTEX FontEx = { 0 };

                    FontEx.font = hfont;
                    FontEx.spreset = -1;
                    FontEx.sbank = -1;
                    FontEx.dpreset = -1;
                    FontEx.dbank = 0;
                    FontEx.dbanklsb = 0;

                    /* Simplest case, whole bank loading */
                    const json_value * PatchMappingsItem = GetItem(Object, "patchMappings");
                    const json_value * ChannelsItem = GetItem(Object, "channels");

                    if (ChannelsItem->type == json_none && PatchMappingsItem->type == json_none)
                    {
                        if (PresetIndex < presetArray->Count)
                            presetArray->Items[PresetIndex++] = FontEx;
                    }
                    else
                    if (PatchMappingsItem->type == json_none)
                    {
                        for (unsigned int k = 0; k < ChannelsItem->u.array.length; ++k)
                        {
                            FontEx.dbanklsb = (int) ChannelsItem->u.array.values[k]->u.integer;

                            if (PresetIndex < presetArray->Count)
                                presetArray->Items[PresetIndex++] = FontEx;
                        }
                    }
                    else
                    if (ChannelsItem->type == json_none)
                    {
                        ProcessPatchMappings(presetArray->Items + PresetIndex, &FontEx, PatchMappingsItem, 0);
                        PresetIndex += PatchMappingsItem->u.array.length;
                    }
                    else
                    {
                        for (unsigned int k = 0; k < ChannelsItem->u.array.length; ++k)
                        {
                            ProcessPatchMappings(presetArray->Items + PresetIndex, &FontEx, PatchMappingsItem, (unsigned int) ChannelsItem->u.array.values[k]->u.integer);

                            PresetIndex += PatchMappingsItem->u.array.length;
                        }
                    }
                }
            }
        }

        return presetArray;
    }

error:
    if (hfont)
        ::BASS_MIDI_FontFree(hfont);

    if (presetArray)
        ::DeletePresetArray(presetArray);

    return nullptr;
}

static int strpbrkn_all(const char * data, size_t size, const char * characters)
{
    const char * Tail = data + size;

    while ((data < Tail) && *characters)
    {
        while (data < Tail && (*data != *characters))
            ++data;

        ++data, ++characters;
    }

    return data < Tail;
}

PresetArray * LoadPresetArray(const char * data, size_t size, const char * directoryPath, char * errorMessage)
{
    /* Handle Unicode byte order markers */
    if (size >= 2)
    {
        if ((data[0] == 0xFF && data[1] == 0xFE) || (data[0] == 0xFE && data[1] == 0xFF))
        {
            ::strcpy_s(errorMessage, sflist_max_error, "UTF-16 encoding is not supported at this time");

            return nullptr;
        }

        if ((size >= 3) && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        {
            data += 3;
            size -= 3;
        }
    }

    json_value * Values = SFListLoadv2(data, size, errorMessage);

    if (Values == nullptr)
    {
        if (!strpbrkn_all(data, size, "{[]}"))
            Values = SFListLoadv1(data, size, errorMessage);
    }

    if (Values == nullptr)
        return nullptr;

    PresetArray * Presets = CreatePresetArray(Values, directoryPath, errorMessage);

    ::json_builder_free(Values);

    return Presets;
}

void DeletePresetArray(PresetArray * presetArray)
{
    if (presetArray == nullptr)
        return;

    if (presetArray->Items)
    {
        for (size_t i = 0; i < presetArray->Count; ++i)
        {
            HSOUNDFONT hSoundFont = presetArray->Items[i].font;

            if (hSoundFont)
                ::BASS_MIDI_FontFree(hSoundFont);
        }

        ::free(presetArray->Items);
    }

    ::free(presetArray);
}

const char * UpgradeSFList(const char * data, size_t size, char * errorMessage)
{
    json_value * Values = SFListLoadv2(data, size, errorMessage);

    if (Values == nullptr)
    {
        if (!strpbrkn_all(data, size, "{[]}"))
            Values = SFListLoadv1(data, size, errorMessage);
    }

    if (Values == nullptr)
        return nullptr;

    const json_serialize_opts Options =
    {
       json_serialize_mode_multiline,
       0,
       3  /* indent_size */
    };

    size_t Size = json_measure_ex(Values, Options);

    char * Result = (char *) ::malloc(Size + 1);

    if (Result)
    {
        ::json_serialize_ex(Result, Values, Options);

        Result[Size] = '\0';
    }
    else
        ::strcpy_s(errorMessage, sflist_max_error, "Out of memory");

    if (Values)
        ::json_builder_free(Values);

    return (const char *) Result;
}

void FreeUpgradedSFList(const char * data)
{
    ::free((void *) data);
}

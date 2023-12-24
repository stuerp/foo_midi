
/** $VER: ConfigurationMap (2023.12.23) **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include <sdk/foobar2000-lite.h>
#include <sdk/cfg_var.h>

#include <map>

using namespace cfg_var_modern;

/// <summary>
/// Implements a configuration variable for maps.
/// </summary>
class cfg_map : public cfg_var_common, public std::map<uint32_t, std::vector<uint8_t>>
{
public:
    cfg_map(const GUID& guid) : cfg_var_common(guid), std::map<t_uint32, std::vector<uint8_t>>() { }
    cfg_map(const cfg_map&) = delete;
    cfg_map(const cfg_map&&) = delete;
    cfg_map& operator=(const cfg_map&) = delete;
    cfg_map& operator=(cfg_map&&) = delete;
    virtual ~cfg_map() { };

    #pragma region("cfg_var_writer")
    virtual void get_data_raw(stream_writer * streamWriter, abort_callback & handleAbort)
    {
        if (streamWriter == nullptr)
            return;

        stream_writer_formatter<> out(*streamWriter, handleAbort);

        out.write_int(size());

        for (auto it = begin(); it != end(); ++it)
        {
            out.write_int(it->first);

            const t_uint32 size = pfc::downcast_guarded<t_uint32>(it->second.size());

            out << size;

            for (t_uint32 walk = 0; walk < size; ++walk)
                out << it->second[walk];
        }
    }
    #pragma endregion

    #pragma region("cfg_var_reader")
    virtual void set_data_raw(stream_reader * streamReader, t_size, abort_callback & handleAbort)
    {
        if (streamReader == nullptr)
            return;

        stream_reader_formatter<> in(*streamReader, handleAbort);

        clear();

        t_size Count;

        in.read_int(Count);

        for (t_size i = 0; i < Count; ++i)
        {
            t_uint32 Key;

            in.read_int(Key);

            t_uint32 Size;

            in >> Size;

            std::vector<uint8_t> Value(Size);

            for (size_t j = 0; j < (size_t) Size; ++j)
                in >> Value[j];

            operator[](Key) = Value;
        }
    }
    #pragma endregion
};

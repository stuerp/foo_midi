
/** $VER: ConfigurationMap.h (2025.07.30) **/

#pragma once

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include <sdk/foobar2000-lite.h>
#include <sdk/cfg_var.h>

#include <map>

using namespace cfg_var_modern;

/// <summary>
/// Implements a configuration variable for maps that uses integer keys.
/// </summary>
class cfg_vsti_map : public cfg_var, public std::map<uint32_t, std::vector<uint8_t>>
{
public:
    cfg_vsti_map(const GUID & guid) noexcept : cfg_var(guid), std::map<uint32_t, std::vector<uint8_t>>() { }

    cfg_vsti_map(const cfg_vsti_map &) = delete;
    cfg_vsti_map(const cfg_vsti_map &&) = delete;
    cfg_vsti_map& operator=(const cfg_vsti_map &) = delete;
    cfg_vsti_map& operator=(cfg_vsti_map &&) = delete;

    virtual ~cfg_vsti_map() { };

    #pragma region cfg_var_writer

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

    #pragma region cfg_var_reader

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

/// <summary>
/// Implements a configuration variable for maps that uses string keys.
/// </summary>
class cfg_clap_map : public cfg_var, public std::map<std::string, std::vector<uint8_t>>
{
public:
    cfg_clap_map(const GUID & guid) noexcept : cfg_var(guid), std::map<std::string, std::vector<uint8_t>>() { }

    cfg_clap_map(const cfg_clap_map &) = delete;
    cfg_clap_map(const cfg_clap_map &&) = delete;
    cfg_clap_map & operator=(const cfg_clap_map &) = delete;
    cfg_clap_map & operator=(cfg_clap_map &&) = delete;

    virtual ~cfg_clap_map() { };

    #pragma region cfg_var_writer

    virtual void get_data_raw(stream_writer * streamWriter, abort_callback & handleAbort)
    {
        if (streamWriter == nullptr)
            return;

        stream_writer_formatter<> swf(*streamWriter, handleAbort);

        swf.write_int(size());

        for (auto it = begin(); it != end(); ++it)
        {
            swf.write_string(it->first.c_str());

            const t_uint32 Size = pfc::downcast_guarded<t_uint32>(it->second.size());

            swf << Size;

            for (size_t i = 0; i < (size_t) Size; ++i)
                swf << it->second[i];
        }
    }

    #pragma endregion

    #pragma region cfg_var_reader

    virtual void set_data_raw(stream_reader * streamReader, t_size, abort_callback & handleAbort)
    {
        if (streamReader == nullptr)
            return;

        stream_reader_formatter<> srf(*streamReader, handleAbort);

        clear();

        t_size Count;

        srf.read_int(Count);

        for (t_size i = 0; i < Count; ++i)
        {
            std::string Key = srf.read_string().c_str();

            t_uint32 Size;

            srf >> Size;

            std::vector<uint8_t> Value(Size);

            for (size_t j = 0; j < (size_t) Size; ++j)
                srf >> Value[j];

            operator[](Key) = Value;
        }
    }

    #pragma endregion
};

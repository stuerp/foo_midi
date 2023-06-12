
/** $VER: ConfigurationMap (2023.05.18) **/

#pragma once

#include <sdk/foobar2000-lite.h>
#include <sdk/cfg_var.h>

#include <map>

/// <summary>
/// Implements a configuration variable for maps.
/// </summary>
class cfg_map : public cfg_var, public std::map<uint32_t, std::vector<uint8_t>>
{
public:
    cfg_map(const GUID& guid) : cfg_var(guid), std::map<t_uint32, std::vector<uint8_t>>() { }
    cfg_map(const cfg_map&) = delete;
    cfg_map(const cfg_map&&) = delete;
    cfg_map& operator=(const cfg_map&) = delete;
    cfg_map& operator=(cfg_map&&) = delete;
    virtual ~cfg_map() { };

    #pragma region("cfg_var_writer")
    virtual void get_data_raw(stream_writer * streamWriter, abort_callback & handleAbort)
    {
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
        stream_reader_formatter<> in(*streamReader, handleAbort);

        clear();

        t_size count;

        in.read_int(count);

        for (t_size i = 0; i < count; ++i)
        {
            t_uint32 p_key;

            std::vector<uint8_t> p_value;

            in.read_int(p_key);

            {
                t_uint32 size;

                in >> size;

                p_value.resize(size);

                for (t_uint32 walk = 0; walk < size; ++walk)
                    in >> p_value[walk];
            }

            operator[](p_key) = p_value;
        }
    }
    #pragma endregion
};

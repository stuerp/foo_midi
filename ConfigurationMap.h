#pragma once

#include <foobar2000.h>
#include <map>

class cfg_map : public cfg_var, public std::map<uint32_t, std::vector<uint8_t>>
{
public:
    cfg_map(const GUID& guid) : cfg_var(guid), std::map<t_uint32, std::vector<uint8_t>>() { }
    cfg_map(const cfg_map&) = delete;
    cfg_map(const cfg_map&&) = delete;
    cfg_map& operator=(const cfg_map&) = delete;
    cfg_map& operator=(cfg_map&&) = delete;
    virtual ~cfg_map() { };

    void get_data_raw(stream_writer * streamWriter, abort_callback & handleAbort)
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

    void set_data_raw(stream_reader * streamReader, t_size, abort_callback & handleAbort)
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
};

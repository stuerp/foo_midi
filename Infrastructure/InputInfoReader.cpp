 
/** $VER: InputInfoReader.cpp (2025.07.15) **/

#include "pch.h"

#include "InputDecoder.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <libsf.h>

#include "KaraokeProcessor.h"
#include "Log.h"

/// <summary>
/// Retrieves information about the specified subsong.
/// </summary>
void InputDecoder::get_info(t_uint32 subSongIndex, file_info & fileInfo, abort_callback & abortHandler)
{
    if (_IsSysExFile)
        return;

    ConvertMetaDataToTags(subSongIndex, fileInfo, abortHandler);

    fileInfo.set_length((double) _Container.GetDuration(subSongIndex, true) * 0.001); // Without loops, fade-out or decay.
}

/// <summary>
/// Converts the MIDI metadata to tags.
/// </summary>
void InputDecoder::ConvertMetaDataToTags(size_t subSongIndex, file_info & fileInfo, abort_callback & abortHandler)
{
    _IsMT32 = false;
    _IsXG = false;

    KaraokeProcessor kp;

    {
        midi::metadata_table_t MetaData;

        _Container.GetMetaData(subSongIndex, MetaData);

        midi::metadata_item_t TrackItem;

        bool HasTitle = MetaData.GetItem("title", TrackItem);

        {
            for (const auto & Item : MetaData)
            {
                if (pfc::stricmp_ascii(Item.Name.c_str(), "type") == 0)
                {
                    fileInfo.info_set(TagMIDIType, Item.Value.c_str());

                    _IsMT32 = (Item.Value == "MT-32");
                    _IsXG = (Item.Value == "XG");
                }
                else
                if (pfc::stricmp_ascii(Item.Name.c_str(), "lyrics_type") == 0)
                {
                    fileInfo.info_set(TagMIDILyricsType, Item.Value.c_str());
                }
                else
                if (pfc::stricmp_ascii(Item.Name.c_str(), "lyrics") == 0)
                {
                    kp.AddUnsyncedLyrics(Item.Timestamp, Item.Value.c_str());
                }
                else
                if (pfc::stricmp_ascii(Item.Name.c_str(), "soft_karaoke_lyrics") == 0)
                {
                    kp.AddSyncedLyrics(Item.Timestamp, Item.Value.c_str());
                }
                else
                if (pfc::stricmp_ascii(Item.Name.c_str(), "soft_karaoke_text") == 0)
                {
                    kp.AddSyncedText(Item.Value.c_str());
                }
                else
                {
                    std::string Name = Item.Name;

                    if (!HasTitle && (pfc::stricmp_ascii(Name.c_str(), "display_name") == 0))
                        Name = "title";

                    AddTag(fileInfo, Name.c_str(), Item.Value.c_str(), 0);
                }
            }
        }
    }

    // Read a WRD file in the same path and convert it to lyrics.
    {
        std::filesystem::path FilePath(pfc::wideFromUTF8(_FilePath.c_str()).c_str());

        FilePath.replace_extension(L".wrd");

        std::ifstream Stream(FilePath);

        if (Stream.is_open())
        {
            std::string Line;

            while (std::getline(Stream, Line))
            {
                if (Line[0] != '@')
                    kp.AddUnsyncedLyrics(0, Line.c_str());
            }
        }
    }

    if (!kp.GetUnsyncedLyrics().empty())
    {
        const auto Lyrics = kp.GetUnsyncedLyrics();

        fileInfo.meta_set("lyrics", (const char *) Lyrics.c_str());
    }

    if (!kp.GetSyncedLyrics().empty())
    {
        const auto Lyrics = kp.GetSyncedLyrics();

        fileInfo.meta_set("syncedlyrics", (const char *) Lyrics.c_str());
    }

    // General info
    fileInfo.info_set_int   ("channels", 2);
    fileInfo.info_set       ("encoding", "Synthesized");

    // Specific info
    fileInfo.info_set_int   (TagMIDIFormat,       _Container.GetFormat());
    fileInfo.info_set_int   (TagMIDITrackCount,   _Container.GetFormat() == 2 ? 1 : _Container.GetTrackCount());
    fileInfo.info_set_int   (TagMIDIChannelCount, _Container.GetChannelCount(subSongIndex));
    fileInfo.info_set_int   (TagMIDITicks,        _Container.GetDuration(subSongIndex));

    {
        uint32_t LoopBegin     = _Container.GetLoopBeginTimestamp(subSongIndex);
        uint32_t LoopEnd       = _Container.GetLoopEndTimestamp  (subSongIndex);
        uint32_t LoopBeginInMS = _Container.GetLoopBeginTimestamp(subSongIndex, true);
        uint32_t LoopEndInMS   = _Container.GetLoopEndTimestamp  (subSongIndex, true);

        if (LoopBegin != ~0U)     fileInfo.info_set_int(TagMIDILoopStart, LoopBegin);
        if (LoopEnd != ~0U)       fileInfo.info_set_int(TagMIDILoopEnd, LoopEnd);
        if (LoopBeginInMS != ~0U) fileInfo.info_set_int(TagMIDILoopStartInMs, LoopBeginInMS);
        if (LoopEndInMS != ~0U)   fileInfo.info_set_int(TagMIDILoopEndInMs, LoopEndInMS);
    }

    // Add a tag that identifies the embedded soundfont, if present.
    try
    {
        const auto & Data = _Container.GetSoundfontData();

        if (Data.size() > 12)
        {
            std::string TagValue("DLS");

            if (::memcmp(Data.data() + 8, "DLS ", 4) != 0)
            {
                TagValue.clear();

                sf::bank_t sf;

                riff::memory_stream_t ms;

                if (ms.Open(Data.data(), Data.size()))
                {
                    sf::reader_t sr;

                    if (sr.Open(&ms, riff::reader_t::option_t::None))
                    {
                        sr.Process({ false }, sf); // Don't load the sample data.

                        TagValue = ::FormatText("SF %d.%d", sf.Major, sf.Minor);
                    }

                    ms.Close();
                }
            }

            fileInfo.info_set(TagMIDIEmbeddedSoundfont, TagValue.c_str());
        }
    }
    catch (std::exception e)
    {
        Log.AtWarn().Write(STR_COMPONENT_BASENAME " is unable to create tag \"%s\": %s", TagMIDIEmbeddedSoundfont, e.what());
    }

    {
        pfc::string FileHashString;

        for (size_t i = 0U; i < 16; ++i)
            FileHashString += pfc::format_uint((t_uint8)_FileHash.m_data[i], 2, 16);

        fileInfo.info_set(TagMIDIHash, FileHashString);
    }

    {
        service_ptr_t<metadb_index_client> IndexClient = new service_impl_t<FileHasher>;

        _Hash = IndexClient->transform(fileInfo, playable_location_impl(_FilePath, (t_uint32) subSongIndex));

        pfc::array_t<t_uint8> Tags;

        static_api_ptr_t<metadb_index_manager>()->get_user_data_t(GUIDTagMIDIHash, _Hash, Tags);

        t_size TagCount = Tags.get_count();

        if (TagCount > 0)
        {
            file::ptr File;

            filesystem::g_open_tempmem(File, abortHandler);

            File->write_object(Tags.get_ptr(), TagCount, abortHandler);

            fileInfo.meta_remove_all();

            tag_processor::read_trailing(File, fileInfo, abortHandler);

            fileInfo.info_set("tagtype", "apev2 db");
        }
    }
}

/// <summary>
/// Adds the specified tag.
/// </summary>
void InputDecoder::AddTag(file_info & fileInfo, const char * name, const char * value, t_size maxLength)
{
    if ((name == nullptr) || (value == nullptr))
        return;

    if (value[0] == '\0')
        return;

    pfc::string Value;

    if ((maxLength != 0) && value[maxLength - 1])
    {
        Value.set_string(value, maxLength);
        value = Value;
    }
    else
        maxLength = ::strlen(value);

    if (!pfc::is_lower_ascii(value) && !pfc::is_valid_utf8(value, maxLength))
    {
        if (IsShiftJIS(value, maxLength))
        {
            Value = pfc::stringcvt::string_utf8_from_codepage(932, value);
        }
        else
        if (IsEUCJP(value, maxLength))
        {
            Value = pfc::stringcvt::string_utf8_from_codepage(20932, value);

            // Try other code pages.
            if (Value.is_empty())
                Value = pfc::stringcvt::string_utf8_from_codepage(51932, value); // EUC Japanese
        }
        else
            Value = pfc::stringcvt::string_utf8_from_ansi(value);

        if (!Value.is_empty())
            value = Value;
    }

    fileInfo.meta_add(name, value);
}


/** $VER: AlbumArt.cpp (2025.06.09) **/

#include "pch.h"

#include "InputDecoder.h"

#include <sdk/foobar2000.h>

/// <summary>
/// Implements access to the album art stored in the MIDI file.
/// </summary>
class AlbumArtExtractorInstance : public album_art_extractor_instance
{
public:
    AlbumArtExtractorInstance(const std::vector<uint8_t> & artwork)
    {
        _Artwork = artwork;
    }

    AlbumArtExtractorInstance(const AlbumArtExtractorInstance &) = delete;
    AlbumArtExtractorInstance(const AlbumArtExtractorInstance &&) = delete;
    AlbumArtExtractorInstance & operator=(const AlbumArtExtractorInstance &) = delete;
    AlbumArtExtractorInstance & operator=(AlbumArtExtractorInstance &&) = delete;

    virtual ~AlbumArtExtractorInstance() { };

    virtual album_art_data_ptr query(const GUID & what, abort_callback & abortHandler)
    {
        if (what != album_art_ids::cover_front)
            throw exception_album_art_not_found();

        if (_Artwork.empty())
            throw exception_album_art_not_found();

        return album_art_data_impl::g_create(_Artwork.data(), _Artwork.size());
    }

private:
    std::vector<uint8_t> _Artwork;
};

/// <summary>
/// Implements the entry point for album art extraction.
/// </summary>
class AlbumArtExtractor : public album_art_extractor_v2
{
public:
    AlbumArtExtractor() { }

    AlbumArtExtractor(const AlbumArtExtractor &) = delete;
    AlbumArtExtractor(const AlbumArtExtractor &&) = delete;
    AlbumArtExtractor & operator=(const AlbumArtExtractor &) = delete;
    AlbumArtExtractor & operator=(AlbumArtExtractor &&) = delete;

    virtual ~AlbumArtExtractor() { };

    /// <summary>
    /// Returns the GUID of the corresponding input class. Null GUID if none.
    /// </summary>
    GUID get_guid() final
    {
        return InputDecoder::g_get_guid();
    }

    /// <summary>
    /// Instantiates album_art_extractor_instance providing access to the album art.
    /// </summary>
    album_art_extractor_instance_ptr open(file_ptr fileHint, const char * filePath, abort_callback & abortHandler) final
    {
        file_ptr File(fileHint);

        if (File.is_empty())
            filesystem::g_open_read(File, filePath, abortHandler);

        t_filestats _FileStats = File->get_stats(abortHandler);

        if ((_FileStats.m_size == 0) || (_FileStats.m_size > (t_filesize)(1 << 30)))
            throw exception_io_unsupported_format();

        std::vector<uint8_t> Object((size_t) _FileStats.m_size);

        File->read_object(Object.data(), (t_size) _FileStats.m_size, abortHandler);

        midi::container_t Container;

        try
        {
            midi::processor_options_t Options
            (
                (uint16_t) CfgLoopExpansion,
                CfgWriteBarMarkers,
                CfgWriteSysExNames,
                CfgExtendLoops,
                CfgWolfteamLoopMode,
                CfgKeepMutedChannels,
                CfgIncludeControlData,

                (uint16_t) CfgDefaultTempo,

                true, // End of Track is required
                CfgDetectExtraDrum
            );

            midi::processor_t::Process(Object, pfc::wideFromUTF8(filePath), Container, Options);
        }
        catch (std::exception & e)
        {
            pfc::string Message = "Failed to read MIDI file: ";

            throw exception_io_data(Message + e.what());
        }

        return new service_impl_t<AlbumArtExtractorInstance>(Container.GetArtwork());
    }

    /// <summary>
    /// Returns whether the specified file is one of the supported formats.
    /// </summary>
    bool is_our_path(const char * path, const char * extension) final
    {
        return InputDecoder::g_is_our_path(path, extension);
    }
};

FB2K_SERVICE_FACTORY(AlbumArtExtractor)

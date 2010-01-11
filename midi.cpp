#define MYVERSION "1.7"

/*
	change log

2006-12-24 05:15 UTC - kode54
- Fixed plug-in droplist change handler for VSTi plug-ins so the last item in the list won't enable
  the GS/XG to GM2 checkbox.

2006-08-21 09:22 UTC - kode54
- Added call to effMainsChanged before effStartProcess for some VST instruments which require
  it. (Steinberg Hypersonic 2 crashes without this.)
- Fixed a bug in VSTiPlayer Play(), where processReplace would be called even if it didn't exist.
- Added GM initialization preset for Steinberg Hypersonic 2, requires foo_unpack.
- Version is now 1.7

2005-11-14 00:00 UTC - kode54
- Fixed bug in VSTiPlayer seeking ( bleh, C-style cast flubbed changes to mem_block class )

2005-10-08 22:10 UTC - kode54
- Ported to 0.9 beta 7 + input API
- DXi support disabled because it is a bloated mess and should die
- Version is now 1.6

2005-06-06 08:02 UTC - kode54
- VSTi seeking now resets the synthesizer when seeking backwards, and drops complete note on/off pairs
- Version is now 1.55

2005-05-29 04:21 UTC - kode54
- Completed VSTi support (complete with default polyphony override for S-YXG50)
- Version is now 1.5

2005-03-12 11:02 UTC - kode54
- Added basic GS/XG to GM2 remapping for Hyper Canvas
- Version is now 1.2.3

2004-09-29 19:11 UTC - kode54
- Bug fix in first rmi_data parsing loop, forgot to enum next item uhuhuhu
- Version is now 1.2.2

2004-09-25 04:48 UTC - kode54
- Renamed internal synth to "Emu de MIDI" hopefully to prevent confusion with the EMIDI setting
- Implemented Data Increment and Decrement controller changes in CMIDIModule.cpp
- Changed default pitch bend range in CMIDIModule.cpp from 12 semitones to 2
- Version is now 1.2.1

2004-09-23 10:45 UTC - kode54
- Added internal Emu de MIDI synthesizer, for great chip style justice
- Commented out external input wrapper
- Version is now 1.2

2004-09-19 08:50 UTC - kode54
- Implemented external input wrapper with looping, for foo_emidi
- Version is now 1.1.1

2004-09-19 04:27 UTC - kode54
- Whoops, forgot that all RIFF chunks are word aligned, fixed LIST INFO parser
- Version is now 1.1

2004-09-19 03:08 UTC - kode54
- Changed grow_buf truncate from a #define to an inline function wrapper
- Implemented RIFF LIST INFO chunk parser
- Expanded metadata gathering

2004-03-31 17:53 UTC - kode54
- Fixed seeking past the end of the file
- Version is now 1.0.1

*/

#ifdef DXISUPPORT
#include "stdafx.h"
#endif

#include <foobar2000.h>
#include "../helpers/dropdown_helper.h"

#include <shlobj.h>
#include <shlwapi.h>

#include "VSTiPlayer.h"

//#define DXISUPPORT 1

#ifdef DXISUPPORT
#include "DXiPlayer.h"
#include "PlugInInventory.h"

#include "MfxSeq.h"
#include "MfxBufferFactory.h"

#pragma comment( lib, "strmiids.lib" )
#endif

#include "main.h"

#include "resource.h"

#include "CSMFPlay.hpp"

#define XMIDI_CONTROLLER_FOR_LOOP			0x74	// For Loop
#define XMIDI_CONTROLLER_NEXT_BREAK			0x75	// Next/Break

#define EMIDI_CONTROLLER_TRACK_DESIGNATION	110		// Track Designation
#define EMIDI_CONTROLLER_TRACK_EXCLUSION	111		// Track Exclusion
#define EMIDI_CONTROLLER_LOOP_BEGIN			XMIDI_CONTROLLER_FOR_LOOP
#define EMIDI_CONTROLLER_LOOP_END			XMIDI_CONTROLLER_NEXT_BREAK

// {49B4E0E6-E572-4d5d-88DB-78CEA20EADC1}
static const GUID guid_cfg_xmiloopz = 
{ 0x49b4e0e6, 0xe572, 0x4d5d, { 0x88, 0xdb, 0x78, 0xce, 0xa2, 0xe, 0xad, 0xc1 } };
// {D1E8D624-C2F8-4fe1-A13E-B1F19A6F2CB6}
static const GUID guid_cfg_ff7loopz = 
{ 0xd1e8d624, 0xc2f8, 0x4fe1, { 0xa1, 0x3e, 0xb1, 0xf1, 0x9a, 0x6f, 0x2c, 0xb6 } };
// {C090F9C7-47F9-4f6f-847A-27CD7596C9D4}
static const GUID guid_cfg_emidi_exclusion = 
{ 0xc090f9c7, 0x47f9, 0x4f6f, { 0x84, 0x7a, 0x27, 0xcd, 0x75, 0x96, 0xc9, 0xd4 } };
// {FE5B24D8-C8A5-4b49-A163-972649217185}
static const GUID guid_cfg_recover_tracks = 
{ 0xfe5b24d8, 0xc8a5, 0x4b49, { 0xa1, 0x63, 0x97, 0x26, 0x49, 0x21, 0x71, 0x85 } };
// {DA3A7D23-BCEB-40f9-B594-2A9428A1E533}
static const GUID guid_cfg_loop_type = 
{ 0xda3a7d23, 0xbceb, 0x40f9, { 0xb5, 0x94, 0x2a, 0x94, 0x28, 0xa1, 0xe5, 0x33 } };
// {AE5BA73B-B0D4-4261-BFF2-11A1C44E57EA}
static const GUID guid_cfg_srate = 
{ 0xae5ba73b, 0xb0d4, 0x4261, { 0xbf, 0xf2, 0x11, 0xa1, 0xc4, 0x4e, 0x57, 0xea } };
// {1253BAC2-9193-420c-A919-9A1CF8706E2C}
static const GUID guid_cfg_plugin = 
{ 0x1253bac2, 0x9193, 0x420c, { 0xa9, 0x19, 0x9a, 0x1c, 0xf8, 0x70, 0x6e, 0x2c } };
// {408AA155-4C42-42b5-8C3E-D10C35DD5EF1}
static const GUID guid_cfg_history_rate = 
{ 0x408aa155, 0x4c42, 0x42b5, { 0x8c, 0x3e, 0xd1, 0xc, 0x35, 0xdd, 0x5e, 0xf1 } };
// {F3EE2258-65D3-4219-B932-BF52119F2484}
static const GUID guid_cfg_gm2 = 
{ 0xf3ee2258, 0x65d3, 0x4219, { 0xb9, 0x32, 0xbf, 0x52, 0x11, 0x9f, 0x24, 0x84 } };
// {1A6EA7E5-718A-485a-B167-CFDF3B406145}
static const GUID guid_cfg_vst_path = 
{ 0x1a6ea7e5, 0x718a, 0x485a, { 0xb1, 0x67, 0xcf, 0xdf, 0x3b, 0x40, 0x61, 0x45 } };

cfg_int cfg_xmiloopz(guid_cfg_xmiloopz, 0), cfg_ff7loopz(guid_cfg_ff7loopz, 0),
		cfg_emidi_exclusion(guid_cfg_emidi_exclusion, 0), /*cfg_hack_xg_drums("yam", 0),*/
		cfg_recover_tracks(guid_cfg_recover_tracks, 0), cfg_loop_type(guid_cfg_loop_type, 0),
		/*cfg_nosysex("sux", 0),*/ cfg_gm2(guid_cfg_gm2, 0),
		cfg_srate(guid_cfg_srate, 44100), cfg_plugin(guid_cfg_plugin, 0);

cfg_string cfg_vst_path(guid_cfg_vst_path, "");

static const char * exts[]=
{
	"MID",
	"MIDI",
	"RMI",
	"MIDS",
	"CMF",
	"GMF",
	"HMI",
	"HMP",
	"MUS",
	"XMI",
};

#ifdef DXISUPPORT
HRESULT LoadMid( CMfxSeq& rSeq, CFile& rFile );
#endif

static critical_section sync;
static int g_running = 0;
static int g_srate;

class input_midi
{
#ifdef DXISUPPORT
	bool initialized;
	CDXiPlayer * thePlayer;
	CMfxSeq * theSequence;
#endif

	VSTiPlayer * vstPlayer;

	MIDI_file * mf;

	unsigned srate;
	unsigned plugin;

	bool b_xmiloopz;
	bool b_ff7loopz;
	bool b_emidi_ex;
	bool b_gm2;

	unsigned length_samples;
	unsigned length_ticks;
	unsigned samples_done;

	int loop_begin;
	int loop_end;

	bool eof;
	bool dont_loop;

	bool first_block;

	t_filestats m_stats;

#if audio_sample_size != 32
	pfc::array_t<float> sample_buffer;
#endif

	/* crap for external input */
	/*
	input * external_decoder;
	reader * mem_reader;

	unsigned sample_loop_start;
	unsigned sample_loop_end;
	*/
	dsa::CSMFPlay * smfplay;
	pfc::array_t<int> smfplay_buffer;

public:
	input_midi() : srate(cfg_srate), plugin(cfg_plugin), b_xmiloopz(!!cfg_xmiloopz),
		b_ff7loopz(!!cfg_ff7loopz), b_emidi_ex(!!cfg_emidi_exclusion), b_gm2(!!cfg_gm2)
	{
#ifdef DXISUPPORT
		initialized = false;
		thePlayer = NULL;
		theSequence = NULL;
#endif

		vstPlayer = NULL;

		mf = NULL;

		length_samples = 0;
		length_ticks = 0;

		/*
		external_decoder = 0;
		mem_reader = 0;
		*/
		smfplay = 0;
	}

	~input_midi()
	{
		/*if (external_decoder) external_decoder->service_release();
		if (mem_reader) mem_reader->reader_release();*/
		if (smfplay)
		{
			delete smfplay;
			insync(sync);
			g_running--;
		}
#ifdef DXISUPPORT
		if (thePlayer) delete thePlayer;
		if (theSequence) delete theSequence;
#endif
		if (vstPlayer) delete vstPlayer;
		if (mf) mf->Free();
#ifdef DXISUPPORT
		if (initialized) CoUninitialize();
#endif
	}

private:
	HRESULT load_file(const void * data, unsigned size
#ifdef DXISUPPORT
		, CMfxSeq * out
#endif
		)
	{
		mf = MIDI_file::Create(data, size);
		if (!mf) return E_FAIL;
		
		loop_begin = -1;
		loop_end = -1;

#ifdef DXISUPPORT
		HRESULT rval;
		TRY
		rval = LoadMid(*out, CMemFile((BYTE*)mf->data, mf->size));
		CATCH_ALL(e)
			TCHAR foo[256];
			e->GetErrorMessage(foo, sizeof(foo) / sizeof(*foo));
			if (*foo) console::info(pfc::stringcvt::string_utf8_from_os(foo));
			return -1;
		END_CATCH_ALL
		if (FAILED(rval)) return rval;

		int i, j;

		if (b_emidi_ex || b_xmiloopz || b_gm2)
		{
			BYTE part_to_ch[16] = {9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13, 14, 15};

			map<int,CMfxTrack>::iterator it;
			for (it = theSequence->GetBeginTrack(); it != theSequence->GetEndTrack(); it++)
			{
				int index = it->first;
				CMfxTrack & trk = it->second;

				if (b_gm2)
				{
					if (trk.GetBank() == 16256) // XG
						trk.SetBank(15360); // remap to GM2
				}

				for (i = 0, j = trk.size(); i < j; i++)
				{
					switch (trk[i].GetType())
					{
					case MfxEvent::Patch:
						if (b_gm2)
						{
							if (trk[i].GetBank() == 16256) // XG
								trk[i].SetBank(15360); // GM2
						}
						break;
					case MfxEvent::Control:
						switch (trk[i].GetCtrlNum())
						{
						case 0:
							if (b_gm2)
							{
								if (trk[i].GetCtrlVal() == 127) // XG drum, hackery
									trk[i].SetCtrlVal(120); // GM2 drum, hackery
							}
							break;
						case EMIDI_CONTROLLER_TRACK_DESIGNATION:
							if (b_emidi_ex)
							{
								int dev = trk[i].GetCtrlVal();
								if (dev != 0 && dev != 127)
								{
									trk.clear();
									i = j;
								}
							}
							break;
						case EMIDI_CONTROLLER_TRACK_EXCLUSION:
							if (b_emidi_ex)
							{
								if (trk[i].GetCtrlNum() == 0)
								{
									trk.clear();
									i = j;
								}
							}
							break;
						case EMIDI_CONTROLLER_LOOP_BEGIN:
							if (b_xmiloopz)
							{
								if (loop_begin == -1)
								{
									loop_begin = trk[i].GetTime();
								}
							}
							break;
						case EMIDI_CONTROLLER_LOOP_END:
							if (b_xmiloopz)
							{
								if (loop_end == -1)
								{
									loop_end = trk[i].GetTime();
								}
							}
							break;
						}
						break;
					case MfxEvent::Sysx:
						if (b_gm2)
						{
							BYTE * pData;
							DWORD dwLength;
							if (theBufferFactory.GetPointer(trk[i].m_hBuffer, (void**) &pData, &dwLength) == S_OK &&
								dwLength == 11 &&
								pData[0] == 0xF0 && pData[1] == 0x41 && pData[3] == 0x42 && pData[4] == 0x12 && pData[5] == 0x40 && (pData[6] & 0xF0) == 0x10 && pData[10] == 0xF7)
							{
								if (pData[7] == 2)
								{
									// GS MIDI channel to part assign
									part_to_ch[pData[6] & 15] = pData[8];
								}
								else if (pData[7] == 0x15)
								{
									// GS part to rhythm allocation
									if (part_to_ch[pData[6] & 15] < 16)
									{
										CMfxEvent event( MfxEvent::Patch );
										event.SetTime( trk[i].GetTime() );
										event.SetChannel( part_to_ch[ pData[6] & 15 ] );
										event.SetPatch( 0 );
										event.SetBank( pData[8] ? 15360 : 15488 );
										trk[i] = event;
									}
								}
							}
						}
						break;
					}
				}
			}
		}

		if (b_ff7loopz)
		{
			for (i = 0, j = theSequence->m_markers.size(); i < j; i++)
			{
				const CMarker & marker = theSequence->m_markers[i];
				if (loop_begin == -1 && !strcmp(marker.GetName(), "loopStart"))
				{
					loop_begin = marker.GetTime();
				}
				else if (loop_end == -1 && !strcmp(marker.GetName(), "loopEnd"))
				{
					loop_end = marker.GetTime();
				}
			}
		}
#endif

		return S_OK;
	}

	double get_length()
	{
		int len = mf->GetLength();
		double length = len * .001 + 1.;
		length_ticks = mf->tix; //theSequence->m_tempoMap.Sample2Tick(len, 1000);
		length_samples = (unsigned)(((__int64)len * (__int64)srate) / 1000) + srate;
		return length;
	}

	void set_loop()
	{
#ifdef DXISUPPORT
		if (plugin>1 && thePlayer)
		{
			thePlayer->SetLoopStart(loop_begin != -1 ? loop_begin : 0);
			thePlayer->SetLoopEnd(loop_end != -1 ? loop_end : length_ticks);
			thePlayer->SetLooping(TRUE);
		}
		/*else
		{
			sample_loop_start = theSequence->m_tempoMap.Tick2Sample(loop_begin != -1 ? loop_begin : 0, srate);
			sample_loop_end = theSequence->m_tempoMap.Tick2Sample((loop_end != -1 ? loop_end : length_ticks) + 1, srate);
		}*/
		else
#endif
		if (smfplay)
		{
			smfplay->SetEndPoint(loop_end != -1 ? loop_end : length_ticks);
		}
		dont_loop = false;
	}

	/*
	input * get_external_decoder()
	{
		service_enum_t<input> e;
		input * i;
		for (i = e.first(); i; i = e.next())
		{
			if (i->needs_reader() && i->test_filename("", "mid"))
			{
				input_midi * im = service_query_t(input_midi,i);
				if (!im) break;
				im->service_release();
			}
			i->service_release();
		}
		return i;
	}
	*/

public:
	void open( service_ptr_t<file> p_file,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_unsupported_format();

		if ( p_file.is_empty() )
		{
			filesystem::g_open( p_file, p_path, filesystem::open_mode_read, p_abort );
		}

		m_stats = p_file->get_stats( p_abort );
		if ( ! m_stats.m_size || m_stats.m_size > ( 1 << 30 ) ) throw exception_io_data();

		unsigned sz;
		pfc::array_t< t_uint8 > buffer;

		sz = (unsigned) m_stats.m_size;
		buffer.set_size( sz );
		p_file->read_object( buffer.get_ptr(), sz, p_abort );

#ifdef DXISUPPORT
		theSequence = new CMfxSeq;
		if ( FAILED( load_file( buffer, sz, theSequence ) ) ) throw exception_io_data();
#else
		if (FAILED(load_file(buffer.get_ptr(), sz))) throw exception_io_data();
#endif
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		bool title = false;
		if (mf->title.length())
		{
			if (!mf->rmi_data.is_empty())
			{
				const void * entry = mf->rmi_data.enum_entry(0);
				while (entry)
				{
					if (!strcmp(meta_table::enum_name(entry), "title"))
					{
						title = true;
						break;
					}
					entry = meta_table::enum_next_entry(entry);
				}
			}
			p_info.meta_add(title ? "display_name" : "title", pfc::stringcvt::string_utf8_from_ansi(mf->title));
			title = true;
		}
		if (mf->info.copyright.length()) p_info.meta_add("copyright", pfc::stringcvt::string_utf8_from_ansi(mf->info.copyright));
		if (mf->info.markers.length()) p_info.meta_add("track_markers", pfc::stringcvt::string_utf8_from_ansi(mf->info.markers));

		if (!mf->rmi_data.is_empty())
		{
			const void * entry = mf->rmi_data.enum_entry(0);
			while (entry)
			{
				p_info.meta_add(meta_table::enum_name(entry), pfc::stringcvt::string_utf8_from_ansi(meta_table::enum_data(entry)));
				entry = meta_table::enum_next_entry(entry);
			}
		}

		if (mf->info.traxnames && mf->info.traxtext)
		{
			pfc::string8_fastalloc name;
			for (unsigned i = 0, j = mf->info.ntrax; i < j; i++)
			{
				if (mf->info.traxnames[i] && mf->info.traxnames[i].length())
				{
					if (!i && mf->info.fmt && !title) name = "title";
					else
					{
						name = "track";
						if (i < 10) name.add_byte('0');
						name << i;
					}
					p_info.meta_add(name, pfc::stringcvt::string_utf8_from_ansi(mf->info.traxnames[i].get_ptr()));
				}
				if (mf->info.traxtext[i] && mf->info.traxtext[i].length())
				{
					name = "track";
					if (i < 10) name.add_byte('0');
					name << i;
					p_info.meta_add(name, pfc::stringcvt::string_utf8_from_ansi(mf->info.traxtext[i].get_ptr()));
				}
			}
		}

		p_info.info_set_int("midi_format", mf->info.fmt);
		p_info.info_set_int("midi_tracks", mf->info.ntrax);
		p_info.info_set_int("midi_channels", mf->info.channels);
		p_info.info_set_int("midi_ticks", mf->info.tix);
		if (mf->info.e_type) p_info.info_set("midi_type", mf->info.e_type);

		if (loop_begin != -1) p_info.info_set_int("midi_loop_start", loop_begin);
		if (loop_end != -1) p_info.info_set_int("midi_loop_end", loop_end);
		//p_info.info_set_int("samplerate", srate);
		p_info.info_set_int("channels", 2);
		p_info.set_length( get_length() );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		first_block = true;

		get_length();

#ifdef DXISUPPORT
		if (plugin>1)
		{
			thePlayer = new CDXiPlayer;
			CoInitialize(NULL);
			initialized = true;
			if (SUCCEEDED(thePlayer->Initialize()))
			{
				thePlayer->SetSampleRate(srate);
				if (SUCCEEDED(thePlayer->SetSeq(theSequence)))
				{
					CPlugInInventory theInventory;
					if (SUCCEEDED(theInventory.EnumPlugIns()))
					{
						IBaseFilter* pFilter = NULL;
						if (SUCCEEDED(theInventory.CreatePlugIn(plugin-2, &pFilter)))
						{
							thePlayer->SetFilter(pFilter);
							pFilter->Release();
							thePlayer->Stop();
							thePlayer->Play(TRUE);

							eof = false;
							dont_loop = true;

							if ( ! ( p_flags & input_flag_no_looping ) && cfg_loop_type)
							{
								if (cfg_loop_type == 1)
								{
									if (loop_begin != -1 || loop_end != -1)
									{
										set_loop();
									}
								}
								else
								{
									set_loop();
								}
							}

							samples_done = 0;
							return io_result_success;
						}
					}
				}
			}
		}
		else
#endif
			if (plugin)
			{
				delete vstPlayer;
				vstPlayer = new VSTiPlayer;
				if (vstPlayer->LoadVST(cfg_vst_path))
				{
					{
						pfc::array_t< t_uint8 > buffer;
						vstPlayer->getChunk(buffer);
						if (buffer.get_size())
						{
							static const unsigned char def_syxg50[] = { 0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74,
								0x20, 0x53, 0x65, 0x74, 0x75, 0x70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
								0x20, 1, 1, 1, 0x65 };

							static const unsigned char def_hypersonic2_id[] = { 0x52, 0xB8, 0x9E, 0x3E };

							if (vstPlayer->getUniqueID() == 'd2w1' &&
								buffer.get_size() == sizeof(def_syxg50) &&
								!memcmp(buffer.get_ptr(), &def_syxg50, sizeof(def_syxg50)))
							{
								unsigned char * ptr = (unsigned char *) buffer.get_ptr();
								ptr[0x1C] = 128;
								vstPlayer->setChunk(ptr, sizeof(def_syxg50));
							}
							else if ( vstPlayer->getUniqueID() == 'StSS' &&
								buffer.get_size() == 35559 && 
								!memcmp((char*)buffer.get_ptr() + 0x88BB, &def_hypersonic2_id, 4))
							{
								try
								{
#include "hypersonic2.h"
									abort_callback_impl m_abort;
									service_ptr_t<file> in, temp;
									//filesystem::g_open(in, "file://c:\\new_chunk.gz", filesystem::open_mode_read, m_abort);
									filesystem::g_open_tempmem(in, m_abort);
									in->write(hypersonic2_gm, sizeof(hypersonic2_gm), m_abort);
									in->reopen(m_abort);
									unpacker::g_open(temp, in, m_abort);
									buffer.set_size(temp->get_size_ex(m_abort));
									temp->read(buffer.get_ptr(), buffer.get_size(), m_abort);
									vstPlayer->setChunk(buffer.get_ptr(), buffer.get_size());
								}
								catch(...)
								{
								}
							}
						}
					}
					vstPlayer->setSampleRate(srate);

					unsigned loop_mode = 0;

					if ( ! ( p_flags & input_flag_no_looping ) && cfg_loop_type )
					{
						loop_mode = VSTiPlayer::loop_mode_enable;
						if ( cfg_loop_type > 1 ) loop_mode |= VSTiPlayer::loop_mode_force;
						if ( b_xmiloopz ) loop_mode |= VSTiPlayer::loop_mode_xmi;
						if ( b_ff7loopz ) loop_mode |= VSTiPlayer::loop_mode_marker;
					}

					if ( vstPlayer->Load( mf, loop_mode, b_emidi_ex ? CLEAN_EMIDI : 0 ) )
					{
						eof = false;
						dont_loop = true;

						return;
					}
				}
			}
			else
			{
				/* oh boy, yank in an external service! */
				/*
				input * i = get_external_decoder();
				if (i)
				{
				external_decoder = i;

				mem_reader = new reader_mem_temp((void*)mf->data, mf->size);

				file_info_i info;

				if (i->open(mem_reader, &info, flags))
				{
				eof = false;
				dont_loop = true;

				if (!(flags & OPEN_FLAG_NO_LOOPING) && cfg_loop_type)
				{
				if (cfg_loop_type == 1)
				{
				if (loop_begin != -1 || loop_end != -1)
				{
				set_loop();
				}
				}
				else
				{
				set_loop();
				}
				}

				samples_done = 0;

				return 1;
				}
				}
				*/
				delete smfplay;
				smfplay = new dsa::CSMFPlay(srate, 8);
				if (smfplay->Load(mf->data, mf->size))
				{
					{
						insync(sync);
						if (++g_running == 1) g_srate = srate;
						else if (srate != g_srate)
						{
							srate = g_srate;
							get_length();
							delete smfplay;
							smfplay = new dsa::CSMFPlay(srate, 8);
							if (!smfplay->Load(mf->data, mf->size)) throw exception_io_data();
						}
					}

					eof = false;
					dont_loop = true;

					smfplay->Start();

					if ( ! ( p_flags & input_flag_no_looping ) && cfg_loop_type )
					{
						if ( cfg_loop_type == 1 )
						{
							if ( loop_begin != -1 || loop_end != -1 )
							{
								set_loop();
							}
						}
						else
						{
							set_loop();
						}
					}

					samples_done = 0;

					return;
				}
			}

		throw std::bad_alloc();
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if (eof) return false;

#ifdef DXISUPPORT
		if (plugin>1)
		{
			unsigned todo = 1024;

			if (dont_loop)
			{
				if (length_samples && samples_done + todo > length_samples)
				{
					todo = length_samples - samples_done;
					if (!todo) return io_result_eof;
				}
			}

#if audio_sample_size != 32
			if ( ! sample_buffer.check_size( todo * 2 ) )
				return io_result_error_out_of_memory;

			float * ptr = sample_buffer.get_ptr();

			thePlayer->FillBuffer(ptr, todo);

			if ( ! p_chunk.set_data_32( ptr, todo, 2, srate ) )
				return io_result_error_out_of_memory;
#else
			if ( ! p_chunk.check_data_size( todo * 2 ) )
				return io_result_error_out_of_memory;
			float * ptr = p_chunk.get_data();
			thePlayer->FillBuffer( ptr, todo );
			p_chunk.set_srate( srate );
			p_chunk.set_channels( 2 );
			p_chunk.set_sample_count( todo );
#endif

			samples_done += todo;

			return io_result_success;
		}
		else
#endif
		if (plugin)
		{
			unsigned todo = 1024;
			unsigned nch = vstPlayer->getChannelCount();

			p_chunk.set_data_size( todo * nch );

			audio_sample * out = p_chunk.get_data();

			unsigned done = vstPlayer->Play( out, todo );

			if ( ! done ) return false;

			p_chunk.set_srate( srate );
			p_chunk.set_channels( nch );
			p_chunk.set_sample_count( done );

			if ( done < todo ) eof = true;

			return true;
		}
		else
		{
			/*
			int rval = external_decoder->run(chunk);
			if (rval >= 0)
			{
				if (!dont_loop)
				{
					if (rval)
					{
						unsigned new_rate = chunk->get_srate();
						if (srate != new_rate)
						{
							ULONGLONG meh = UInt32x32To64(samples_done, new_rate);
							samples_done = (unsigned)(meh / (ULONGLONG)srate);
							srate = new_rate;
							set_loop();
							get_length();
						}
					}

					unsigned done = chunk->get_sample_count();
					if (!rval || (samples_done + done >= sample_loop_end))
					{
						if (!external_decoder->seek((double)sample_loop_start / (double)srate))
						{
							goto fagotry;
						}

						if (rval)
						{
							done = sample_loop_end - samples_done;
							if (done)
							{
								chunk->set_sample_count(done);
								samples_done = sample_loop_start;
								return 1;
							}
						}

						rval = external_decoder->run(chunk);

						if (!rval)
						{
							/* gee, looks like input needs a bit more coaxing to reset, damnit
fagotry:
							external_decoder->service_release();
							external_decoder = get_external_decoder();
							if (!external_decoder) return 0;

							file_info_i info;
							mem_reader->seek(0);
							if (!external_decoder->open(mem_reader, &info, OPEN_FLAG_DECODE)) return 0;
							if (!external_decoder->seek((double)sample_loop_start / (double)srate)) return 0;

							rval = external_decoder->run(chunk);
						}

						samples_done = sample_loop_start;
						done = chunk->get_sample_count();
					}
					samples_done += done;
				}
			}
			return rval;
			*/
			unsigned todo = 1024;

			if (dont_loop)
			{
				if (length_samples && samples_done + todo > length_samples)
				{
					todo = length_samples - samples_done;
					if (!todo) return false;
				}
			}

			smfplay_buffer.grow_size( todo * 2 );

			int * ptr = smfplay_buffer.get_ptr();

			unsigned done;
			
			try
			{
				done = smfplay->Render(ptr, todo);
			}
			catch ( const dsa::RuntimeException & e )
			{
				console::formatter() << e.m_message << " (" << e.m_file << ":" << e.m_lineno << ")";
				throw exception_io_data();
			}

			if (!dont_loop && done < todo)
			{
				smfplay->Start(loop_begin <= 0);
				set_loop();
				try
				{
					if (loop_begin > 0) smfplay->Render(NULL, loop_begin);
					if (!done) done = smfplay->Render(ptr, todo);
				}
				catch ( const dsa::RuntimeException & e )
				{
					throw std::exception( pfc::string8() << e.m_message << " (" << e.m_file << ":" << e.m_lineno << ")" );
				}
			}

			if (done)
			{
				p_chunk.set_data_size( done * 2 );

				audio_sample * out = p_chunk.get_data();

				audio_math::convert_from_int32( ptr, done * 2, out, 1 << 16 );

				samples_done += done;

				p_chunk.set_srate( srate );
				p_chunk.set_channels( 2 );
				p_chunk.set_sample_count( done );
				//chunk->set_data_fixedpoint(ptr, done * 4, srate, 2, 16);
			}

			return !! done;
		}
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		unsigned seek_msec = unsigned( audio_math::time_to_samples( p_seconds, 1000 ) );

		first_block = true;
		eof = false;

		unsigned done = unsigned( ( t_int64( seek_msec ) * t_int64( srate) ) / 1000 );
		if ( length_samples && done >= ( length_samples - srate ) )
		{
			eof = true;
			return;
		}

#ifdef DXISUPPORT
		if ( plugin > 1 )
		{
			unsigned tick = theSequence->m_tempoMap.Sample2Tick( seek_msec, 1000 );

			thePlayer->Stop();
			thePlayer->SetPosition( tick );
			thePlayer->Play( TRUE );

			samples_done = done;

			return io_result_success;
		}
		else
#endif
		if ( plugin )
		{
			vstPlayer->Seek( done );
			return;
		}
		else
		{
			unsigned samples_wanted = unsigned( p_seconds * double( srate ) );
			if ( samples_wanted < samples_done )
			{
				smfplay->Start();
				if ( ! dont_loop ) set_loop();
				samples_done = 0;
			}
			try
			{
				while ( samples_done < samples_wanted )
				{
					p_abort.check();

					unsigned todo = samples_wanted - samples_done;
					if ( todo > 1024 ) todo = 1024;
					smfplay_buffer.grow_size( todo * 2 );
					int * ptr = smfplay_buffer.get_ptr();
					if ( smfplay->Render( ptr, todo ) < todo )
					{
						eof = true;
						break;
					}
					samples_done += todo;
				}
			}
			catch ( const dsa::RuntimeException & e )
			{
				throw std::exception( pfc::string8() << e.m_message << " (" << e.m_file << ":" << e.m_lineno << ")" );
			}
		}
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
	{
		if ( first_block )
		{
			p_out.info_set_int( "samplerate", srate );
			p_timestamp_delta = 0.;
			first_block = false;
			return true;
		}

		return false;
	}

	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
	}

	void retag( const file_info & p_info, abort_callback & p_abort )
	{
		throw exception_io_unsupported_format();
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return ! strcmp( p_content_type, "audio/midi" );
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		for( unsigned n=0; n< tabsize( exts ); ++n )
		{
			if ( ! stricmp( p_extension, exts[ n ] ) ) return true;
		}
		return false;
	}
};

static const char * loop_txt[] = {"Never", "When loop info detected", "Always"};

static cfg_dropdown_history cfg_history_rate(guid_cfg_history_rate,16);

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000};

struct vsti_info
{
	pfc::string8 path, display_name;
};

static pfc::array_t< vsti_info > vsti_plugins;

class preferences_page_midi : public preferences_page
{
	static void enum_vsti_plugins( const char * _path = 0, puFindFile _find = 0 )
	{
		pfc::string8 ppath;
		if ( ! _find )
		{
			vsti_plugins.set_size( 0 );
			TCHAR path[ MAX_PATH + 1 ];
			if ( SUCCEEDED( SHGetFolderPath( 0, CSIDL_PROGRAM_FILES, 0, SHGFP_TYPE_CURRENT, path ) ) )
			{
				ppath = pfc::stringcvt::string_utf8_from_os( path );
				ppath += "\\Steinberg\\VstPlugins";
				ppath.add_byte( '\\' );
				ppath += "*.*";
				_path = ppath;
				_find = uFindFirstFile( ppath );
			}
		}
		if ( _find )
		{
			do
			{
				if ( _find->IsDirectory() && strcmp( _find->GetFileName(), "." ) && strcmp( _find->GetFileName(), ".." ) )
				{
					pfc::string8 npath( _path );
					npath.truncate( npath.length() - 3 );
					npath += _find->GetFileName();
					npath.add_byte( '\\' );
					npath += "*.*";
					puFindFile pfind = uFindFirstFile( npath );
					if ( pfind ) enum_vsti_plugins( npath, pfind );
				}
				else if ( _find->GetFileSize() )
				{
					pfc::string8 npath( _path );
					npath.truncate( npath.length() - 3 );
					npath += _find->GetFileName();
					if ( !stricmp_utf8( npath.get_ptr() + npath.length() - 4, ".dll" ) )
					{
						VSTiPlayer vstPlayer;
						if ( vstPlayer.LoadVST( npath ) )
						{
							vsti_info info;
							info.path = npath;

							pfc::string8 vendor, product;
							vstPlayer.getVendorString(vendor);
							vstPlayer.getProductString(product);

							if (product.length() || vendor.length())
							{
								if (!vendor.length() ||
									(product.length() >= vendor.length() &&
									!strncmp(vendor, product, vendor.length())))
								{
									info.display_name = product;
								}
								else
								{
									info.display_name = vendor;
									if (product.length())
									{
										info.display_name.add_byte(' ');
										info.display_name += product;
									}
								}
							}
							else info.display_name = _find->GetFileName();

							vsti_plugins.append_single( info );
						}
					}
				}
			} while ( _find->FindNext() );
			delete _find;
		}
	}

	/*
	static bool set_vsti(HWND wnd, bool choose)
	{
		pfc::string8 path;
		if (choose)
		{
			if (!uGetOpenFileName(wnd, "Dll Files|*.dll", 0, "dll", "Choose a VST instrument...", 0, path, FALSE))
			{
				return false;
			}
		}
		else path = cfg_vst_path;

		bool rval = false;
		pfc::string8_fastalloc display;
		display = "VST instrument";

		if (path.length())
		{
			VSTiPlayer vstPlayer;
			if (vstPlayer.LoadVST(path))
			{
				pfc::string8 vendor, product;
				vstPlayer.getVendorString(vendor);
				vstPlayer.getProductString(product);

				if (product.length() || vendor.length())
				{
					if (!vendor.length() ||
						(product.length() >= vendor.length() &&
						!strncmp(vendor, product, vendor.length())))
					{
						display = product;
					}
					else
					{
						display = vendor;
						if (product.length())
						{
							display.add_byte(' ');
							display += product;
						}
					}
				}
				else display = path.get_ptr() + path.scan_filename();

				rval = true;
				cfg_vst_path = path;
				cfg_plugin = 1;
			}
			else
			{
				if (choose) uMessageBox(wnd, pfc::string8() << path.get_ptr() + path.scan_filename() << " is not a working VST instrument.", "Error", MB_ICONEXCLAMATION );
				else
				{
					cfg_vst_path = "";
					cfg_plugin = 0;
				}
			}
		}

		HWND w = GetDlgItem(wnd, IDC_PLUGIN);

		if (choose)
		{
			uSendMessage(w, CB_DELETESTRING, 1, 0);
			uSendMessageText(w, CB_INSERTSTRING, 1, display);
			uSendMessage(w, CB_SETCURSEL, cfg_plugin, 0);
		}
		else
		{
			uSendMessageText(w, CB_ADDSTRING, 0, display);
		}

		return rval;
	}
	*/

	static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			{
				HWND w;
				int plugin = cfg_plugin;

				w = GetDlgItem(wnd, IDC_PLUGIN);
				uSendMessageText(w, CB_ADDSTRING, 0, "Emu de MIDI");

				/*
				if (plugin == 1)
				{
					if (!set_vsti(wnd, false)) plugin = 0;
				}
				else uSendMessageText(w, CB_ADDSTRING, 0, "VST instrument");
				*/

				enum_vsti_plugins();

				unsigned vsti_count = vsti_plugins.get_size(), vsti_selected = ~0;

				for ( unsigned i = 0; i < vsti_count; ++i )
				{
					uSendMessageText( w, CB_ADDSTRING, 0, vsti_plugins[ i ].display_name );
					if ( plugin == 1 && ! stricmp_utf8( vsti_plugins[ i ].path, cfg_vst_path ) )
						vsti_selected = i;
				}

#ifdef DXISUPPORT
				CoInitialize(NULL);
				{
					CPlugInInventory theInventory;
					if (SUCCEEDED(theInventory.EnumPlugIns()))
					{
						unsigned count = theInventory.GetCount();
						pfc::string8_fastalloc name;
						CLSID foo;
						for (unsigned i = 0; i < count; i++)
						{
							theInventory.GetInfo(i, &foo, name);
							uSendMessageText(w, CB_ADDSTRING, 0, name);
						}
					}
				}
				CoUninitialize();
#endif
				if ( plugin == 1 ) plugin += vsti_selected;
#ifdef DXISUPPORT
				else if ( plugin ) plugin += vsti_count;
#endif
				uSendMessage(w, CB_SETCURSEL, plugin, 0);

				{
					char temp[16];
					int n;
					for(n=tabsize(srate_tab);n--;)
					{
						if (srate_tab[n] != cfg_srate)
						{
							itoa(srate_tab[n], temp, 10);
							cfg_history_rate.add_item(temp);
						}
					}
					itoa(cfg_srate, temp, 10);
					cfg_history_rate.add_item(temp);
					cfg_history_rate.setup_dropdown(w = GetDlgItem(wnd,IDC_SAMPLERATE));
					uSendMessage(w, CB_SETCURSEL, 0, 0);
				}

				if (!plugin)
				{
					if (g_running) EnableWindow(GetDlgItem(wnd,IDC_SAMPLERATE), FALSE);
					EnableWindow(GetDlgItem(wnd,IDC_EMIDI_EX), FALSE);
				}

				if ( plugin <= vsti_count ) EnableWindow(GetDlgItem(wnd,IDC_GM2), FALSE);

				w = GetDlgItem(wnd, IDC_LOOP);
				for (unsigned i = 0; i < tabsize(loop_txt); i++)
				{
					uSendMessageText(w, CB_ADDSTRING, 0, loop_txt[i]);
				}
				uSendMessage(w, CB_SETCURSEL, cfg_loop_type, 0);

				uSendDlgItemMessage(wnd, IDC_XMILOOPZ, BM_SETCHECK, cfg_xmiloopz, 0);
				uSendDlgItemMessage(wnd, IDC_FF7LOOPZ, BM_SETCHECK, cfg_ff7loopz, 0);

				uSendDlgItemMessage(wnd, IDC_EMIDI_EX, BM_SETCHECK, cfg_emidi_exclusion, 0);
				uSendDlgItemMessage(wnd, IDC_GM2, BM_SETCHECK, cfg_gm2, 0);
				uSendDlgItemMessage(wnd, IDC_RECOVER, BM_SETCHECK, cfg_recover_tracks, 0);
				//uSendDlgItemMessage(wnd, IDC_NOSYSEX, BM_SETCHECK, cfg_nosysex, 0);
				//uSendDlgItemMessage(wnd, IDC_HACK_XG_DRUMS, BM_SETCHECK, cfg_hack_xg_drums, 0);
			}
			break;
		case WM_COMMAND:
			switch (wp)
			{
			case (CBN_SELCHANGE << 16) | IDC_PLUGIN:
				{
					t_size vsti_count = vsti_plugins.get_size();
					int plugin = uSendMessage((HWND)lp,CB_GETCURSEL,0,0);

					if ( ! plugin )
					{
						cfg_plugin = 0;
					}
					else if ( plugin <= vsti_count )
					{
						cfg_plugin = 1;
						cfg_vst_path = vsti_plugins[ plugin - 1 ].path;
					}
					else
					{
						cfg_plugin = plugin - vsti_count + 1;
					}
					cfg_plugin = plugin;
					EnableWindow( GetDlgItem( wnd, IDC_SAMPLERATE ), plugin || !g_running );
					EnableWindow( GetDlgItem( wnd, IDC_EMIDI_EX ), !! plugin );
					EnableWindow( GetDlgItem( wnd, IDC_GM2 ), plugin > vsti_count );
				}
				break;
			case (CBN_KILLFOCUS<<16)|IDC_SAMPLERATE:
				{
					int t = GetDlgItemInt(wnd,IDC_SAMPLERATE,0,0);
					if (t<6000) t=6000;
					else if (t>192000) t=192000;
					cfg_srate = t;
				}
				break;
			case (CBN_SELCHANGE << 16) | IDC_LOOP:
				cfg_loop_type = uSendMessage((HWND)lp,CB_GETCURSEL,0,0);
				break;
			case IDC_XMILOOPZ:
				cfg_xmiloopz = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_FF7LOOPZ:
				cfg_ff7loopz = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_EMIDI_EX:
				cfg_emidi_exclusion = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_GM2:
				cfg_gm2 = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_RECOVER:
				cfg_recover_tracks = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
				/*case IDC_NOSYSEX:
				cfg_nosysex = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;*/
				/*case IDC_HACK_XG_DRUMS:
				cfg_hack_xg_drums = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;*/
			}
			break;
		case WM_DESTROY:
			{
				vsti_plugins.set_size( 0 );
			}
			break;
		}
		return 0;
	}

public:
	HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG, parent, ConfigProc);
	}
	GUID get_guid()
	{
		// {1623AA03-BADC-4bab-8A17-C737CF782661}
		static const GUID guid = 
		{ 0x1623aa03, 0xbadc, 0x4bab, { 0x8a, 0x17, 0xc7, 0x37, 0xcf, 0x78, 0x26, 0x61 } };
		return guid;
	}
	const char * get_name() { return "MIDI synthesizer host"; }
	GUID get_parent_guid() {return guid_input;}

	bool reset_query() {return true;}
	void reset()
	{
		cfg_xmiloopz = 0;
		cfg_ff7loopz = 0;
		cfg_emidi_exclusion = 0;
		/* cfg_hack_xg_drums = 0; */
		cfg_recover_tracks = 0;
		cfg_loop_type = 0;
		/* cfg_nosysex = 0; */
		cfg_gm2 = 0;
		cfg_srate = 44100;
		cfg_plugin = 0;
		cfg_vst_path = "";
	}
};

class midi_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 1;
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		if (idx > 0) return false;
		out = "MIDI files";
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		if (idx > 0) return false;
		out.reset();
		for (int n = 0; n < tabsize(exts); n++)
		{
			if (n) out.add_byte(';');
			out << "*." << exts[n];
		}
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

static input_singletrack_factory_t<input_midi>                      g_input_midi_factory;
static preferences_page_factory_t <preferences_page_midi>           g_config_midi_factory;
static service_factory_single_t   <midi_file_types> g_input_file_type_midi_factory;

DECLARE_COMPONENT_VERSION("MIDI synthesizer host", MYVERSION, "Special thanks go to DEATH's cat.\n\nEmu de MIDI alpha - Copyright (C) Mitsutaka Okazaki 2004\n\nVST Plug-In Technology by Steinberg.");

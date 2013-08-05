#ifndef __VSTiPlayer_h__
#define __VSTiPlayer_h__

#include <foobar2000.h>

#include <midi_container.h>

class VSTiPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
	};

	// zero variables
	VSTiPlayer();

	// close, unload
	~VSTiPlayer();

	// load, open, verify type
	bool LoadVST(const char * path);

	// must be loaded for the following to work
	void getVendorString(pfc::string_base & out);
	void getProductString(pfc::string_base & out);
	long getVendorVersion();
	long getUniqueID();

	// configuration
	void getChunk(pfc::array_t<t_uint8> & out);
	void setChunk( const void * in, unsigned size );

	// editor
	bool hasEditor();
	void displayEditorModal();

	// setup
	void setSampleRate(unsigned rate);
	unsigned getChannelCount();

	bool Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags);
	unsigned Play(audio_sample * out, unsigned count);
	void Seek(unsigned sample);

private:
	void send_event( DWORD );
	void render(audio_sample *, unsigned);

	unsigned test_plugin_platform();

	bool connect_pipe( HANDLE hPipe );
	bool process_create();
	void process_terminate();
	bool process_running();
	t_uint32 process_read_code();
	void process_read_bytes( void * buffer, t_uint32 size );
	t_uint32 process_read_bytes_pass( void * buffer, t_uint32 size );
	void process_write_code( t_uint32 code );
	void process_write_bytes( const void * buffer, t_uint32 size );

	pfc::string8 sPlugin;
	unsigned     uPluginPlatform;

	bool         bInitialized;
	HANDLE       hProcess;
	HANDLE       hThread;
	HANDLE       hReadEvent;
 	HANDLE       hChildStd_IN_Rd;
 	HANDLE       hChildStd_IN_Wr;
 	HANDLE       hChildStd_OUT_Rd;
 	HANDLE       hChildStd_OUT_Wr;

	char       * sName;
	char       * sVendor;
	char       * sProduct;
	t_uint32     uVendorVersion;
	t_uint32     uUniqueId;

	unsigned     uSamplesRemaining;

	unsigned     uSampleRate;
	unsigned     uLoopMode;
	unsigned     uNumOutputs;

	system_exclusive_table mSysexMap;
	std::vector<midi_stream_event> mStream;

	UINT         uStreamPosition;
	DWORD        uTimeCurrent;
	DWORD        uTimeEnd;

	UINT         uStreamLoopStart;
	DWORD        uTimeLoopStart;
};

#endif

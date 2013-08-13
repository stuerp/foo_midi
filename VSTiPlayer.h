#ifndef __VSTiPlayer_h__
#define __VSTiPlayer_h__

#include "MIDIPlayer.h"

class VSTiPlayer : public MIDIPlayer
{
public:
	// zero variables
	VSTiPlayer();

	// close, unload
	virtual ~VSTiPlayer();

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
	unsigned getChannelCount();

protected:
	virtual void send_event( DWORD );
	virtual void render(audio_sample *, unsigned);

	virtual void shutdown();
	virtual bool startup();

private:
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

	unsigned     uNumOutputs;

	pfc::array_t<t_uint8> blChunk;
};

#endif

#ifndef __VSTiPlayer_h__
#define __VSTiPlayer_h__

#include "MIDIPlayer.h"

typedef void* HANDLE;

class VSTiPlayer : public MIDIPlayer
{
public:
	// zero variables
	VSTiPlayer(bool disableMessagePump = false);

	// close, unload
	virtual ~VSTiPlayer();

	// load, open, verify type
	bool LoadVST(const char * path);

	// must be loaded for the following to work
	void getVendorString(std::string & out);
	void getProductString(std::string & out);
	long getVendorVersion();
	long getUniqueID();

	// configuration
	void getChunk(std::vector<uint8_t> & out);
	void setChunk( const void * in, unsigned long size );

	// editor
	bool hasEditor();
	void displayEditorModal();

	// setup
	unsigned getChannelCount();

protected:
	virtual void send_event( uint32_t );
	virtual void render(float *, unsigned long);

	virtual void shutdown();
	virtual bool startup();

private:
	unsigned test_plugin_platform();

	bool connect_pipe( HANDLE hPipe );
	bool process_create();
	void process_terminate();
	bool process_running();
	uint32_t process_read_code();
	void process_read_bytes( void * buffer, uint32_t size );
	uint32_t process_read_bytes_pass( void * buffer, uint32_t size );
	void process_write_code( uint32_t code );
	void process_write_bytes( const void * buffer, uint32_t size );

	std::string  sPlugin;
	unsigned     uPluginPlatform;

	bool         bDisableMessagePump;
	bool         bInitialized;
	bool         bTerminating;
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
	uint32_t     uVendorVersion;
	uint32_t    uUniqueId;

	unsigned     uNumOutputs;

	std::vector<uint8_t> blChunk;
};

#endif

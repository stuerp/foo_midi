#ifndef __VSTiPlayer_h__
#define __VSTiPlayer_h__

#include "MIDIPlayer.h"

typedef void* HANDLE;

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
	virtual unsigned int send_event_needs_time();
	virtual void send_event( uint32_t );
	virtual void send_sysex(const uint8_t* event, size_t size, size_t port);
	virtual void render(float *, unsigned long);

	virtual void shutdown();
	virtual bool startup();

	virtual void send_event_time(uint32_t b, unsigned int time);
	virtual void send_sysex_time(const uint8_t* event, size_t size, size_t port, unsigned int time);

private:
	unsigned test_plugin_platform();

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

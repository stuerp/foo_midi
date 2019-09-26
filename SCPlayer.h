#ifndef __SCPlayer_h__
#define __SCPlayer_h__

#include "MIDIPlayer.h"

#include <foobar2000.h>

extern char g_sc_name[];

class SCPlayer : public MIDIPlayer
{
public:
	// zero variables
	SCPlayer();

	// close, unload
	virtual ~SCPlayer();

	typedef enum
	{
		sc_default = 0,
		sc_gm,
		sc_gm2,
		sc_gs,
		sc_xg
	}
	sc_mode;

	typedef enum
	{
		gs_default = 0,
		gs_sc55,
		gs_sc88,
		gs_sc88pro,
		gs_sc8820,
	}
	sc_gs_level;
	
	void set_mode(sc_mode m);

	void set_gs_level(sc_gs_level l);

	void set_reverb(bool enable);

	void set_sccore_path(const char * path);

protected:
	virtual bool send_event_needs_time();
	virtual void send_event(uint32_t b);
	virtual void render(float * out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();
    
private:
	bool LoadCore(const char * path);

	void send_sysex_simple(uint32_t port, const uint8_t * data, uint32_t size);
	void send_sysex(uint32_t port, const uint8_t * data, uint32_t size);
	void send_gs(uint32_t port, uint8_t * data, uint32_t size);
	void reset_sc(uint32_t port);

	void send_command(uint32_t port, uint32_t command);

	void render_port(uint32_t port, float * out, uint32_t count);

	void reset(uint32_t port);

	void junk(uint32_t port, unsigned long count);

	unsigned test_plugin_platform();

	bool process_create(uint32_t port);
	void process_terminate(uint32_t port);
	bool process_running(uint32_t port);
	uint32_t process_read_code(uint32_t port);
	void process_read_bytes(uint32_t port, void * buffer, uint32_t size);
	uint32_t process_read_bytes_pass(uint32_t port, void * buffer, uint32_t size);
	void process_write_code(uint32_t port, uint32_t code);
	void process_write_bytes(uint32_t port, const void * buffer, uint32_t size);

	std::string  sPlugin;
	unsigned     uPluginPlatform;

	bool         initialized;
	int          iInitialized;
	bool         bTerminating[3];
	HANDLE       hProcess[3];
	HANDLE       hThread[3];
	HANDLE       hReadEvent[3];
	HANDLE       hChildStd_IN_Rd[3];
	HANDLE       hChildStd_IN_Wr[3];
	HANDLE       hChildStd_OUT_Rd[3];
	HANDLE       hChildStd_OUT_Wr[3];

	sc_mode mode;

	sc_gs_level level;

	bool reverb;

	char * sccore_path;
};

#endif

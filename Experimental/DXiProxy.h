#ifndef _DXI_PROXY_H_
#define _DXI_PROXY_H_

class CDXiPlayer;
class CMfxSeq;

class DXiProxy {
	bool initialized;
	CDXiPlayer *thePlayer;
	CMfxSeq *theSequence;

	public:
	DXiProxy();
	~DXiProxy();

	HRESULT initialize();
	void setSampleRate(unsigned);
	HRESULT setSequence(unsigned char *, unsigned);
	HRESULT setPlugin(CLSID);

	void Play(BOOL);
	void Stop();

	void fillBuffer(float *, unsigned);

	void setPosition(unsigned msec);

	void setLoop(unsigned loop_start, unsigned loop_end);
};

#endif

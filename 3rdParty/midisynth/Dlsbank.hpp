#ifndef dlsbank_h
#define dlsbank_h

#include <stdint.h>

#define DLSMAXREGIONS		512 // 128
#define DLSMAXENVELOPES		2048

// Region Flags
#define DLSREGION_KEYGROUPMASK		0x0F
#define DLSREGION_OVERRIDEWSMP		0x10
#define DLSREGION_PINGPONGLOOP		0x20
#define DLSREGION_SAMPLELOOP		0x40
#define DLSREGION_SELFNONEXCLUSIVE	0x80
#define DLSREGION_SUSTAINLOOP		0x100

typedef struct DLSREGION
{
	uint32_t ulLoopStart;
	uint32_t ulLoopEnd;
	uint16_t nWaveLink;
	uint16_t uPercEnv;
	uint16_t usVolume;		// 0..256
	uint16_t fuOptions;	// flags + key group
	int16_t sFineTune;	// 1..100
	uint8_t uKeyMin;
	uint8_t uKeyMax;
	uint8_t uUnityNote;
} DLSREGION;

typedef struct DLSENVELOPE
{
	// Volume Envelope
	float fVolAttack;       // Attack Time: 0.0-20.0 -> [0-20s]
	float fVolDecay;        // Decay Time: 0.0-20.0 -> [0-20s]
	float fVolRelease;      // Release Time: 0.0-20.0 -> [0-20s]
	float fVolSustainLevel;	// Sustain Level: 0.0-1.0, 1.0 = 100%
	float fVolDelay;        // Delay Time: 0.0-20.0 -> [0-20s]
	float fVolHold;         // Hold Level: 0.0-1.0, 1.0 = 100%
	// Default Pan
	float fDefPan;          // -1.0-1.0, 0.0 = Centered
} DLSENVELOPE;

// Special Bank bits
#define F_INSTRUMENT_DRUMS		0x80000000

typedef struct DLSINSTRUMENT
{
	uint32_t ulBank, ulInstrument;
	unsigned int nRegions, nMelodicEnv;
	DLSREGION Regions[DLSMAXREGIONS];
	char szName[32];
	// SF2 stuff (DO NOT USE! -> used internally by the SF2 loader)
	uint16_t wPresetBagNdx, wPresetBagNum;
} DLSINSTRUMENT;

typedef struct DLSSAMPLEEX
{
	char szName[20];
	uint32_t dwLen;
	uint32_t dwStartloop;
	uint32_t dwEndloop;
	uint32_t dwSampleRate;
	uint8_t byOriginalPitch;
	int8_t chPitchCorrection;
} DLSSAMPLEEX;

#define SOUNDBANK_TYPE_INVALID	0
#define SOUNDBANK_TYPE_DLS		0x01
#define SOUNDBANK_TYPE_SF2		0x02

typedef struct SOUNDBANKINFO
{
	char szBankName[256];
	char szCopyRight[256];
	char szComments[512];
	char szEngineer[256];
	char szSoftware[256];		// ISFT: Software
	char szDescription[256];	// ISBJ: Subject
} SOUNDBANKINFO;


//============
class CDLSBank
//============
{
protected:
	SOUNDBANKINFO m_BankInfo;
	char m_szFileName[1024];
	unsigned int m_nType;
	uint32_t m_dwWavePoolOffset;
	// DLS Information
	unsigned int m_nInstruments, m_nWaveForms, m_nEnvelopes, m_nSamplesEx, m_nMaxWaveLink;
	uint32_t *m_pWaveForms;
	DLSINSTRUMENT *m_pInstruments;
	DLSSAMPLEEX *m_pSamplesEx;
	DLSENVELOPE m_Envelopes[DLSMAXENVELOPES];

public:
	CDLSBank();
	virtual ~CDLSBank();
	void Destroy();
	static BOOL IsDLSBank(char const* lpszFileName);
	static DWORD MakeMelodicCode(UINT bank, UINT instr) { return ((bank << 16) | (instr));}
	static DWORD MakeDrumCode(UINT rgn, UINT instr) { return (0x80000000 | (rgn << 16) | (instr));}

public:
	bool Open(char const* lpszFileName);
	char const* GetFileName() const { return m_szFileName; }
	unsigned int GetBankType() const { return m_nType; }
	unsigned int GetBankInfo(SOUNDBANKINFO *pBankInfo=NULL) const { if (pBankInfo) *pBankInfo = m_BankInfo; return m_nType; }

public:
	unsigned int GetNumInstruments() const { return m_nInstruments; }
	unsigned int GetNumSamples() const { return m_nWaveForms; }
	DLSINSTRUMENT *GetInstrument(unsigned int iIns) { return (m_pInstruments) ? &m_pInstruments[iIns] : NULL; }
	DLSINSTRUMENT *FindInstrument(bool bDrum, unsigned int nBank=0xFF, uint32_t dwProgram=0xFF, uint32_t dwKey=0xFF, unsigned int *pInsNo=NULL);
	unsigned int GetRegionFromKey(unsigned int nIns, unsigned int nKey);
	bool FreeWaveForm(uint8_t * p);
	bool ExtractWaveForm(unsigned int nIns, unsigned int nRgn, uint8_t **ppWave, uint32_t *pLen);

// Internal Loader Functions
protected:
	BOOL UpdateInstrumentDefinition(DLSINSTRUMENT *pins, void *pchunk, uint32_t dwMaxLen);
	BOOL UpdateSF2PresetData(void *psf2info, void *pchunk, uint32_t dwMaxLen);
	BOOL ConvertSF2ToDLS(void *psf2info);

public:
	// DLS Unit conversion
	static int32_t __cdecl DLS32BitTimeCentsToMilliseconds(int32_t lTimeCents);
	static int32_t __cdecl DLS32BitRelativeGainToLinear(int32_t lCentibels);	// 0dB = 0x10000
	static int32_t __cdecl DLS32BitRelativeLinearToGain(int32_t lGain);		// 0dB = 0x10000
	static int32_t __cdecl DLSMidiVolumeToLinear(unsigned int nMidiVolume);		// [0-127] -> [0-0x10000]
};

#endif

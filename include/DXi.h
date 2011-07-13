#ifndef _DXI_H_
#define _DXI_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <DeferZeroFill.h>

////////////////////////////////////////////////////////////////////////////////

struct AudioBuffer
{
	long				cSamp;	// number of samples in the buffer
	DWORD				idPin;	// destination output pin for the buffer
	IMediaSample*	pms;		// the raw IMediaSample for this buffer

	AudioBuffer() : cSamp(0), idPin(0), pms(NULL) {}

	//----------------------------------------------------------------------------
	// Get a pointer to the audio samples, zero-filling if necesssary

	float* GetPointer()
	{
		// Get the raw-pointer
		BYTE* pb = NULL;
		pms->GetPointer( &pb );

		// We cannot defer the zero fill any longer!
		if (bZero)
		{
			IDeferZeroFill* pdzf;
			if (SUCCEEDED( pms->QueryInterface( IID_IDeferZeroFill, (void**)&pdzf ) ))
			{
				// IDeferZeroFill will have taken care of the zero-fill for us, by
				// virtue of our calling IMediaSample::GetPointer.  Nothing more to do.
				pdzf->Release();
			}
			else
			{
				// No IDeferZeroFill is available.  We must zero-fill the hard way.
				memset( pb, 0, cSamp * sizeof(float) );
			}
			bZero = FALSE;
		}

		return reinterpret_cast<float*>( pb );
	}

	//----------------------------------------------------------------------------
	// Allow buffers to be tagged as being all zeroes, without actually filling
	// any data until someone asks for the buffer pointer

	BOOL GetZerofill() const { return bZero; }

	void SetZerofill( BOOL bZerofill )
	{
		bZero = bZerofill;
		IDeferZeroFill* pdzf;
		if (SUCCEEDED( pms->QueryInterface( IID_IDeferZeroFill, (void**)&pdzf ) ))
		{
			pdzf->put_NeedsZerofill( bZero );
			pdzf->Release();
		}
	}

private:

	BOOL bZero;
};

////////////////////////////////////////////////////////////////////////////////

struct DXiEvent
{
	// Helper class for managing queues of MfxEvents

	MfxEvent		me;						// the original MIDI event
	MFX_CHANNEL	mfxChannel;				// which MFX channel the event originated from
	LONGLONG		llSampTimestamp;		// timestamp on the synth's free running clock
	void*			pvData;					// additional synth-specific data, aka, voice info
	DWORD			dwFlags;					// additional information about this event

	enum
	{
		F_LIVE		= 0x00000001,	// this event is being played live, just-in-time
		F_STREAMED	= 0x00000002,	// this event is being streamed from a track
	};

	inline bool operator< ( const DXiEvent& rhs ) const
	{
		return llSampTimestamp < rhs.llSampTimestamp;
	}

	inline bool operator==( const DXiEvent& rhs ) const
	{
		return llSampTimestamp == rhs.llSampTimestamp;
	}
};

////////////////////////////////////////////////////////////////////////////////

class CDXiSynthContext
{
public:
	virtual HRESULT	CreateOutputPin( const char* pszPinName = NULL, DWORD* pidCreated = NULL ) = 0;
	virtual HRESULT	DestroyOutputPin( DWORD id ) = 0;
	virtual HRESULT	DestroyAllOutputPins() = 0;
	virtual CBasePin*	FindOutputPin( DWORD id ) const = 0;
	virtual int			GetOutputPinCount() const = 0;
	virtual DWORD		GetOutputPinId( int nIndex ) const = 0;
	virtual LONGLONG	TicksToSamples( LONG lTicks ) const = 0;
	virtual LONG		SamplesToTicks( LONGLONG llSamples ) const = 0;
};

////////////////////////////////////////////////////////////////////////////////

class CDXi : public CCritSec
{
public:
	CDXi( CDXiSynthContext* pCtx ) : m_pCtx(pCtx) {}

	virtual HRESULT Initialize() = 0;

	virtual HRESULT IsValidInputFormat( const WAVEFORMATEX* pwfx ) const = 0;
	virtual HRESULT IsValidOutputFormat( const WAVEFORMATEX* pwfx ) const = 0;
	virtual HRESULT IsValidTransform( const WAVEFORMATEX* pwfxIn, const WAVEFORMATEX* pwfxOut ) const = 0;
	virtual HRESULT SuggestOutputFormat( WAVEFORMATEX* pwfx ) const = 0;

	virtual const WAVEFORMATEX* GetInputFormat() const { return &m_wfxIn; }
	virtual const WAVEFORMATEX* GetOutputFormat() const { return &m_wfxOut; }

	virtual HRESULT	Process( LONGLONG llSampAudioTimestamp,
										AudioBuffer* pbufIn,
										AudioBuffer* abufOut, long cBufOut,
										LONGLONG llSampMidiClock, deque<DXiEvent>& qMidi ) = 0;

	virtual HRESULT	IsValidEvent( DXiEvent& de ) = 0;
 	virtual HRESULT	InitializeNoteEvent( DXiEvent& de ) = 0;
	virtual HRESULT	ExpireNoteEvent( DXiEvent& de, BOOL bForce ) = 0;

	virtual HRESULT	AllocateResources() = 0;
	virtual HRESULT	FreeResources() = 0;

	virtual int			PersistGetSize() const = 0;
	virtual HRESULT	PersistLoad( IStream* pStream ) = 0;
	virtual HRESULT	PersistSave( IStream* pStream ) = 0;

protected:
	CDXiSynthContext*	m_pCtx;
	WAVEFORMATEX		m_wfxIn;
	WAVEFORMATEX		m_wfxOut;
};

////////////////////////////////////////////////////////////////////////////////

#endif //_DXI_H_

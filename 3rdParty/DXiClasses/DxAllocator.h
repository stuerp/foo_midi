// DxAllocator.h: interface for the CDxAllocator class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#ifndef _DXALLOCATOR_H_
#define _DXALLOCATOR_H_

#include <DeferZeroFill.h>

////////////////////////////////////////////////////////////////////////////////

extern const GUID IID_IDeferZeroFillAllocator;

#undef  INTERFACE
#define INTERFACE IDeferZeroFillAllocator

DECLARE_INTERFACE_( IDeferZeroFillAllocator, IUnknown )
{
	// *** IUnknown methods ***
	STDMETHOD_(HRESULT,	QueryInterface)( THIS_ REFIID riid, LPVOID* ppvObj ) PURE;
	STDMETHOD_(ULONG,		AddRef)( THIS )  PURE;
	STDMETHOD_(ULONG,		Release)( THIS ) PURE;

	// *** IDeferZeroFillAllocator methods ***
	STDMETHOD_(HRESULT,	ResetProperties)( THIS_ long cBufInitial ) PURE;
	STDMETHOD_(HRESULT,	AllocateAll)( THIS ) PURE;
	STDMETHOD_(HRESULT,	FreeAll)( THIS ) PURE;
};

////////////////////////////////////////////////////////////////////////////////

class CDxAllocator : public CBaseAllocator,
							public IDeferZeroFillAllocator
{
public:

	// Factory-style creation
	static HRESULT Create( CDxAllocator** ppAlloc );

	// IUnknown
	STDMETHODIMP_(HRESULT)	QueryInterface( REFIID riid, void** ppv );
	STDMETHODIMP_(ULONG)		AddRef();
	STDMETHODIMP_(ULONG)		Release();

	// IDeferZeroFillAllocator
	STDMETHODIMP SetPropertiesEx( REFCLSID guidFilter, ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual );
	STDMETHODIMP ResetProperties( long cBufInitial );
	STDMETHODIMP AllocateAll();
	STDMETHODIMP FreeAll();

	// IMemAllocator overrides
	STDMETHODIMP SetProperties( ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual );
	STDMETHODIMP GetProperties( ALLOCATOR_PROPERTIES* pActual );
	STDMETHODIMP GetBuffer( IMediaSample** ppms, REFERENCE_TIME* ptStart, REFERENCE_TIME* ptEnd, DWORD dwFlags );
	STDMETHODIMP ReleaseBuffer( IMediaSample* pms );
	STDMETHODIMP Commit();
	STDMETHODIMP Decommit();

	long GetMaxBufferSize() const { return m_lSizeMax; }
	void SetMaxBufferSize( long l ) { m_lSizeMax = l; }

protected:

	// Override to free the memory when decommit completes
	void Free(void);

	// Override to allocate the memory when commit called
	HRESULT Alloc(void);

private:

	STDMETHODIMP PrepSetProperties( ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual, LONG* plSize );

private:

	void*		m_pBuf;				// combined memory for all buffers
	BOOL		m_bPropsWild;		// TRUE when no properties have been set yet, so anything goes
	long		m_lMaxUsed;			// maximum number of buffers that were acquired by filters
	long		m_lInUse;			// current number of buffers in use
	long		m_lSizeMax;			// maximum buffer size we'll allow

private:

	CDxAllocator( TCHAR* pszName, LPUNKNOWN pUnk, HRESULT* phr );
	virtual ~CDxAllocator();
};

////////////////////////////////////////////////////////////////////////////////

class CDeferZeroFillMediaSample : public CMediaSample,
											 public IDeferZeroFill
{
public:

	CDeferZeroFillMediaSample( TCHAR* pszName, CBaseAllocator* pAlloc, HRESULT* phr, BYTE* pb = NULL, long cb = 0 );

	STDMETHODIMP GetPointer( BYTE** ppBuffer );

	// IUnknown
	STDMETHODIMP_(HRESULT)	QueryInterface( REFIID riid, void** ppv );
	STDMETHODIMP_(ULONG)		AddRef();
	STDMETHODIMP_(ULONG)		Release();

	// IDeferZeroFill
	STDMETHODIMP_(BOOL)		get_NeedsZerofill();
	STDMETHODIMP_(void)		put_NeedsZerofill( BOOL bZero );
	STDMETHODIMP_(HRESULT)	GetRawPointer( BYTE** ppBuffer );

private:
	BOOL m_bNeedsZerofill;
};

////////////////////////////////////////////////////////////////////////////////

// Checks if the allocator is an instance of our DeferZeroFillAllocator
BOOL IsDeferZeroFillAllocator( IMemAllocator* pAlloc, CDxAllocator** ppAudMemAlloc );

// Is this a media sample which is still deferred for zerofill?
BOOL IsDeferredZerofill( IMediaSample* pms );

// Fill a media sample with zeroes, trying to defer the work via IDeferZeroFill
HRESULT DeferZeroFill( IMediaSample* pms );

// Get a raw buffer pointer, bypassing any zero-fill steps
HRESULT GetMediaSamplePointer( IMediaSample* pms, BYTE** ppBuffer );

#endif // _DXALLOCATOR_H_


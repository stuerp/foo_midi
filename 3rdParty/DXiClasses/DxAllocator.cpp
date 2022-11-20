// DxDxAllocator.cpp - Implementation of allocator classes (allocator
// and media sample).
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <InitGuid.h>
#include "DxAllocator.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const GUID IID_IDeferZeroFillAllocator =
	{ 0x243e2ac0, 0x59bd, 0x11d1, { 0x9a, 0x70, 0x0, 0xa0, 0xc9, 0xd, 0xa1, 0xef } };


// Since we know that only one thread will ever be calling GetDeliveryBuffer(),
// we can avoid some O/S synchronization overhead and bypass DShow's default
// implemention which uses critical sections and semaphores.  For safety's sake,
// the code can be compiled back in via this flag.
//
#define MULTITHREADED_ALLOCATOR (FALSE)


//------------------------------------------------------------------------------
// Fill a buffer with with psuedo-zero values (tiny DC offset), to avoid
// denormalization problems from downstream plug-ins.

static void fillWithTinyDC( float* pfBuf, int nBufLen )
{
	/*
	static float TINY_DC = 1e-30f;
	for (int ix = 0; ix < nBufLen; ix++)
		pfBuf[ ix ] = TINY_DC;
	*/
	memset(pfBuf, 0, nBufLen * sizeof(float));
}


//------------------------------------------------------------------------------
// Checks if the allocator is an instance of our DeferZeroFillAllocator
// If true, return the CDxAllocator interface. 

BOOL IsDeferZeroFillAllocator( IMemAllocator* pAlloc, CDxAllocator** ppAudMemAlloc )
{
	IDeferZeroFillAllocator* pIAudMemAlloc = NULL;

	ASSERT( ppAudMemAlloc );
	*ppAudMemAlloc = NULL;

	// If the allocator isn't our custom allocator, fail right away
	if (S_OK != pAlloc->QueryInterface( IID_IDeferZeroFillAllocator, (void**)&pIAudMemAlloc ))
		return FALSE;

	// Release the pointer we just got above, and give them a pointer to
	// the real allocator object.
	pIAudMemAlloc->Release();
	*ppAudMemAlloc = (CDxAllocator*)( pAlloc );
	ASSERT( *ppAudMemAlloc );
	return TRUE;

}


//------------------------------------------------------------------------------
// Is this a media sample which is still deferred for zerofill?

BOOL IsDeferredZerofill( IMediaSample* pms )
{
	IDeferZeroFill* pdzf = 0;
	if (S_OK != pms->QueryInterface( IID_IDeferZeroFill, (void**)&pdzf ) )
		return FALSE;

	BOOL bRet = pdzf->get_NeedsZerofill();
	pdzf->Release();
	return bRet;
}


//------------------------------------------------------------------------------
// Fill a media sample with zeroes, trying to defer the work via IDeferZeroFill

HRESULT DeferZeroFill( IMediaSample* pms )
{
	// If it supports IDeferZeroFill, just mark it as zero.  Otherwise, zerofill it.
	IDeferZeroFill* pdzf = 0;
	if (S_OK == pms->QueryInterface( IID_IDeferZeroFill, (void**)&pdzf ))
	{
		pdzf->put_NeedsZerofill( TRUE );
		pdzf->Release();
	}
	else
	{
		BYTE* pb;
		CHECK( pms->GetPointer( &pb ) );
		fillWithTinyDC( (float*)pb, pms->GetSize() / sizeof(float) );
	}

	return S_OK;
}


//--------------------------------------------------------------------------------
// Get a raw buffer pointer, bypassing any zero-fill steps

HRESULT GetMediaSamplePointer( IMediaSample* pms, BYTE** ppBuffer )
{
	IDeferZeroFill* pdzf = 0;
	if (S_OK != pms->QueryInterface( IID_IDeferZeroFill, (void**)&pdzf ) )
	{
		return pms->GetPointer( ppBuffer );
	}
	else
	{
		HRESULT hr = pdzf->GetRawPointer( ppBuffer );
		pdzf->Release();
		CHECK( hr );
		return S_OK;
	}
}


//******************************************************************************
// CDxAllocator - Custom allocator that spews out CDeferZeroFillMediaSamples,
// a custom media sample that provide "lazy evaluation" of silent buffers.
// Note that this class is a nearly verbatim copy the base-class CMemAllocator.
//******************************************************************************

//------------------------------------------------------------------------------
// Factory style creation

HRESULT CDxAllocator::Create( CDxAllocator** ppAlloc )
{
	CheckPointer( ppAlloc, E_POINTER );
	ValidateReadWritePtr( ppAlloc, sizeof(IMemAllocator*) );

	HRESULT hr = S_OK;

	// Create our custom allocator
	CDxAllocator* pAlloc = new CDxAllocator( NAME("MemAlloc"), NULL, &hr );
	if (!pAlloc)
		hr = E_OUTOFMEMORY;
	CHECK( hr );

	// Return it to the caller
	pAlloc->AddRef();
	*ppAlloc = pAlloc;

	return S_OK;
}


//------------------------------------------------------------------------------
// Ctor

CDxAllocator::CDxAllocator( TCHAR* pszName, LPUNKNOWN pUnk, HRESULT* phr) :
	CBaseAllocator(pszName, pUnk, phr),
	m_bPropsWild(TRUE),
	m_lSizeMax(128*1024),
	m_pBuf(NULL)
{
	ResetProperties( 0 );
}


//------------------------------------------------------------------------------
// Expose our IDeferZeroFillAllocator interface

HRESULT CDxAllocator::QueryInterface( REFIID riid, void** ppv )
{
	if (IID_IDeferZeroFillAllocator == riid)
	{
		*ppv = (IDeferZeroFillAllocator*) this;
		AddRef();
		return S_OK;
	}
	else
		return CBaseAllocator::QueryInterface( riid, ppv );
}

ULONG CDxAllocator::AddRef()
{
	return CBaseAllocator::AddRef();
}

ULONG CDxAllocator::Release()
{
	return CBaseAllocator::Release();
}

//------------------------------------------------------------------------------

HRESULT CDxAllocator::PrepSetProperties( ALLOCATOR_PROPERTIES* pRequest,
													  ALLOCATOR_PROPERTIES* pActual,
													  LONG* plSize )
{
	CheckPointer( pActual, E_POINTER );
	ValidateReadWritePtr( pActual, sizeof(ALLOCATOR_PROPERTIES) );
	
	ZeroMemory( pActual, sizeof(ALLOCATOR_PROPERTIES) );
	
	ASSERT( pRequest->cbBuffer > 0 );
	
	// Can't do this if already committed, there is an argument that says we
	// should not reject the SetProperties call if there are buffers still
	// active. However this is called by the source filter, which is the same
	// person who is holding the samples. Therefore it is not unreasonable
	// for them to free all their samples before changing the requirements
	if (m_bCommitted)
		CHECK(VFW_E_ALREADY_COMMITTED);
	
	// Must be no outstanding buffers
	if (m_lFree.GetCount() < m_lAllocated)
		CHECK(VFW_E_BUFFERS_OUTSTANDING);
	
	// There isn't any real need to check the parameters as they
	// will just be rejected when the user finally calls Commit
	
	// Round length up to alignment - remember that prefix is included in
	// the alignment
	LONG lSize = pRequest->cbBuffer + pRequest->cbPrefix;
	LONG lRemainder = lSize % pRequest->cbAlign;
	if (lRemainder)
		lSize = lSize - lRemainder + pRequest->cbAlign;
	*plSize = lSize;

	return S_OK;
}


//------------------------------------------------------------------------------
// This sets the size and count of the required samples. The memory isn't
// actually allocated until Commit() is called, if memory has already been
// allocated then assuming no samples are outstanding the user may call us
// to change the buffering, the memory will be released in Commit()

STDMETHODIMP CDxAllocator::SetProperties( ALLOCATOR_PROPERTIES*	pRequest,
													   ALLOCATOR_PROPERTIES* pActual )
{
	CAutoLock	cObjectLock(this);
	LONG			lSize;

	CHECK( PrepSetProperties( pRequest, pActual, &lSize ) );

	BOOL const bPropsWild = m_bPropsWild;
	m_bPropsWild = FALSE;
	if (bPropsWild)
	{
		m_lAlignment	= pRequest->cbAlign;
		m_lPrefix		= pRequest->cbPrefix;
		m_bPropsWild	= FALSE;
	}
	else
	{
		m_lAlignment = max( m_lAlignment, pRequest->cbAlign );
		ASSERT( m_lPrefix == pRequest->cbPrefix );
	}

	m_lCount += pRequest->cBuffers;

	m_lSize = max( m_lSize, lSize - m_lPrefix );
	m_lSize = min( GetMaxBufferSize(), m_lSize );

	// Even though we are going to allocate more and larger buffers than each
	// individual request, we still give each caller exactly the answer they
	// are looking for.
	pActual->cbBuffer	= (lSize - pRequest->cbPrefix);
	pActual->cBuffers	= pRequest->cBuffers;
	pActual->cbAlign	= pRequest->cbAlign;
	pActual->cbPrefix	= pRequest->cbPrefix;
	m_bChanged = TRUE;

	return S_OK;
}


//------------------------------------------------------------------------------
// Return some meaningful value for our current properties.  We need to override
// the base class implemention of this, because it will report *all* buffer we
// currently are using, not just some minimum.

STDMETHODIMP CDxAllocator::GetProperties( ALLOCATOR_PROPERTIES* pActual )
{
#ifdef _DEBUG
	CheckPointer(pActual,E_POINTER);
	ValidateReadWritePtr(pActual,sizeof(ALLOCATOR_PROPERTIES));
#endif

	CAutoLock cObjectLock(this);

	pActual->cbBuffer = m_lSize;
	pActual->cBuffers = min( m_lCount, 2 );
	pActual->cbAlign = m_lAlignment;
	pActual->cbPrefix = m_lPrefix;
	
	return S_OK;
}


//------------------------------------------------------------------------------
// Clear out all allocator properties, e.g., to prepare for a reconnect.

HRESULT CDxAllocator::ResetProperties( long cBufInitial )
{
	// Free the block of memory for real
	if (m_pBuf)
	{
		VirtualFree( m_pBuf, 0, MEM_RELEASE );
		m_pBuf = NULL;
	}

	// Reset all properties
	m_bPropsWild = TRUE;
	m_lSize = 0;
	m_lCount = cBufInitial;
	m_lAlignment = 0;
	m_lPrefix = 0;
	m_lMaxUsed = 0;
	m_lInUse = 0;
	m_bChanged = TRUE;

	return S_OK;
}


//------------------------------------------------------------------------------
// Give a buffer the caller, flagging it as being fully silent.

STDMETHODIMP CDxAllocator::GetBuffer( IMediaSample** ppms,
												  REFERENCE_TIME* ptStart,
												  REFERENCE_TIME* ptEnd,
												  DWORD dwFlags )
{
	// The following code is mostly lifted from AMFilter.cpp, in DirectShow class library.
	// It has been changed to use IDeferZeroFill, and to be conditionally compiled
	// for single-threaded access.

	UNREFERENCED_PARAMETER( ptStart );
	UNREFERENCED_PARAMETER( ptEnd );
	UNREFERENCED_PARAMETER( dwFlags );

	CMediaSample* pSample;

	*ppms = NULL;
	for (;;)
	{
		{
			#if MULTITHREADED_ALLOCATOR
				CAutoLock cObjectLock(this);
			#endif // MULTITHREADED_ALLOCATOR

			// Check we are committed
			if (!m_bCommitted)
				CHECK(VFW_E_NOT_COMMITTED);
			pSample = (CMediaSample*) m_lFree.RemoveHead();

			#if MULTITHREADED_ALLOCATOR
				if (pSample == NULL)
					SetWaiting();
			#endif // MULTITHREADED_ALLOCATOR
		}

		if (pSample)
			break;

		#if MULTITHREADED_ALLOCATOR
			// If we didn't get a sample then wait for the list to signal
			if (dwFlags & AM_GBF_NOWAIT)
				CHECK(VFW_E_TIMEOUT);

			// Here, DirectShow waits INFINITE.  We wait for 5 seconds.
			ASSERT(m_hSem != NULL);
			if (WAIT_TIMEOUT == WaitForSingleObject(m_hSem, 5000))
				CHECK(VFW_E_TIMEOUT);
		#else
			// No sample is ready, so we die
			if (NULL == pSample)
				CHECK(E_OUTOFMEMORY);
		#endif // MULTITHREADED_ALLOCATOR
	}

	// Addref the buffer up to one. On release
	// back to zero instead of being deleted, it will requeue itself by
	// calling the ReleaseBuffer member function. NOTE the owner of a
	// media sample must always be derived from CBaseAllocator

	ASSERT( pSample->m_cRef == 0 );
	pSample->m_cRef = 1;
	*ppms = pSample;

	// Mark the newly gotten buffer as not needing zero filling
	if (*ppms)
	{
		IDeferZeroFill* pdzf = 0;
		CHECK( (*ppms)->QueryInterface( IID_IDeferZeroFill, (void**)&pdzf ) );
		ASSERT( pdzf );
		pdzf->put_NeedsZerofill( FALSE );
		pdzf->Release();
	}

	++m_lInUse;
	m_lMaxUsed = max( m_lMaxUsed, m_lInUse );

	return S_OK;
}


//------------------------------------------------------------------------------
// A buffer's ref-count has gone back to zero.

HRESULT CDxAllocator::ReleaseBuffer( IMediaSample* pms )
{
	// The following code is mostly lifted from AMFilter.cpp, in DirectShow class library.
	// It has been changed to be conditionally compiled for single-threaded access.

	if (NULL == pms)
		CHECK(E_POINTER);

	BOOL bRelease = FALSE;
	{
		#if MULTITHREADED_ALLOCATOR
			CAutoLock cObjectLock(this);
		#endif // MULTITHREADED_ALLOCATOR

		// Put back on the free list
		m_lFree.Add( (CMediaSample*) pms );

		#if MULTITHREADED_ALLOCATOR
			if (m_lWaiting != 0)
				NotifySample();
		#endif // MULTITHREADED_ALLOCATOR
		
		// If there is a pending Decommit, then we need to complete it by
		// calling Free() when the last buffer is placed on the free list
		LONG l1 = m_lFree.GetCount();
		if (m_bDecommitInProgress && (l1 == m_lAllocated))
		{
			Free();
			m_bDecommitInProgress = FALSE;
			bRelease = TRUE;
		}
	}
	
	if (m_pNotify)
	{
		// Note that this is not synchronized with setting up a notification method.
		ASSERT(m_fEnableReleaseCallback);
		m_pNotify->NotifyRelease();
	}
	
	// For each buffer there is one AddRef, made in GetBuffer and released
	// here. This may cause the allocator and all samples to be deleted.
	if (bRelease)
		Release();

	--m_lInUse;

	return S_OK;
}


//------------------------------------------------------------------------------
// One of our clients really wants to start using the memory

HRESULT CDxAllocator::Commit()
{
	return S_OK;
}

HRESULT CDxAllocator::Decommit()
{
	return S_OK;
}


//------------------------------------------------------------------------------
// No-op when called by DShow filters.  Allocations really happen when
// CDxFilterGraph says so.

HRESULT CDxAllocator::Alloc(void)
{
	return S_OK;
}

//------------------------------------------------------------------------------
// Free up any resources we have allocated.  Called from the base class on
// Decommit when all buffers have been returned to the free list.
// In our case, we keep the memory until we are deleted, so
// we do nothing here. The memory is deleted when CDxFilterGraph says so.

void CDxAllocator::Free(void)
{
	return;
}

//------------------------------------------------------------------------------
// Allocate all of our memory

HRESULT CDxAllocator::AllocateAll()
{
	CAutoLock	lck(this);
	HRESULT		hr = S_OK;
	
	// No-op if there's nothing to do
	if (0 == m_lCount && 0 == m_lSize)
		return S_OK;

	// Make sure they called SetProperties
	if (m_lCount <= 0 || m_lSize <= 0 || m_lAlignment <= 0)
		CHECK(VFW_E_SIZENOTSET);

	DWORD const cb = m_lCount * (m_lSize + m_lPrefix);

	// If the requirements haven't changed then don't reallocate
	if (!m_bChanged)
	{
		ASSERT( m_pBuf );
	}
	else
	{
		// Free the old resources
		FreeAll();
		if (m_pBuf)
		{
			VirtualFree( m_pBuf, 0, MEM_RELEASE );
			m_pBuf = NULL;
		}
	
		// Create the contiguous memory block for the samples,
		// making sure it's properly aligned
		ASSERT( m_lAlignment && !((m_lSize + m_lPrefix) % m_lAlignment) );
	
		// Allocate a new buffer
		m_pBuf = VirtualAlloc( NULL, cb, MEM_COMMIT, PAGE_READWRITE );
		if (!m_pBuf)
			CHECK(E_OUTOFMEMORY);
	}

//	TRACE("Alloc %d * (%d + %d) = %d bytes\r\n", m_lCount, m_lSize, m_lPrefix, cb);

	BYTE* pNext = static_cast<BYTE*>( m_pBuf );
	CDeferZeroFillMediaSample* pms;
	
	ASSERT( !m_lAllocated );
	
	// Create the new samples - we have allocated m_lSize bytes for each sample
	// plus m_lPrefix bytes per sample as a prefix. We set the pointer to
	// the memory after the prefix - so that GetPointer() will return a pointer
	// to m_lSize bytes.
	for (; m_lAllocated < m_lCount; m_lAllocated++, pNext += (m_lSize + m_lPrefix))
	{
		pms = new CDeferZeroFillMediaSample( NAME("DeferZeroFillMediaSample"), this, &hr,
											pNext + m_lPrefix,		// GetPointer() value,
											m_lSize );					// not including prefix
	
		if (!pms)
			hr = E_OUTOFMEMORY;
		CHECK( hr );
	
		// This CANNOT fail
		m_lFree.Add( pms );
	}
	
	m_lMaxUsed = 0;
	m_lInUse = 0;
	m_bCommitted = TRUE;
	m_bChanged = FALSE;
	return S_OK;
}


//------------------------------------------------------------------------------
// Called from the destructor (and from Alloc if changing size/count) to
// actually free up the memory

HRESULT CDxAllocator::FreeAll()
{
	CAutoLock lck(this);

//	if (m_lCount && m_lSize)
//	{
//		DWORD const cb = m_lCount * (m_lSize + m_lPrefix);
//		TRACE("Free %d * (%d + %d) = %d bytes : %d used last time\r\n", m_lCount, m_lSize, m_lPrefix, cb, m_lMaxUsed);
//	}

	// Free the CDeferZeroFillMediaSamples
	CMediaSample* pms;
	do
	{
		pms = m_lFree.RemoveHead();
		if (pms)
			delete pms;
	}
	while (pms != NULL);
	
	m_bCommitted = FALSE;
	m_lAllocated = 0;

	return S_OK;
}


//------------------------------------------------------------------------------
// Destructor frees our memory

CDxAllocator::~CDxAllocator()
{
	FreeAll();
}


//******************************************************************************
// CDeferZeroFillMediaSample -  Custom media sample that provide "lazy evaluation" of
// silent buffers.
//******************************************************************************

//------------------------------------------------------------------------------
// Ctor

CDeferZeroFillMediaSample::CDeferZeroFillMediaSample(
	TCHAR*				pszName,
	CBaseAllocator*	pAlloc,
	HRESULT*				phr,
	BYTE*					pb /* = NULL */,
	long					cb /* = 0 */
) :
	CMediaSample( pszName, pAlloc, phr, pb, cb )
{
	m_bNeedsZerofill = FALSE;
}


//------------------------------------------------------------------------------
// IUnknown

HRESULT CDeferZeroFillMediaSample::QueryInterface( REFIID riid, void** ppv )
{
	if (IID_IDeferZeroFill == riid)
	{
		*ppv = (IDeferZeroFill*) this;
		AddRef();
		return S_OK;
	}
	else
		return CMediaSample::QueryInterface( riid, ppv );
}

ULONG CDeferZeroFillMediaSample::AddRef()
{
	return CMediaSample::AddRef();
}

ULONG CDeferZeroFillMediaSample::Release()
{
	return CMediaSample::Release();
}


//------------------------------------------------------------------------------
// IDeferZeroFill

BOOL CDeferZeroFillMediaSample::get_NeedsZerofill() { return m_bNeedsZerofill; }
void CDeferZeroFillMediaSample::put_NeedsZerofill( BOOL bZero ) { m_bNeedsZerofill = bZero; }


//------------------------------------------------------------------------------
// Return a pointer to our buffer.  If we are a zero-filled buffer, we
// must do the actual work of zerofilling here and now.

STDMETHODIMP CDeferZeroFillMediaSample::GetPointer( BYTE** ppBuffer )
{
	// Ask the base class to get the buffer
	CHECK( CMediaSample::GetPointer( ppBuffer ) );

	// Do we need to zerofill?
	if (ppBuffer && m_bNeedsZerofill)
	{
		long cbFill = GetActualDataLength();
		if (!( cbFill >= 0 && cbFill <= m_cbBuffer ))
			cbFill = m_cbBuffer;

		fillWithTinyDC( (float*)(*ppBuffer), cbFill / sizeof(float) );
		m_bNeedsZerofill = FALSE;
	}

	return S_OK;
}

//------------------------------------------------------------------------------
// Return a "raw" pointer to our buffer, without zerofilling.

HRESULT CDeferZeroFillMediaSample::GetRawPointer( BYTE** ppBuffer )
{
	// Ask the base class to get the buffer
	CHECK( CMediaSample::GetPointer( ppBuffer ) );

	// If we were a deferred zero-fill buffer, make sure no subseqent calls
	// to GetPointer will actually zero-fill the buffer.
	m_bNeedsZerofill = FALSE;

	return S_OK;
}

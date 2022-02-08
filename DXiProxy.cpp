#include "StdAfx.h"

#include "DXiProxy.h"

#include "DXiPlayer.h"
#include "PlugInInventory.h"

#include "MfxBufferFactory.h"
#include "MfxSeq.h"

#include "../../pfc/pfc.h"

HRESULT LoadMid(CMfxSeq& rSeq, CFile& rFile);

DXiProxy::DXiProxy() {
	initialized = false;
	thePlayer = NULL;
	theSequence = NULL;
}

DXiProxy::~DXiProxy() {
	delete thePlayer;
	delete theSequence;
	if(initialized) CoUninitialize();
}

void DXiProxy::setLoop(unsigned loop_start, unsigned loop_end) {
	if(thePlayer) {
		thePlayer->SetLoopStart(loop_start);
		thePlayer->SetLoopEnd(loop_end);
		thePlayer->SetLooping(TRUE);
	}
}

void DXiProxy::setPosition(unsigned msec) {
	if(theSequence && thePlayer) {
		unsigned tick = theSequence->m_tempoMap.Sample2Tick(msec, 1000);

		thePlayer->Stop();
		thePlayer->SetPosition(tick);
		thePlayer->Play(TRUE);
	}
}

HRESULT DXiProxy::initialize() {
	CoInitialize(NULL);
	initialized = true;

	thePlayer = new CDXiPlayer;

	return thePlayer->Initialize();
}

void DXiProxy::setSampleRate(unsigned srate) {
	if(thePlayer) thePlayer->SetSampleRate(srate);
}

HRESULT DXiProxy::setSequence(unsigned char* sequence, unsigned size) {
	if(thePlayer) {
		theSequence = new CMfxSeq;
		HRESULT ret = LoadMid(*theSequence, CMemFile(sequence, size));
		if(SUCCEEDED(ret)) {
			ret = thePlayer->SetSeq(theSequence);
		}
		return ret;
	}
	return E_INVALIDARG;
}

HRESULT DXiProxy::setPlugin(CLSID plugin) {
	if(thePlayer) {
		CPlugInInventory theInventory;
		HRESULT ret = theInventory.EnumPlugIns();
		if(SUCCEEDED(ret)) {
			unsigned i, j;

			pfc::string8_fastalloc temp;

			for(i = 0, j = theInventory.GetCount(); i < j; ++i) {
				CLSID theClsid;
				if(SUCCEEDED(theInventory.GetInfo(i, &theClsid, temp)) && theClsid == plugin) break;
			}

			if(i == j) return E_INVALIDARG;

			IBaseFilter* pFilter = NULL;
			ret = theInventory.CreatePlugIn(i, &pFilter);
			if(SUCCEEDED(ret)) {
				thePlayer->SetFilter(pFilter);
				pFilter->Release();
			}
		}

		return ret;
	}
	return E_INVALIDARG;
}

void DXiProxy::Play(BOOL bPlaySeq) {
	if(thePlayer) thePlayer->Play(bPlaySeq);
}

void DXiProxy::Stop() {
	if(thePlayer) thePlayer->Stop();
}

void DXiProxy::fillBuffer(float* buffer, unsigned count) {
	if(thePlayer) thePlayer->FillBuffer(buffer, count);
}

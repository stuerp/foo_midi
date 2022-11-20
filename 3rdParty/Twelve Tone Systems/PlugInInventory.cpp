// PlugInInventory.cpp: implementation of the CPlugInInventory class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <uuids.h>

#include <foobar2000.h>

#include "PlugInInventory.h"

#include <initguid.h>

DEFINE_GUID(CLSID_KMixer,
            0x17CCA71B, 0xECD7, 0x11D0, 0xB9, 0x08, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

static const char* SZ_MFX_SYNTH_REGKEY = "MfxSoftSynths"; // HKEY_CLASSES_ROOT/MfxSoftSynths

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlugInInventory::CPlugInInventory() {
	m_cRegFilter = 0;
	m_aRegFilter = NULL;
}

CPlugInInventory::~CPlugInInventory() {
	if(m_aRegFilter) {
		for(ULONG ix = 0; ix < m_cRegFilter; ix++)
			CoTaskMemFree((void*)m_aRegFilter[ix].Name);
		SAFE_ARRAY_DELETE(m_aRegFilter);
	}
	m_cRegFilter = 0;
}

////////////////////////////////////////////////////////////////////////////////
// Build the list of all installed DirectShow filters

HRESULT CPlugInInventory::EnumPlugIns() {
	// Get the filter mapper.  If this initial step fails, then it's most
	// likely that DirectX isn't installed.
	IFilterMapper2* pMap = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FilterMapper, NULL, CLSCTX_INPROC_SERVER, IID_IFilterMapper2, (void**)&pMap);
	if(FAILED(hr))
		return hr;

	// Enumerate all registered filters which have both input and outputs
	// pins taking MEDIATYPE_Audio.
	GUID arrayInTypes[2];
	arrayInTypes[0] = MEDIATYPE_Audio;
	arrayInTypes[1] = MEDIASUBTYPE_NULL;
	IEnumMoniker* pEnum = NULL;
	hr = pMap->EnumMatchingFilters(
	&pEnum,
	0, // Reserved.
	FALSE, // Use exact match?
	0, // merit
	TRUE, // At least one input pin?
	1, // Number of major type/subtype pairs for input.
	arrayInTypes, // Array of major type/subtype pairs for input.
	NULL, // Input medium.
	NULL, // Input pin category.
	FALSE, // Must be a renderer?
	TRUE, // At least one output pin?
	0, // Number of major type/subtype pairs for output.
	NULL, // Array of major type/subtype pairs for output.
	NULL, // Output medium.
	NULL); // Output pin category.
	if(FAILED(hr)) {
		pMap->Release();
		return hr;
	}

	IMoniker* apMoniker[1024];
	ULONG cMoniker = 0;
	hr = pEnum->Next(1024, apMoniker, &cMoniker);
	pEnum->Release();
	pMap->Release();
	if(FAILED(hr))
		return hr;

	// Allocate REGFILTER array for CLSIDs and names
	m_aRegFilter = new REGFILTER[cMoniker];
	if(NULL == m_aRegFilter)
		return E_OUTOFMEMORY;
	memset(m_aRegFilter, 0, sizeof(REGFILTER) * cMoniker);

	// Retrieve the moniker properties from the property bag
	m_cRegFilter = 0;
	for(ULONG ixMoniker = 0; ixMoniker < cMoniker; ixMoniker++) {
		REGFILTER* prf = m_aRegFilter + m_cRegFilter;

		char szCLSID[128];
		char szKeyBuf[1024];
		BOOL bIsDXi = FALSE;

		// Get the property bag for this moniker
		IPropertyBag* pPropBag;
		hr = apMoniker[ixMoniker]->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
		if(FAILED(hr))
			continue;

		// CLSID
		VARIANT var;
		var.vt = VT_BSTR;
		hr = pPropBag->Read(L"CLSID", &var, 0);
		if(FAILED(hr)) {
			pPropBag->Release();
			continue;
		}
		wcstombs(szCLSID, var.bstrVal, sizeof(szCLSID));
		hr = CLSIDFromString(var.bstrVal, &prf->Clsid);
		VariantClear(&var);
		if(FAILED(hr) || prf->Clsid == CLSID_KMixer) {
			pPropBag->Release();
			continue;
		}

		// Check if this is a DXi.  Construct registry key name with the plugin's CLSID
		strcpy(szKeyBuf, SZ_MFX_SYNTH_REGKEY);
		strcat(szKeyBuf, "\\");
		strcat(szKeyBuf, szCLSID);

		// Attempt to open the synth key
		HKEY hKey = NULL;
		if(ERROR_SUCCESS == RegOpenKeyA(HKEY_CLASSES_ROOT, szKeyBuf, &hKey)) {
			if(hKey)
				RegCloseKey(hKey);
			bIsDXi = TRUE;
		}

		// Not a DXi?  Skip it.
		if(!bIsDXi) {
			pPropBag->Release();
			continue;
		}

		// Friendly name
		var.vt = VT_BSTR;
		hr = pPropBag->Read(L"FriendlyName", &var, 0);
		if(FAILED(hr)) {
			pPropBag->Release();
			continue;
		}
		prf->Name = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * (wcslen(var.bstrVal) + 1));
		if(NULL == prf->Name) {
			pPropBag->Release();
			continue;
		}
		wcscpy(prf->Name, var.bstrVal);
		VariantClear(&var);

		m_cRegFilter++;
		pPropBag->Release();
	}

	// Free moniker array
	for(ULONG ixMoniker = 0; ixMoniker < cMoniker; ixMoniker++)
		apMoniker[ixMoniker]->Release();

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPlugInInventory::GetInfo(ULONG ix, CLSID* pClsid, pfc::string_base& out) const {
	if(ix >= m_cRegFilter)
		return E_INVALIDARG;
	if(NULL == pClsid)
		return E_POINTER;

	*pClsid = m_aRegFilter[ix].Clsid;
	out = pfc::stringcvt::string_utf8_from_wide(m_aRegFilter[ix].Name);
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPlugInInventory::CreatePlugIn(ULONG ix, IBaseFilter** ppFilter) const {
	if(ix >= m_cRegFilter)
		return E_INVALIDARG;
	if(NULL == ppFilter)
		return E_POINTER;

	const CLSID& clsid = m_aRegFilter[ix].Clsid;

	CHECK(CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)ppFilter));

	return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

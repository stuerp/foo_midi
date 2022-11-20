// PlugInInventory.h: interface for the CPlugInInventory class.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PLUGININVENTORY_H__166175C6_DB5D_456D_B228_685DD738B506__INCLUDED_)
#define AFX_PLUGININVENTORY_H__166175C6_DB5D_456D_B228_685DD738B506__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../../pfc/pfc.h"

class IBaseFilter;
struct REGFILTER;

class CPlugInInventory {
	public:
	CPlugInInventory();
	virtual ~CPlugInInventory();

	HRESULT EnumPlugIns();
	ULONG GetCount() const {
		return m_cRegFilter;
	}
	HRESULT GetInfo(ULONG ix, CLSID* pClsid, pfc::string_base& out) const;

	HRESULT CreatePlugIn(ULONG ix, IBaseFilter** ppFilter) const;

	private:
	REGFILTER* m_aRegFilter;
	ULONG m_cRegFilter;
};

#endif // !defined(AFX_PLUGININVENTORY_H__166175C6_DB5D_456D_B228_685DD738B506__INCLUDED_)

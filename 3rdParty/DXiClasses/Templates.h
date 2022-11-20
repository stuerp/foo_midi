// Templates.h: helpful C++ templates for searching sorted containers.
//
// Copyright (c) 2002 Twelve Tone Systems, Inc.  All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#ifndef _TEMPLATES_H_
#define _TEMPLATES_H_
#pragma once

////////////////////////////////////////////////////////////////////////////////

template<class T> class TimeCompareEQ
{
public:
	TimeCompareEQ( LONG t ) : m_t(t) {}
	int operator()( const T& item ) const { return item.GetTime() == m_t; }
private:
	LONG m_t;
};

template<class T> class TimeCompare
{
public:
	int operator()( const T& a, const T& b ) const { return a.GetTime() < b.GetTime(); }
};

////////////////////////////////////////////////////////////////////////////////

template<class T, class VALTYPE, class VALGET>
int bsearch_for_value( const T& obj, VALTYPE val, const VALGET getVal )
{
	int const nLen = obj.size();
	if (val >= getVal( obj[ nLen - 1 ] ))
		return nLen - 1;

	int low	= 0;
	int high	= nLen - 1;
	int mid	= (low + high) / 2;
	do
	{
		if (val < getVal( obj[ mid ] ))
			high = mid;
		else if (val > getVal( obj[ mid ] ))
			low = mid;
		else
			break;

		mid = (low + high) / 2;
		
	} while (low != mid && high != mid);

	while (low < nLen - 1 && getVal( obj[ low + 1 ] ) <= val)
		++low;

	return low;
}

////////////////////////////////////////////////////////////////////////////////

template<class T>
class GetTime
{
public:
	LONG operator()( const T& t ) const { return t.GetTime(); }
};

////////////////////////////////////////////////////////////////////////////////

#endif //_TEMPLATES_H_
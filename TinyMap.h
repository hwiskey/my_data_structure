/*
@file		TinyMap.h
@author		huangwei
@param		Email: huang-wei@corp.netease.com
@param		Copyright (c) 2004-2013  ���������׵繤����
@date		2013/5/7
@brief		
*/



#pragma once

#ifndef __TINYMAP_H__
#define __TINYMAP_H__

#include <vector>
#include <algorithm>

/**
 * TinyMap
 *
 * ʹ��˳������ʵ��map����
 * �������滻С��������map�����߸���Ƶ�ʵ͵Ľṹ������������ݣ�
 * �ŵ㣺�ڴ�С�������������
 * ȱ�㣺����ɾ��������
 */

template <typename K, typename T>
struct key_less
{
	typedef K									key_type;
	typedef T									mapped_type;
	typedef std::pair <K, T>					value_type;

	bool operator()(const value_type& _Left, const value_type& _Right) const
	{
		const key_type& _Leftkey = _Left.first;
		const key_type& _Rightkey = _Right.first;
		return (_Leftkey < _Rightkey);
	}
};

template < typename K, typename T, class Pr = key_less<K, T> >
class TinyMap
{
public:
	typedef K									key_type;
	typedef T									mapped_type;
	typedef std::pair <K, T>					value_type;
	typedef T									reference;
	typedef Pr									key_compare;
	typedef std::vector <value_type>			container_type;
	typedef typename container_type::size_type	size_type;
	typedef typename container_type::iterator	iterator;

	TinyMap() {}
	~TinyMap() {}

	bool empty() const
	{
		return storage.empty();
	}

	size_type size() const
	{
		return storage.size();
	}

	void clear()
	{
		storage.clear();
	}

	iterator erase(iterator _Where)
	{
		return storage.erase(_Where);
	}

	iterator begin()
	{
		return storage.begin();
	}

	iterator end()
	{
		return storage.end();
	}

	iterator find(const key_type& _Keyval)
	{
		value_type _Val(_Keyval, mapped_type());
		iterator _Where = std::lower_bound(storage.begin(), storage.end(), _Val, comp);
		if (_Where == storage.end() || comp(_Val, *_Where))
			return storage.end();
		return _Where;
	}

	iterator insert(const value_type& _Val, bool cover_old = true)
	{
		if (storage.empty() || comp(storage.back(), _Val)) // back() < _Val
			return storage.insert(storage.end(), _Val);
		else if (!storage.empty() && comp(_Val, storage.front())) // _Val < front()
			return storage.insert(storage.begin(), _Val);

		// lower_bound find >= _Val
		iterator _Where = std::lower_bound(storage.begin(), storage.end(), _Val, comp);
		if (_Where == storage.end() || comp(_Val, *_Where))
			_Where = storage.insert(_Where, _Val);
		else if (cover_old)
			*_Where = _Val;
		return _Where;
	}

	mapped_type& operator[](const key_type& _Keyval)
	{
		iterator _Where = insert(value_type(_Keyval, mapped_type()), false);
		return ((*_Where).second);
	}

protected:
	container_type	storage;
	key_compare		comp;
};

#endif // __TINYMAP_H__
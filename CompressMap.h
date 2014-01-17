/*
@file		CompressMap.h
@author		huangwei
@param		Copyright (c) 2004-2013  ���������׵繤����
@date		2013/4/28
@brief		
*/

#pragma once

#ifndef __COMPRESSMAP_H__
#define __COMPRESSMAP_H__

#include <map>
#include <vector>
#include <algorithm>

/**
 * CompressStorage
 *
 * ������ѹ���洢
 * ���ڻ���֧�ֻ��ջ���
 * ���̰߳�ȫ
 */
class CompressStorage
{
public:
	struct Pointer
	{
		int		key;
		int		idx;
		int		off;
		int		len;
	};

protected:
	struct _Block
	{
		int		idx;		// blocks��λ��
		int		w_off;		// bufд��ƫ��
		int		old_len;	// δѹ������
		int		compress_len;	// ѹ������
		int		cbuf_len;	// cbuf����
		char*	cbuf;		// ����(ѹ��)
		int		buf_len;	// buf����
		char*	buf;		// ����(��ѹ��)
	};

	typedef std::vector <_Block>		_BlockVec;
	typedef std::map <int, Pointer>		_PointMap;

public:
	CompressStorage(int size = 0);
	~CompressStorage();

	int			Insert(char* src, int src_len);
	void		Remove(int key);
	void		Clear();
	Pointer		Query(int key);
	void		Compress();
	char*		GetData(int key);
	char*		GetData(Pointer pos);

	int			QueryBytes();
	int			QueryCBytes();

protected:
	void		_BlockInit(_Block* block);
	void		_BlockNew(_Block* block, int size);
	void		_BlockNewBuf(_Block* block, int size);
	void		_BlockNewCBuf(_Block* block, int size);
	void		_BlockClear(_Block* block);
	void		_BlockClearBuf(_Block* block);
	void		_BlockClearCBuf(_Block* block);
	bool		_BlockIsSufficient(_Block* block, int len);
	bool		_BlockCopy(_Block* block, char* dst, int off, int len);
	bool		_BlockMove(_Block* block, _Block* block_src, Pointer* ptr);
	bool		_BlockWrite(_Block* block, Pointer* ptr, char* src, int src_len, int w_off = -1);
	bool		_BlockCompress(_Block* block);
	bool		_BlockUnCompress(_Block* block);

protected:
	int			default_bufsize;
	_BlockVec	blocks;
	_PointMap	keymap;
};

/**
 * CompressLazyMap
 *
 * ��ѹ��������map
 * ֧������insert�󣬽���sort��Ȼ��find
 */
template <typename K, typename T>
class CompressLazyMap
{
public:
	template <typename K, typename T>
	struct pair_struct
	{
		K							first;	// key
		int							storage_key;

		pair_struct(const K& k)
			: first(k), storage_key(0) {}

		bool operator< (const pair_struct& right) const 
		{
			if (first == right.first)
				return storage_key < right.storage_key;
			return first < right.first;
		}
	};

	typedef size_t								size_type;
	typedef K									key_type;
	typedef T									mapped_type;
	typedef pair_struct <K, T>					value_type;
	typedef std::vector <value_type>			container_type;
	typedef typename container_type::iterator	iterator;
	
public:
	CompressLazyMap(int size = 0)
		: storage(size)
	{
		ordered = true;
		fakeptr = (mapped_type*)malloc(sizeof(mapped_type));
	}

	~CompressLazyMap() 
	{
		clear();

		free(fakeptr);
		fakeptr = NULL;
	}

	void sort()
	{	// lazy sort all
		if (ordered || nodes.empty())
			return;

		container_type new_nodes;
		std::sort(nodes.begin(), nodes.end()); // ordered
		container_type::iterator it_pre = nodes.begin();
		for (container_type::iterator it = it_pre + 1; it != nodes.end(); ++ it)
		{
			if ((*it_pre).first != (*it).first)
				new_nodes.push_back(*it_pre);
			else
			{
				destroy(get(it_pre)); // important!!!
				storage.Remove((*it_pre).storage_key);
			}
			it_pre = it;
		}
		new_nodes.push_back(*it_pre);

		std::swap(new_nodes, nodes);
		storage.Compress();
		ordered = true;
	}

	size_type size() const
	{	// return length of sequence
		return nodes.size();
	}

	bool empty() const
	{	// return true only if sequence is empty
		return (size() == 0);
	}

	iterator begin()
	{	// return iterator for beginning of mutable sequence
		return nodes.begin();
	}

	iterator end()
	{	// return iterator for end of mutable sequence
		return nodes.end();
	}

	void clear()
	{	// destroy object and clear
		for (container_type::iterator it = nodes.begin(); it != nodes.end(); ++ it)
			destroy(get(it)); // important!!!
		nodes.clear();
		storage.Clear();
	}

	void del(const key_type& _Keyval)
	{	// delete key and value
		if (ordered)
			erase(find(_Keyval));
		else
		{
		}
	}

	void set(const key_type& _Keyval, const mapped_type& _Mapval)
	{	// find element matching _Keyval or insert with default mapped
		if (!ordered)
		{
			insert(_Keyval, _Mapval);
			return;
		}

		iterator _Where = std::lower_bound(nodes.begin(), nodes.end(), value_type(_Keyval));
		if (_Where == end() || !(_Keyval == (*_Where).first))
		{
			_Where = insert(_Keyval, _Mapval); // unordered
			ordered = false;
		}
		else
		{
			*get(_Where) = _Mapval;
		}
	}

	mapped_type* get(iterator _Where)
	{	// convert storage key to object
		if (_Where == end())
			return NULL;

		value_type& val = *_Where;
		char* ptr = storage.GetData(val.storage_key);
		return (mapped_type*)ptr;
	}

	iterator find(const key_type& _Keyval)
	{	// find an element in mutable sequence that matches _Keyval
		if (!ordered)
			return end();

		iterator _Where = std::lower_bound(nodes.begin(), nodes.end(), value_type(_Keyval));
		return ((_Where == end() || !(_Keyval == (*_Where).first)) ? end() : _Where);
	}

protected:
	iterator insert(const key_type& _Keyval, const mapped_type& _Mapval)
	{	// insert a {key, mapped} value, with hint
		value_type _Val(_Keyval);
		construct(fakeptr, _Mapval); // important!!! construct object with share-memory
		_Val.storage_key = storage.Insert((char*)fakeptr, sizeof(mapped_type));
		return nodes.insert(end(), _Val);
	}

	void erase(iterator _Where)
	{	// erase element at _Where
		if (_Where == end())
			return;

		value_type& _Val = *_Where;
		destroy(get(_Where)); // important!!! destroy object
		storage.Remove(_Val.storage_key);
		nodes.erase(_Where);
	}

	void construct(mapped_type* p, const mapped_type &t)
	{	// construct object at _Ptr with value _Val
		::new ((void *)p) mapped_type(t); 
	}

	void destroy(mapped_type* p) 
	{	// destroy object at _Ptr
		(void)(p);
		p->~T();
	}

protected:
	container_type	nodes;
	bool			ordered;
	CompressStorage	storage;
	mapped_type*	fakeptr;
};

#endif // __COMPRESSMAP_H__

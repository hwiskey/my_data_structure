/*
@file		MultiIndexMap.h
@author		huangwei
@param		Email: huang-wei@corp.netease.com
@param		Copyright (c) 2004-2013  杭州网易雷电工作室
@date		2013/5/6
@brief		
*/

#pragma once

#ifndef __MULTIINDEXMAP_H__
#define __MULTIINDEXMAP_H__

#include <set>
#include <map>
#include <vector>
#include <cassert>
#include <algorithm>


/**
 * MultiIndexMMap
 *
 * 支持多索引的set，内部维护同步索引（节点更新用update函数）
 * 支持equal_range范围查找
 * 多索引有内存开销，但比多map节省
 * 实际数据存放在顺序容器
 *
 * TODO:
 *	storage中去重
 */
template <typename T>
struct i_multi_key_comp
{
	virtual bool operator () (const T* lp, const T* rp) = 0;
};

template <typename T>
class MultiIndexMMap
{
public:
	struct value_compare;
	struct index_pair;
	struct index_value_pair;
	struct index_value_iterator;

	typedef size_t														size_type;
	typedef T															key_type;
	typedef T															value_type;
	typedef T*															pointer;
	typedef T&															reference;
	typedef i_multi_key_comp<T>											comp_type;
	typedef comp_type*													comp_pointer;
	typedef std::list <value_type>										container_type;
	typedef std::multiset <index_value_pair, value_compare>				index_type;
	typedef std::list <index_pair>										index_container_type;
	typedef std::pair<index_value_iterator, index_value_iterator>		index_value_it_pair;
	typedef typename index_container_type::iterator						index_iterator;
	typedef typename container_type::iterator							iterator;

	struct index_pair
	{
		index_type		index;
		comp_pointer	comp;

		index_pair(comp_pointer c = NULL) : comp(c) {}
	};
	struct index_value_pair
	{
		pointer			val;
		comp_pointer	comp;

		index_value_pair(pointer v, comp_pointer c) : val(v), comp(c) {}
	};
	struct value_compare
	{
		bool operator () (const index_value_pair& ls, const index_value_pair& rs) const
		{
			return (*ls.comp)(ls.val, rs.val);
		}
	};
	struct index_value_iterator : public index_type::iterator
	{
		typedef typename index_type::iterator			base_iterator;
		typedef typename MultiIndexMMap::pointer		pointer;
		typedef typename MultiIndexMMap::comp_pointer	comp_pointer;
		typedef typename MultiIndexMMap::reference		reference;
		typedef typename MultiIndexMMap::index_iterator	index_iterator;

		index_iterator	index_it;

		index_value_iterator(const base_iterator& it, const index_iterator& it2)
			: base_iterator(it), index_it(it2)
		{}

		pointer get_value() const
		{
			return (base_iterator::operator*()).val;
		}

		reference operator*() const
		{
			return *get_value();
		}

		pointer operator->() const
		{
			return get_value();
		}

		bool operator==(const index_value_iterator& _Right) const
		{
			if (index_it != _Right.index_it)
				assert(false);
			return base_iterator::operator==(_Right);
		}

		bool operator!=(const index_value_iterator& _Right) const
		{
			if (index_it != _Right.index_it)
				assert(false);
			return base_iterator::operator!=(_Right);
		}
	};


public:
	MultiIndexMMap() {}
	~MultiIndexMMap()
	{
		clear();
	}

	bool empty()
	{
		return storage.empty();
	}

	size_type size()
	{
		return storage.size();
	}

	void clear()
	{
		for (index_iterator it = index.begin(); it != index.end(); ++ it)
		{
			comp_pointer comp = it->comp;
			delete comp;
		}
		index.clear();
		storage.clear();
	}

	void erase(iterator _Where)
	{
		for (index_iterator it = index.begin(); it != index.end(); ++ it)
		{
			index_type& index = it->index;
			index.erase(index_value_pair(&(*_Where), it->comp));
		}
		storage.erase(_Where);
	}

	void erase(pointer _Pval)
	{
		for (index_iterator it = index.begin(); it != index.end(); ++ it)
		{
			index_type& index = it->index;
			index.erase(index_value_pair(_Pval, it->comp));
		}
		// O(N) !!!
		// use map <pointer, iterator> can reduce time cost
		for (iterator it = storage.begin(); it != storage.end(); ++ it)
		{
			if (_Pval != &(*it))
				continue;
			storage.erase(it);
			break;
		}
	}

	pointer find(const key_type& _Keyval, const index_iterator& _Index)
	{
		index_type& key_index = (*_Index).index;
		typename index_type::iterator it = key_index.find(index_value_pair((pointer)&_Keyval, (*_Index).comp));
		if (it == key_index.end())
			return NULL;
		return (*it).val;
	}

	index_iterator insert_index(const comp_pointer _Keycomp)
	{
		if (_Keycomp == NULL)
			return index.end();

		index_iterator ret = index.insert(index.end(), index_pair(_Keycomp));
		index_type& key_index = ret->index;
		for (typename container_type::iterator it = storage.begin(); it != storage.end(); ++ it)
			key_index.insert(index_value_pair(&(*it), _Keycomp));
		return ret;
	}

	void insert(const value_type& _Val)
	{
		storage.push_back(_Val);
		pointer ptr = &storage.back();
		for (index_iterator it = index.begin(); it != index.end(); ++ it)
		{
			index_type& index = it->index;
			index.insert(index_value_pair(ptr, it->comp));
		}
	}

	void update(pointer _Poldval, const value_type& _Val)
	{
		erase(_Poldval);
		insert(_Val);
	}

	iterator begin()
	{
		return storage.begin();
	}

	iterator end()
	{
		return storage.end();
	}

	index_value_iterator begin(const index_iterator& _Index)
	{
		index_type& key_index = (*_Index).index;
		return index_value_iterator(key_index.begin(), _Index);
	}

	index_value_iterator end(const index_iterator& _Index)
	{
		index_type& key_index = (*_Index).index;
		return index_value_iterator(key_index.end(), _Index);
	}

	index_value_iterator lower_bound(const key_type& _Keyval, const index_iterator& _Index)
	{
		index_type& key_index = (*_Index).index;
		return index_value_iterator(key_index.lower_bound(index_value_pair((pointer)&_Keyval, (*_Index).comp)), _Index);
	}

	index_value_iterator upper_bound(const key_type& _Keyval, const index_iterator& _Index)
	{
		index_type& key_index = (*_Index).index;
		return index_value_iterator(key_index.upper_bound(index_value_pair((pointer)&_Keyval, (*_Index).comp)), _Index);
	}

	index_value_it_pair equal_range(const key_type& _Keyval, const index_iterator& _Index)
	{
		return index_value_it_pair(lower_bound(_Keyval, _Index), upper_bound(_Keyval, _Index));
	}

	index_value_it_pair equal_range(const key_type& _KeyvalL, const key_type& _KeyvalR, const index_iterator& _Index)
	{
		const key_type* pkey_l = &_KeyvalL;
		const key_type* pkey_r = &_KeyvalR;
		if (!(*_Index->comp)(pkey_l, pkey_r))
			std::swap(pkey_l, pkey_r);
		return index_value_it_pair(lower_bound(*pkey_l, _Index), upper_bound(*pkey_r, _Index));
	}

	// no {key/value}, only value
// 	value_type& operator[](const key_type& _Keyval)
// 	{
// 		if (index.empty())
// 		{
// 			static value_type null_value;
// 			return null_value;
// 		}
// 
// 		index_type& first_index = index.front().index;
// 		typename index_type::iterator _Where = first_index.lower_bound(index_value_pair((pointer)&_Keyval,index.front().comp));
// 		if (_Where == first_index.end() || 
// 			(*index.front().comp)((pointer)&_Keyval, (*_Where).val))
// 		{
// 			insert(_Keyval);
// 			_Where = first_index.lower_bound(index_value_pair((pointer)&_Keyval,index.front().comp));
// 		}
// 		return *(*_Where).val;
// 	}

protected:
	container_type			storage;
	index_container_type	index;
};


#endif // __MULTIINDEXMAP_H__

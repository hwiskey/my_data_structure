/*
@file		STLAllocator.h
@author		huangwei
@param		Email: huang-wei@corp.netease.com
@param		Copyright (c) 2004-2013  杭州网易雷电工作室
@date		2013/4/28
@brief		
*/

#pragma once

#ifndef __STLALLOCATOR_H__
#define __STLALLOCATOR_H__


#include <string>
#include <xmemory>

/**
 * TAllocatorCounter
 *
 */
template <typename C>
struct TAllocatorCounter {
	typedef size_t		size_type;

	static size_type get_mem_size() {
		return mem_total;
	}

	static size_type get_class_size() {
		return class_total;
	}

	static size_type	mem_total;
	static size_type	class_total;
};


/**
 * TAllocator
 *
 * allocator 模板
 * T参数参考stl默认配置
 * C参数表示容器类型
 * 只用作内存统计
 */
template <typename T, typename C>
struct TAllocator {
	typedef size_t	size_type, difference_type;
	typedef T		value_type;
	typedef T		*pointer, &reference;
	typedef const T	*const_pointer, &const_reference;

	template <typename U>
	struct rebind { typedef TAllocator <U, C> other; };

	TAllocator() {
		++ TAllocatorCounter<C>::class_total;
	}

	TAllocator(const TAllocator <T, C> &) {
		++ TAllocatorCounter<C>::class_total;
	}

	template<class U>
	TAllocator(const TAllocator<U, C>&) {
		++ TAllocatorCounter<C>::class_total;
	}

	~TAllocator() {
		-- TAllocatorCounter<C>::class_total;
	}

	pointer address(reference r) const { return &r; }
	const_pointer address(const_reference r) const { return &r; }
	size_type max_size() const { // estimate maximum array size
		size_type _Count = (size_type)(-1) / sizeof (T);
		return (0 < _Count ? _Count : 1);
	}
	void construct(pointer p, const T &t) { // construct object at _Ptr with value _Val
		::new ((void *)p) T(t); 
	}
	void destroy(pointer p) { // destroy object at _Ptr
		(void)(p);
		p->~T();
	}
	pointer allocate(size_type n) { // allocate array of _Count elements
		TAllocatorCounter<C>::mem_total += n * sizeof(value_type);
		return (pointer)::operator new(n * sizeof(value_type)); 
	}
	pointer allocate(size_type n, const void *) { // allocate array of _Count elements, ignore hint
		return (allocate(n));
	}
	void deallocate(pointer p, size_type n) { // deallocate object at _Ptr, ignore size
		TAllocatorCounter<C>::mem_total -= n * sizeof(value_type);
		::operator delete((void *)p);
	}
};

//////////////////////////////////////////////////////////////////////////
#define StringAllocator					TAllocator<char, std::string>
#define StringAllocatorCounter			TAllocatorCounter<std::string>

#define SetAllocator(T)					TAllocator<T, std::set<T>>
#define SetAllocatorCounter(T)			TAllocatorCounter<std::set<T>>

#define MapAllocator(KT, T)				TAllocator<std::pair<const KT, T>, std::map<KT, T>>
#define MapAllocatorCounter(KT, T)		TAllocatorCounter<std::map<KT, T>>

class managed_string : public std::basic_string<char, std::char_traits<char>, StringAllocator>
{
public:
	typedef	StringAllocatorCounter	counter;

	managed_string() {
	}

	managed_string(const std::string& right) {
		this->assign(right.c_str(), right.length());
	}

	managed_string& operator= (const std::string& right) {
		this->assign(right.c_str(), right.length());
		return *this;
	}

	operator std::string() const {
		return c_str();
	}
};

//////////////////////////////////////////////////////////////////////////
template <typename C>
typename TAllocatorCounter<C>::size_type TAllocatorCounter<C>::class_total = 0;
template <typename C>
typename TAllocatorCounter<C>::size_type TAllocatorCounter<C>::mem_total = 0;


#endif // __STLALLOCATOR_H__
/***************************************************************************

    emualloc.h

    Memory allocation helpers for the core emulator.

****************************************************************************

    Copyright Aaron Giles
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in
          the documentation and/or other materials provided with the
          distribution.
        * Neither the name 'MAME' nor the names of its contributors may be
          used to endorse or promote products derived from this software
          without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#pragma once

#ifndef __EMUALLOC_H__
#define __EMUALLOC_H__

#include <new>
#include "osdcore.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

// zeromem_t is a dummy class used to tell new to zero memory after allocation
class zeromem_t { };


// resource_pool_item is a base class for items that are tracked by a resource pool
class resource_pool_item
{
private:
	resource_pool_item(const resource_pool_item &);
	resource_pool_item &operator=(const resource_pool_item &);
	
public:
	resource_pool_item(void *_ptr, size_t _size)
		: next(NULL),
		  ptr(_ptr),
		  size(_size) { }
	virtual ~resource_pool_item() { }
	
	resource_pool_item *	next;
	void *					ptr;
	size_t					size;
};
 

// a resource_pool_object is a simple object wrapper for the templatized type
template<class T> class resource_pool_object : public resource_pool_item
{
private:
	resource_pool_object<T>(const resource_pool_object<T> &);
	resource_pool_object<T> &operator=(const resource_pool_object<T> &);
	
public:
	resource_pool_object(T *_object) 
		: resource_pool_item(reinterpret_cast<void *>(_object), sizeof(T)),
		  object(_object) { }
	virtual ~resource_pool_object() { delete object; }
 
private:
	T *object;
};


// a resource_pool_array is a simple object wrapper for an allocated array of 
// the templatized type
template<class T> class resource_pool_array : public resource_pool_item
{
private:
	resource_pool_array<T>(const resource_pool_array<T> &);
	resource_pool_array<T> &operator=(const resource_pool_array<T> &);
	
public:
	resource_pool_array(T *_array, int _count) 
		: resource_pool_item(reinterpret_cast<void *>(_array), sizeof(T) * _count),
		  array(_array),
		  count(_count) { }
	virtual ~resource_pool_array() { delete[] array; }

private:
	T *array;
	int count;
};


// a resource pool tracks items and frees them upon reset or destruction
class resource_pool
{
private:
	resource_pool(const resource_pool &);
	resource_pool &operator=(const resource_pool &);
	
public:
	resource_pool();
	~resource_pool();
	
	void add(resource_pool_item &item);
	void remove(resource_pool_item &item) { remove(item.ptr); }
	void remove(void *ptr);
	void remove(const void *ptr) { remove(const_cast<void *>(ptr)); }
	resource_pool_item *find(void *ptr);
	bool contains(void *ptrstart, void *ptrend);
	void clear();
 
	template<class T> T *add_object(T* object) { add(*new(__FILE__, __LINE__) resource_pool_object<T>(object)); return object; }
	template<class T> T *add_array(T* array, int count) { add(*new(__FILE__, __LINE__) resource_pool_array<T>(array, count)); return array; }

private:
	static const int		hash_prime = 193;
	
	resource_pool_item *	hash[hash_prime];
	osd_lock *				listlock;
};



/***************************************************************************
    MACROS
***************************************************************************/

// re-route classic malloc-style allocations
#undef malloc
#undef calloc
#undef realloc
#undef free

#define malloc(x)		__error_use_auto_alloc_or_global_alloc_instead__
#define calloc(x,y)		__error_use_auto_alloc_clear_or_global_alloc_clear_instead__
#define realloc(x,y)	__error_realloc_is_dangerous__
#define free(x)			__error_use_auto_free_or_global_free_instead__

// pool allocation helpers
#define pool_alloc(_pool, _type)					(_pool).add_object(new(__FILE__, __LINE__) _type)
#define pool_alloc_clear(_pool, _type)				(_pool).add_object(new(__FILE__, __LINE__, zeromem) _type)
#define pool_alloc_array(_pool, _type, _num)		(_pool).add_array(new(__FILE__, __LINE__) _type[_num], (_num))
#define pool_alloc_array_clear(_pool, _type, _num) 	(_pool).add_array(new(__FILE__, __LINE__, zeromem) _type[_num], (_num))
#define pool_free(_pool, v)							(_pool).remove(v)

// global allocation helpers
#define global_alloc(_type)							pool_alloc(global_resource_pool, _type)
#define global_alloc_clear(_type)					pool_alloc_clear(global_resource_pool, _type)
#define global_alloc_array(_type, _num)				pool_alloc_array(global_resource_pool, _type, _num)
#define global_alloc_array_clear(_type, _num)		pool_alloc_array_clear(global_resource_pool, _type, _num)
#define global_free(v)								pool_free(global_resource_pool, v)



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

// global resource pool
extern resource_pool global_resource_pool;

// dummy objects to pass to the specialized new variants
extern const zeromem_t zeromem;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

// allocate memory with file and line number information
void *malloc_file_line(size_t size, const char *file, int line);

// free memory with file and line number information
void free_file_line(void *memory, const char *file, int line);

// called from the exit path of any code that wants to check for unfreed  memory
void dump_unfreed_mem();

// standard new/delete operators (try to avoid using)
void *operator new(std::size_t size) throw (std::bad_alloc);
void *operator new[](std::size_t size) throw (std::bad_alloc);
void operator delete(void *ptr);
void operator delete[](void *ptr);

// file/line new/delete operators
void *operator new(std::size_t size, const char *file, int line) throw (std::bad_alloc);
void *operator new[](std::size_t size, const char *file, int line) throw (std::bad_alloc);
void operator delete(void *ptr, const char *file, int line);
void operator delete[](void *ptr, const char *file, int line);

// file/line new/delete operators with zeroing
void *operator new(std::size_t size, const char *file, int line, const zeromem_t &) throw (std::bad_alloc);
void *operator new[](std::size_t size, const char *file, int line, const zeromem_t &) throw (std::bad_alloc);
void operator delete(void *ptr, const char *file, int line, const zeromem_t &);
void operator delete[](void *ptr, const char *file, int line, const zeromem_t &);


#endif	/* __EMUALLOC_H__ */

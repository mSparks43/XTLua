//
//  module.cpp
//  xlua
//
//  Created by Ben Supnik on 3/19/16.
//
// xTLua
// Modified by Mark Parker on 04/19/2020
//
//	Copyright 2016, Laminar Research
//	This source code is licensed under the MIT open source license.
//	See LICENSE.txt for the full terms of the license.

#include "module.h"
//#include <XPLMUtilities.h>
#include "xpfuncs.h"
#include <stdlib.h>
#include <assert.h>
#include "lua_helpers.h"
#include <chrono>
#include <math.h>
static const char * shorten_to_file(const char * path)
{
	const char * p = path, * t = path;
	while(*p)
	{
		if(*p == '/' || *p == '\\')
			t = p+1;
		++p;
	}
	return t;
}

static long length_of_dir(const char * p)
{
	const char * f = shorten_to_file(p);
	return f - p;
}

#if !MOBILE

class	xmap_class {
public:
	xmap_class(const string& in_file_name);
	~xmap_class() { free(m_buffer); }
	bool exists() const { return m_buffer != NULL; }
	const char * begin() const { return m_buffer; }
	size_t size() const { return m_size; }
private:
	char *		 m_buffer;
	size_t		 m_size;
};

#endif

#define MALLOC_CHUNK_SIZE 4096

struct module_alloc_block {
	module_alloc_block *		next;
	char *					ptr;
	size_t					remaining;
	char						data[MALLOC_CHUNK_SIZE];
};

static module_alloc_block * make_alloc_block()
{
	module_alloc_block * r = (module_alloc_block *) malloc(MALLOC_CHUNK_SIZE);
	assert(r);
	r->next = NULL;
	r->ptr = r->data;
	r->remaining = MALLOC_CHUNK_SIZE;
	return r;
}

static void * alloc_from_block(module_alloc_block *& head, size_t amt)
{
	assert(amt <= MALLOC_CHUNK_SIZE);
	
	if(head != NULL && head->remaining >= amt)
	{
		void * r = head->ptr;
		head->ptr += amt;
		head->remaining -= amt;
		return r;
	}
	else
	{
		module_alloc_block * nb = make_alloc_block();
		nb->next = head;
		head = nb;
	}	

	void * r = head->ptr;
	head->ptr += amt;
	head->remaining -= amt;
	return r;
}

static void destroy_alloc_block(module_alloc_block * head)
{
	while(head)
	{
		module_alloc_block * k = head;
		head = head->next;
		free(k);
	}
}

#define CTOR_FAIL(errcode,msg) \
if(errcode != 0) { \
	const char * s = lua_tostring(m_interp, -1); \
	printf("%s\n%s failed: %d\n",s,msg,errcode); \
	lua_close(m_interp); \
	m_interp = NULL; \
	return; }

module::module(
							const char *		in_module_path,
							const char *		in_init_script,
							const char *		in_module_script,bool isThread) :
	m_interp(NULL),
	m_memory(NULL),
	m_path(in_module_path)
{
	long boiler_plate_paths = length_of_dir(in_init_script);
	if(isThread)
		printf("Running %s\n", in_module_script+boiler_plate_paths);
	else	
		printf("Init init/%s\n", in_module_script+boiler_plate_paths);

	m_interp = luaL_newstate();


	if(m_interp == NULL)
	{
		printf("Unable to set up Lua.");
		return;
	}
	luaL_openlibs(m_interp);

	lua_pushlightuserdata(m_interp, this);
	lua_setglobal(m_interp, "__module_ptr");		
		
	add_xpfuncs_to_interp(m_interp,isThread);
	
	// Mobile devices like Android don't use a regular file system...they have a bundle of resources in-memory so
	// we need to load the Lua script from an already allocated memory buffer.
	xmap_class linit(in_init_script);
	if(!linit.exists())
		CTOR_FAIL(-1, "load init script");
	
	m_debug_proc = lua_pushtraceback(m_interp);
	
	int load_result = luaL_loadbuffer(m_interp, (const char*)linit.begin(), linit.size(), in_init_script+boiler_plate_paths);
	CTOR_FAIL(load_result, "load init script")

	int init_script_result = lua_pcall(m_interp, 0, 0, m_debug_proc);
	CTOR_FAIL(init_script_result, "run init script");

	lua_getfield(m_interp, LUA_GLOBALSINDEX, "run_module_in_namespace");
	
	// Mobile devices like Android don't use a regular file system...they have a bundle of resources in-memory so
	// we need to load the Lua script from an already allocated memory buffer.
	xmap_class lmod(in_module_script);
	if(!lmod.exists())
		CTOR_FAIL(-1, "load module");
	int module_load_result = luaL_loadbuffer(m_interp, (const char*)lmod.begin(), lmod.size(), in_module_script+boiler_plate_paths);
	CTOR_FAIL(module_load_result,"load module");
	
	int module_run_result = lua_pcall(m_interp, 1, 0, m_debug_proc);
	CTOR_FAIL(module_run_result,"run module");
}

int module::load_module_relative_path(const string& path)
{
	string rel_path(m_path);
	string script_path = rel_path + path;
	
	xmap_class	script_text(script_path);
	
	if(!script_text.exists())
	{
		return luaL_error(m_interp, "Unable to load script file: %s", path.c_str());
	}
	
	int load_result = luaL_loadbuffer(m_interp, (const char*)script_text.begin(), script_text.size(), path.c_str());
	
	return load_result;
}

int module::debug_proc_from_interp(lua_State * interp)
{
	module * me = module_from_interp(interp);
	if(me)
		return me->m_debug_proc;
	return 0;
}

module * module::module_from_interp(lua_State * interp)
{
	lua_getglobal(interp,"__module_ptr");
	
	if(lua_islightuserdata(interp, -1))
	{
		module * me = (module *) lua_touserdata(interp, -1);
		lua_pop(interp, 1);
		return me;
	} 
	else
	{
		assert(!"No defined module");
		lua_pop(interp, 1);
		return NULL;
	}
}

void *		module::module_alloc_tracked(size_t amount)
{
	return alloc_from_block(m_memory, amount);
}

void		module::acf_load()
{
	do_callout("aircraft_load");
}

void		module::acf_unload()
{
	do_callout("aircraft_unload");
}

void		module::flight_init()
{
	do_callout("flight_start");
}

void		module::flight_crash()
{
	do_callout("flight_crash");
}

int	module::pre_physics()
{
	return do_callout("before_physics");
}

int		module::post_physics()
{
	//auto start = std::chrono::high_resolution_clock::now();
	return do_callout("after_physics");
	/*auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = finish - start;
	int diff=round(elapsed.count());
	if(diff>=10){
		printf("warn: xtlua time overflow!=%d for %s\n",diff,m_path.c_str());
	}*/
}

void		module::post_replay()
{
	do_callout("after_replay");
}

int module::do_callout(const char * f)
{
	if(m_interp == NULL)
		return 0;
	lua_getfield(m_interp, LUA_GLOBALSINDEX, "do_callout");
	if(lua_isnil(m_interp, -1))
	{
		lua_pop(m_interp, 1);
		return 0;
	}
	else
	{	
		return fmt_pcall_stdvars(m_interp,m_debug_proc,"s",f);
	}
}

module::~module()
{
	if(m_interp)
		lua_close(m_interp);
	destroy_alloc_block(m_memory);
		
}

#if !MOBILE

#if IBM

#include <Windows.h>

//-----
//https://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte

// Convert a wide Unicode string to an UTF8 string
// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_decode(const std::string &str)
{
	if (str.empty()) return std::wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

//-----

#endif

xmap_class::xmap_class(const string& in_file_name) :
	m_buffer(NULL), m_size(0)
{
#if IBM
	FILE * fi = _wfopen(utf8_decode(in_file_name).c_str(), L"rb");
#else
	FILE * fi = fopen(in_file_name.c_str(), "rb");
#endif
	if(fi)
	{
		fseek(fi,0,SEEK_END);
		m_size = ftell(fi);
		fseek(fi,0,SEEK_SET);
		m_buffer = (char *) malloc(m_size);
		size_t bytes = fread(m_buffer,1,m_size,fi);
		(void) bytes;
		fclose(fi);
	}
}


#endif

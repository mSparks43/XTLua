//
//  xpfuncs.cpp
//  xlua
//
//  Created by Ben Supnik on 3/19/16.
//
//	Copyright 2016, Laminar Research
//	This source code is licensed under the MIT open source license.
//	See LICENSE.txt for the full terms of the license.
// xTLua
// Modified by Mark Parker on 04/19/2020

#include <cstdio>
#include <cstdlib>
#include "xpfuncs.h"
#include "lua_helpers.h"
#include "xpdatarefs.h"
#include "xpcommands.h"
#include "xptimers.h"
#include "module.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

/*
	TODO: figure out when we have to resync our datarefs
	TODO: what if dref already registered before acf reload?  (maybe no harm?)
	TODO: test x-plane-side string read/write - needs test not at startup

 */



// This is kind of a mess - Lua [annoyingly] doesn't give you a way to store a closure/Lua interpreter function
// in C space.  The hack is to use luaL_ref to fill a new key in the registry table with a copy of ANY value from
// the stack - since this is type agnostic and takes a strong reference it (1) prevents the closure from being 
// garbage collected and (2) works with closures.

struct notify_cb_t {
	lua_State *		L;
	int				slot;
};

// Given an interp and a stack arg that is a lua function/closure,
// this routine stashes a strong ref to the closure in the registry,
// allcoates a callback struct and stashes the slot and interp in the
// CB struct.  This CB struct is a single C ptr that we can use to 
// reconstruct the closure from C land.
//
// If the closure is actually nil, we return NULL and allocate nothing.
// The memory is tracked by the interp's module and is collected for us
// at shutdown.
notify_cb_t * wrap_lua_func(lua_State * L, int idx)
{
	if(lua_isnil(L, idx))
	{
		luaL_argerror(L, idx, "nil not allowed for callback");
		return NULL;
	}
	luaL_checktype(L, idx, LUA_TFUNCTION);
	
	module * me = module::module_from_interp(L);
	notify_cb_t * cb = (notify_cb_t *) me->module_alloc_tracked(sizeof(notify_cb_t));
	cb = new notify_cb_t;
	cb->L = L;
	lua_pushvalue (L, idx);
	cb->slot = luaL_ref(L, LUA_REGISTRYINDEX);		
	return cb;
}

notify_cb_t * wrap_lua_func_nil(lua_State * L, int idx)
{
	if(lua_isnil(L,idx))
	{
		return NULL;
	}
	return wrap_lua_func(L, idx);
}

// Given a void * that is really a CB struct, this routine either
// pushes the lua function onto the stack (so that we can then push 
// args and pcall) or returns 0 if we should not call because the CB is
// nil or borked.
lua_State * setup_lua_callback(void * ref)
{
	if(ref == NULL) 
		return NULL;
	notify_cb_t * cb = (notify_cb_t *) ref;
	lua_rawgeti (cb->L, LUA_REGISTRYINDEX, cb->slot);
	if(!lua_isfunction(cb->L, -1))
	{
		printf("ERROR: we did not persist a closure?!?");
		lua_pop(cb->L, 1);
		return 0;
	}
	return cb->L;
}

template <typename T>
T * luaL_checkuserdata(lua_State * L, int narg, const char * msg)
{
	T * ret = (T*) lua_touserdata(L, narg);
	if(ret == NULL)
		luaL_argerror(L, narg, msg);
	return ret;	
}

//----------------------------------------------------------------
// MISC
//----------------------------------------------------------------

static int XLuaGetCode(lua_State * L)
{
	module * me = module::module_from_interp(L);
	assert(me);
	
	const char * name = luaL_checkstring(L, 1);
	
	int result = me->load_module_relative_path(name);
	
	if(result)
	{
		const char * err_msg = luaL_checkstring(L,1);
		printf("%s: %s", name, err_msg);
	}
	
	return 1;
}


//----------------------------------------------------------------
// DATAREFS
//----------------------------------------------------------------

// XPLMFindDataRef "foo" -> dref
static int XLuaFindDataRef(lua_State * L)
{
	const char * name = luaL_checkstring(L, -1);

	xlua_dref * r = xlua_find_dref(name);
	assert(r);
	
	lua_pushlightuserdata(L, r);
	return 1;
}
static int XTLuaFindDataRef(lua_State * L)
{
	const char * name = luaL_checkstring(L, -1);

	xtlua_dref * r = xtlua_find_dref(name);
	assert(r);
	
	lua_pushlightuserdata(L, r);
	return 1;
}
static void xlua_notify_helper(xlua_dref * who, void * ref)
{
	lua_State * L = setup_lua_callback(ref);
	if(L)
	{
		fmt_pcall_stdvars(L,module::debug_proc_from_interp(L),"");
	}
}

// XPLMCreateDataRef name "array[4]" "yes" func -> dref
static int XLuaCreateDataRef(lua_State * L)
{
	const char * name = luaL_checkstring(L, 1);
	const char * typestr = luaL_checkstring(L,2);
	const char * writable = luaL_checkstring(L,3);	
	notify_cb_t * cb = wrap_lua_func_nil(L, 4);
	
	if(strlen(name) == 0)
		return luaL_argerror(L, 1, "dataref name must not be an empty string.");

	int my_writeable;
	if(strcmp(writable,"yes")==0)
		my_writeable = 1;
	else if (strcmp(writable,"no")==0)
		my_writeable = 0;
	else 
		return luaL_argerror(L, 3, "writable must be 'yes' or 'no'");
	
	xtlua_dref_type my_type = xlua_none;
	int my_dim = 1;
	const char * c = typestr;
	if(strcmp(c,"string") == 0)
		my_type = xlua_string;
	else if(strcmp(c,"number")==0)
		my_type = xlua_number;
	else if (strncmp(c,"array[",6) == 0)
	{
		while(*c && *c != '[') ++c;
		if(*c == '[')
		{
			++c;
			my_dim = atoi(c);
			my_type = xlua_array;
		}
	}
	else
		return luaL_argerror(L, 2, "Type must be number, string, or array[n]");
	
	xlua_dref * r = xlua_create_dref(
							name,
							my_type,
							my_dim,
							my_writeable,
							(my_writeable && cb) ? xlua_notify_helper : NULL,
							cb);							
	assert(r);
	
	lua_pushlightuserdata(L, r);
	return 1;
	
}

// dref -> "array[4]"
static int XTLuaGetDataRefType(lua_State * L)
{
	xtlua_dref * d = luaL_checkuserdata<xtlua_dref>(L,1,"expected dataref");

	xtlua_dref_type dt = xtlua_dref_get_type(d);
	
	switch(dt) {
	case xlua_none:
		lua_pushstring(L, "none");
		break;
	case xlua_number:
		lua_pushstring(L, "number");
		break;
	case xlua_array:
		{
			char buf[256];
			sprintf(buf,"array[%d]",xtlua_dref_get_dim(d));
			lua_pushstring(L,buf);
		}
		break;
	case xlua_string:
		lua_pushstring(L, "string");
		break;
	}	
	return 1;
}
static int XLuaGetDataRefType(lua_State * L)
{
	xlua_dref * d = luaL_checkuserdata<xlua_dref>(L,1,"expected dataref");

	xtlua_dref_type dt = xlua_dref_get_type(d);
	
	switch(dt) {
	case xlua_none:
		lua_pushstring(L, "none");
		break;
	case xlua_number:
		lua_pushstring(L, "number");
		break;
	case xlua_array:
		{
			char buf[256];
			sprintf(buf,"array[%d]",xlua_dref_get_dim(d));
			lua_pushstring(L,buf);
		}
		break;
	case xlua_string:
		lua_pushstring(L, "string");
		break;
	}	
	return 1;
}
// XPLMGetNumber dref -> value
static int XTLuaGetNumber(lua_State * L)
{
	xtlua_dref * d = luaL_checkuserdata<xtlua_dref>(L,1,"expected dataref");
	
	lua_pushnumber(L, xtlua_dref_get_number(d));
	return 1;	
}

// XPLMSetNumber dref value
static int XTLuaSetNumber(lua_State * L)
{
	xtlua_dref * d = luaL_checkuserdata<xtlua_dref>(L,1,"expected dataref");
	double v = luaL_checknumber(L, 2);
	
	xtlua_dref_set_number(d,v);
	return 0;	
}
static int XLuaGetNumber(lua_State * L)
{
	xlua_dref * d = luaL_checkuserdata<xlua_dref>(L,1,"expected dataref");
	
	lua_pushnumber(L, xlua_dref_get_number(d));
	return 1;	
}

// XPLMSetNumber dref value
static int XLuaSetNumber(lua_State * L)
{
	xlua_dref * d = luaL_checkuserdata<xlua_dref>(L,1,"expected dataref");
	double v = luaL_checknumber(L, 2);
	xlua_dref_set_number(d,v);
	return 0;	
}
// XPLMGetArray dref idx -> value
static int XTLuaGetArray(lua_State * L)
{
	xtlua_dref * d = luaL_checkuserdata<xtlua_dref>(L,1,"expected dataref");
	double idx = luaL_checknumber(L, 2);	
	
	lua_pushnumber(L, xtlua_dref_get_array(d,idx));
	return 1;	
}

// XPLMSetArray dref dix value
static int XTLuaSetArray(lua_State * L)
{
	xtlua_dref * d = luaL_checkuserdata<xtlua_dref>(L,1,"expected dataref");
	double idx = luaL_checknumber(L, 2);
	double v = luaL_checknumber(L, 3);
	
	xtlua_dref_set_array(d,idx,v);
	return 0;		
}
static int XLuaGetArray(lua_State * L)
{
	xlua_dref * d = luaL_checkuserdata<xlua_dref>(L,1,"expected dataref");
	double idx = luaL_checknumber(L, 2);	
	
	lua_pushnumber(L, xlua_dref_get_array(d,idx));
	return 1;	
}

// XPLMSetArray dref dix value
static int XLuaSetArray(lua_State * L)
{
	xlua_dref * d = luaL_checkuserdata<xlua_dref>(L,1,"expected dataref");
	double idx = luaL_checknumber(L, 2);
	double v = luaL_checknumber(L, 3);
	
	xlua_dref_set_array(d,idx,v);
	return 0;		
}
// XPLMGetString dref -> value
static int XTLuaGetString(lua_State * L)
{
	xtlua_dref * d = luaL_checkuserdata<xtlua_dref>(L,1,"expected dataref");
	
	lua_pushstring(L, xtlua_dref_get_string(d).c_str());
	return 1;	
}

// XPLMSetString dref value
static int XTLuaSetString(lua_State * L)
{
	xtlua_dref * d = luaL_checkuserdata<xtlua_dref>(L,1,"expected dataref");
	const char * s = luaL_checkstring(L, 2);
	xtlua_dref_set_string(d,string(s));
	return 0;	
}
static int XLuaGetString(lua_State * L)
{
	xlua_dref * d = luaL_checkuserdata<xlua_dref>(L,1,"expected dataref");
	
	lua_pushstring(L, xlua_dref_get_string(d).c_str());
	return 1;	
}

// XPLMSetString dref value
static int XLuaSetString(lua_State * L)
{
	xlua_dref * d = luaL_checkuserdata<xlua_dref>(L,1,"expected dataref");
	const char * s = luaL_checkstring(L, 2);
	xlua_dref_set_string(d,string(s));
	return 0;	
}
//----------------------------------------------------------------
// COMMANDS
//----------------------------------------------------------------

// XPLMFindCommand name
static int XTLuaFindCommand(lua_State * L)
{
	const char * name = luaL_checkstring(L, 1);
	xtlua_cmd * r = xtlua_find_cmd(name);
	if(!r)
	{
		lua_pushnil(L);
		return 1;
	}
	assert(r);
	
	lua_pushlightuserdata(L, r);
	return 1;
}
static int XLuaFindCommand(lua_State * L)
{
	const char * name = luaL_checkstring(L, 1);
	xlua_cmd * r = xlua_find_cmd(name);
	if(!r)
	{
		lua_pushnil(L);
		return 1;
	}
	assert(r);
	
	lua_pushlightuserdata(L, r);
	return 1;
}
// XPLMCreateCommand name desc
static int XLuaCreateCommand(lua_State * L)
{
	const char * name = luaL_checkstring(L, 1);
	const char * desc = luaL_checkstring(L, 2);

	xlua_cmd * r = xlua_create_cmd(name,desc);
	assert(r);
	
	lua_pushlightuserdata(L, r);
	return 1;
}

static void cmd_cb_helper(xtlua_cmd * cmd, int phase, float elapsed, void * ref)
{
	//printf("xtcmd_cb_helper\n");
	lua_State * L = setup_lua_callback(ref);
	if(L)
	{
		fmt_pcall_stdvars(L,module::debug_proc_from_interp(L),"if",phase, elapsed);
	}
}
static void xlcmd_cb_helper(xlua_cmd * cmd, int phase, float elapsed, void * ref)
{
	//printf("xlcmd_cb_helper\n");
	lua_State * L = setup_lua_callback(ref);
	if(L)
	{
		fmt_pcall_stdvars(L,module::debug_proc_from_interp(L),"if",phase, elapsed);
	}
}
// XPLMReplaceCommand cmd handler
static int XlLuaReplaceCommand(lua_State * L)
{
	
	xlua_cmd * d = luaL_checkuserdata<xlua_cmd>(L,1,"expected command");

	notify_cb_t * cb = wrap_lua_func(L, 2);
	
	xlua_cmd_install_handler(d, xlcmd_cb_helper, cb);
	return 0;	
}
static int XTLuaReplaceCommand(lua_State * L)
{
	xtlua_cmd * d = luaL_checkuserdata<xtlua_cmd>(L,1,"expected command");
	notify_cb_t * cb = wrap_lua_func(L, 2);
	
	xtlua_cmd_install_handler(d, cmd_cb_helper, cb);
	return 0;	
}
// XPLMWrapCommand cmd handler1 handler2
static int XTLuaWrapCommand(lua_State * L)
{
	xtlua_cmd * d = luaL_checkuserdata<xtlua_cmd>(L,1,"expected command");
	notify_cb_t * cb1 = wrap_lua_func(L, 2);
	notify_cb_t * cb2 = wrap_lua_func(L, 3);
	
	xtlua_cmd_install_pre_wrapper(d, cmd_cb_helper, cb1);
	xtlua_cmd_install_post_wrapper(d, cmd_cb_helper, cb2);
	return 0;	
}

// XPLMCommandStart cmd
static int XTLuaCommandStart(lua_State * L)
{
	xtlua_cmd * d = luaL_checkuserdata<xtlua_cmd>(L,1,"expected command");
	xtlua_cmd_start(d);
	return 0;
}

// XPLMCommandStop cmd
static int XTLuaCommandStop(lua_State * L)
{
	xtlua_cmd * d = luaL_checkuserdata<xtlua_cmd>(L,1,"expected command");
	xtlua_cmd_stop(d);
	return 0;
}

// XPLMCommandOnce cmd
static int XTLuaCommandOnce(lua_State * L)
{
	xtlua_cmd * d = luaL_checkuserdata<xtlua_cmd>(L,1,"expected command");
	xtlua_cmd_once(d);
	return 0;
}
static int XLuaCommandStart(lua_State * L)
{
	//printf("C++ command start");
	xlua_cmd * d = luaL_checkuserdata<xlua_cmd>(L,1,"expected command");
	xlua_cmd_start(d);
	return 0;
}

// XPLMCommandStop cmd
static int XLuaCommandStop(lua_State * L)
{
	xlua_cmd * d = luaL_checkuserdata<xlua_cmd>(L,1,"expected command");
	xlua_cmd_stop(d);
	return 0;
}

// XPLMCommandOnce cmd
static int XLuaCommandOnce(lua_State * L)
{
	xlua_cmd * d = luaL_checkuserdata<xlua_cmd>(L,1,"expected command");
	xlua_cmd_once(d);
	return 0;
}
//----------------------------------------------------------------
// TIMERS
//----------------------------------------------------------------

static void timer_cb(void * ref)
{
	lua_State * L = setup_lua_callback(ref);
	if(L)
	{
		fmt_pcall_stdvars(L,module::debug_proc_from_interp(L),"");
	}	
}

// XPLMCreateTimer func -> ptr
static int XTLuaCreateTimer(lua_State * L)
{
	notify_cb_t * helper = wrap_lua_func(L, -1);
	if(helper == NULL)
		return NULL;
	
	xlua_timer * t = xtlua_create_timer(timer_cb, helper);
	assert(t);
	
	lua_pushlightuserdata(L, t);
	return 1;
}

// XPLMRunTimer timer delay repeat
static int XTLuaRunTimer(lua_State * L)
{
	xlua_timer * t = luaL_checkuserdata<xlua_timer>(L,1,"expected timer");
	if(!t)
		return 0;
	
	xtlua_run_timer(t, lua_tonumber(L, -2), lua_tonumber(L, -1));
	return 0;
}

// XPLMIsTimerScheduled ptr -> int
static int XTLuaIsTimerScheduled(lua_State * L)
{
	xlua_timer * t = luaL_checkuserdata<xlua_timer>(L,1,"expected timer");
	int sched = xtlua_is_timer_scheduled(t);
	lua_pushboolean(L, sched);
	return 1;
}
static int XLuaCreateTimer(lua_State * L)
{
	notify_cb_t * helper = wrap_lua_func(L, -1);
	if(helper == NULL)
		return NULL;
	
	xlua_timer * t = xlua_create_timer(timer_cb, helper);
	assert(t);
	
	lua_pushlightuserdata(L, t);
	return 1;
}

// XPLMRunTimer timer delay repeat
static int XLuaRunTimer(lua_State * L)
{
	xlua_timer * t = luaL_checkuserdata<xlua_timer>(L,1,"expected timer");
	if(!t)
		return 0;
	
	xlua_run_timer(t, lua_tonumber(L, -2), lua_tonumber(L, -1));
	return 0;
}

// XPLMIsTimerScheduled ptr -> int
static int XLuaIsTimerScheduled(lua_State * L)
{
	xlua_timer * t = luaL_checkuserdata<xlua_timer>(L,1,"expected timer");
	int sched = xlua_is_timer_scheduled(t);
	lua_pushboolean(L, sched);
	return 1;
}


//FUNC(XLuaCreateDataRef) 
#define XT_FUNC_LIST \
	FUNC(XLuaGetCode) \
	FUNC(XTLuaFindDataRef) \
	FUNC(XTLuaGetDataRefType) \
	FUNC(XTLuaGetNumber) \
	FUNC(XTLuaSetNumber) \
	FUNC(XTLuaGetArray) \
	FUNC(XTLuaSetArray) \
	FUNC(XTLuaGetString) \
	FUNC(XTLuaSetString) \
	FUNC(XTLuaFindCommand) \
	FUNC(XTLuaReplaceCommand) \
	FUNC(XTLuaWrapCommand) \
	FUNC(XTLuaCommandStart) \
	FUNC(XTLuaCommandStop) \
	FUNC(XTLuaCommandOnce) \
	FUNC(XTLuaCreateTimer) \
	FUNC(XTLuaRunTimer) \
	FUNC(XTLuaIsTimerScheduled)
#define XL_FUNC_LIST \
	FUNC(XLuaGetCode) \
	FUNC(XLuaFindDataRef) \
	FUNC(XLuaGetDataRefType) \
	FUNC(XLuaGetNumber) \
	FUNC(XLuaSetNumber) \
	FUNC(XLuaGetArray) \
	FUNC(XLuaSetArray) \
	FUNC(XLuaGetString) \
	FUNC(XLuaSetString) \
	FUNC(XLuaFindCommand) \
	FUNC(XLuaCreateCommand) \
	FUNC(XlLuaReplaceCommand) \
	FUNC(XLuaCommandStart) \
	FUNC(XLuaCommandStop) \
	FUNC(XLuaCommandOnce) \
	FUNC(XLuaCreateTimer) \
	FUNC(XLuaRunTimer) \
	FUNC(XLuaIsTimerScheduled) \
	FUNC(XLuaCreateDataRef)

void	add_xpfuncs_to_interp(lua_State * L,bool isXT)
{
	#define FUNC(x) \
		lua_register(L,#x,x);
	if(isXT){	
		XT_FUNC_LIST;
	}
	else{
		XL_FUNC_LIST;
	}
}

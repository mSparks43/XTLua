//
//  xpdatarefs.cpp
//  xlua
//
//  Created by Ben Supnik on 3/20/16.
//
//	Copyright 2016, Laminar Research
//	This source code is licensed under the MIT open source license.
//	See LICENSE.txt for the full terms of the license.

// xTLua
// Modified by Mark Parker on 04/19/2020
#include <cstdio>
#include "xpcommands.h"
#define XPLM200 1
#include <XPLMUtilities.h>
//#include <XPLMProcessing.h>
#include "xpdatarefs.h"

//#include <XPLMDataAccess.h>
#include <XPLMPlugin.h>

#include <vector>
#include <assert.h>
#include <algorithm>


#include <XPLMUtilities.h>
#include <XPLMProcessing.h>
#include "xpmtdatarefs.h"
using std::min;
using std::max;
using std::vector;

#define MSG_ADD_DATAREF 0x01000000
#if !MOBILE
#define STAT_PLUGIN_SIG "xplanesdk.examples.DataRefEditor"
#endif

//#define TRACE_DATAREFS printf
#define TRACE_DATAREFS(...)
static XTLuaDataRefs xtluaDefs=XTLuaDataRefs();



static xlua_dref *		s_drefs = NULL;

// For nunmbers

static void resolve_dref(xlua_dref * d)
{
	xtluaDefs.XTqueueresolve_dref(d);
}

//moved to xpmtdatatypes.cpp
/*static void do_resolve_dref(xlua_dref * d)
{
	assert(d->m_dref == NULL);
	assert(d->m_types == 0);
	assert(d->m_index == -1);
	assert(d->m_ours == 0);
	d->m_dref = XPLMFindDataRef(d->m_name.c_str());
	if(d->m_dref)
	{
		d->m_index = -1;
		d->m_types = XPLMGetDataRefTypes(d->m_dref);
	}
	else
	{
		string::size_type obrace = d->m_name.find('[');
		string::size_type cbrace = d->m_name.find(']');
		if(obrace != d->m_name.npos && cbrace != d->m_name.npos)			// Gotta have found the braces
		if(obrace > 0)														// dref name can't be empty
		if(cbrace > obrace)													// braces must be in-order - avoids unsigned math insanity
		if((cbrace - obrace) > 1)											// Gotta have SOMETHING in between the braces
		{
			string number = d->m_name.substr(obrace+1,cbrace - obrace - 1);
			string refname = d->m_name.substr(0,obrace);
			
			XPLMDataRef arr = XPLMFindDataRef(refname.c_str());				// Only if we have a valid name
			if(arr)
			{
				XPLMDataTypeID tid = XPLMGetDataRefTypes(arr);
				if(tid & (xplmType_FloatArray | xplmType_IntArray))			// AND are array type
				{
					int idx = atoi(number.c_str());							// AND have a non-negetive index
					if(idx >= 0)
					{
						d->m_dref = arr;									// Now we know we're good, record all of our info
						d->m_types = tid;
						d->m_index = idx;
					}
				}
			}
		}
	}
}*/

void			xlua_validate_drefs()
{
	for(xlua_dref * f = s_drefs; f; f = f->m_next)
	{
	#if MOBILE
		assert(f->m_dref != NULL);
	#else
		if(f->m_dref == NULL)
			printf("WARNING: dataref %s is used but not defined.\n", f->m_name.c_str());
	#endif
	}
}

xlua_dref *		xlua_find_dref(const char * name)
{
	for(xlua_dref * f = s_drefs; f; f = f->m_next)
	if(f->m_name == name)
	{
		TRACE_DATAREFS("Found %s as %p\n", name,f);
		return f;
	}
	// We have never tried to find this dref before - make a new record
	xlua_dref * d = new xlua_dref;
	d->m_next = s_drefs;
	s_drefs = d;
	d->m_name = name;
	d->m_dref = NULL;
	d->m_index = -1;
	d->m_types = 0;
	d->m_ours = 0;
	//d->m_notify_func = NULL;
	//d->m_notify_ref = NULL;
	//d->m_number_storage = 0;
	
	resolve_dref(d);

	TRACE_DATAREFS("Speculating %s as %p\n", name,d);

	return d;
}

xlua_dref *		xlua_create_dref(const char * name, xlua_dref_type type, int dim, int writable, xlua_dref_notify_f func, void * ref)
{
	printf("ERROR: xTLua cannot create datarefs - us xLua.\n",name);
	return NULL;
	/*
	assert(type != xlua_none);
	assert(name);
	assert(type != xlua_array || dim > 0);
	assert(writable || func == NULL);
	
	string n(name);
	xlua_dref * f;
	for(f = s_drefs; f; f = f->m_next)
	if(f->m_name == n)
	{
		if(f->m_ours || f->m_dref)
		{
			printf("ERROR: %s is already a dataref.\n",name);
			return NULL;
		}
		TRACE_DATAREFS("Reusing %s as %p\n", name,f);		
		break;
	}
	
	if(n.find('[') != n.npos)
	{
		printf("ERROR: %s contains brackets in its name.\n", name);
		return NULL;
	}
	
	XPLMDataRef other = XPLMFindDataRef(name);
	if(other && XPLMIsDataRefGood(other))
	{
		printf("ERROR: %s is used by another plugin.\n", name);
		return NULL;
	}
	
	xlua_dref * d = f;
	if(!d)
	{
		d = new xlua_dref;
		d->m_next = s_drefs;
		s_drefs = d;
		TRACE_DATAREFS("Creating %s as %p\n", name,d);		
	}
	d->m_name = name;
	d->m_index = -1;
	d->m_ours = 1;
	//d->m_notify_func = func;
	//d->m_notify_ref = ref;
	//d->m_number_storage = 0;

	switch(type) {
	case xlua_number:
		d->m_types = xplmType_Int|xplmType_Float|xplmType_Double;
		d->m_dref = XPLMRegisterDataAccessor(name, d->m_types, writable,
						xlua_geti, xlua_seti,
						xlua_getf, xlua_setf,
						xlua_getd, xlua_setd,
						NULL, NULL,
						NULL, NULL,
						NULL, NULL,
						d, d);
		break;
	case xlua_array:
		d->m_types = xplmType_FloatArray|xplmType_IntArray;
		d->m_dref = XPLMRegisterDataAccessor(name, d->m_types, writable,
						NULL, NULL,
						NULL, NULL,
						NULL, NULL,
						xlua_getvi, xlua_setvi,
						xlua_getvf, xlua_setvf,
						NULL, NULL,
						d, d);
		//d->m_array_storage.resize(dim);
		break;
	case xlua_string:
		d->m_types = xplmType_Data;
		d->m_dref = XPLMRegisterDataAccessor(name, d->m_types, writable,
						NULL, NULL,
						NULL, NULL,
						NULL, NULL,
						NULL, NULL,
						NULL, NULL,
						xlua_getvb, xlua_setvb,
						d, d);
		break;
	case xlua_none:
		break;
	}

	return d;*/
}

xlua_dref_type	xlua_dref_get_type(xlua_dref * who)
{
	if(who->m_types & xplmType_Data)
		return xlua_string;
	if(who->m_index >= 0)
		return  xlua_number;
	if(who->m_types & (xplmType_FloatArray|xplmType_IntArray))
		return xlua_array;
	if(who->m_types & (xplmType_Int|xplmType_Float|xplmType_Double))
		return xlua_number;
	return xlua_none;
}

void			xlua_dref_preUpdate(){
	
}
// meat of setting drefs in here - lock the lua thread and update all datarefs to/from X-Plane
void			xlua_dref_postUpdate(){

	//xtluaDefs.ShowDataRefs();
	xtluaDefs.updateDataRefs();
}
int	xlua_dref_get_dim(xlua_dref * who)
{
	//if(who->m_ours)
	//	return who->m_array_storage.size();
	if(who->m_types & xplmType_Data)
		return 0;
	if(who->m_index >= 0)
		return  1;
	if(who->m_types & xplmType_FloatArray||who->m_types & xplmType_IntArray)
	{
		return xtluaDefs.XTGetDatavf(who, NULL, 0, 0,who->m_ours);
	}
	/*if(who->m_types & xplmType_IntArray)
	{
		return xtluaDefs.XTGetDatavi(who->m_dref, NULL, 0, 0,who->m_ours);
	}*/
	if(who->m_types & (xplmType_Int|xplmType_Float|xplmType_Double))
		return 1;
	return 0;
}

double			xlua_dref_get_number(xlua_dref * d)
{
	//if(d->m_ours)
	//	return d->m_number_storage;
	
	if(d->m_index >= 0)
	{
		if(d->m_types & xplmType_FloatArray)
		{
			float r;
			if(xtluaDefs.XTGetDatavf(d, &r, d->m_index, 1,d->m_ours))
				return r;
			return 0.0;
		}
		if(d->m_types & xplmType_IntArray)
		{
			int r;
			if(xtluaDefs.XTGetDatavi(d->m_dref, &r, d->m_index, 1,d->m_ours))
				return r;
			return 0.0;
		}
		return 0.0;
	}
	/*if(d->m_types & xplmType_Double)
	{
		return xtluaDefs.XTGetDatad(d->m_dref,d->m_ours);
	}*/
	if(d->m_types & xplmType_Float||d->m_types & xplmType_Int||d->m_types & xplmType_Double)
	{
		return xtluaDefs.XTGetDataf(d->m_dref,d->m_ours);
	}
	/*if(d->m_types & xplmType_Int)
	{
		return xtluaDefs.XTGetDatai(d->m_dref,d->m_ours);
	}*/
	return 0.0;
}

void			xlua_dref_set_number(xlua_dref * d, double value)
{
	/*if(d->m_ours)
	{
		d->m_number_storage = value;
		return;
	}*/

	if(d->m_index >= 0)
	{
		if(d->m_types & xplmType_FloatArray||d->m_types & xplmType_IntArray)
		{
			/*float r = (float)value;
			printf("set %f\n",value);*/
			xtluaDefs.XTSetDatavf(d, value, d->m_index);
		}
		/*if(d->m_types & xplmType_IntArray)
		{
			int r = value;
			xtluaDefs.XTSetDatavi(d->m_dref, &r, d->m_index, 1,d->m_ours);
		}*/
	}
	/*if(d->m_types & xplmType_Double)
	{
		xtluaDefs.XTSetDatad(d->m_dref, value,d->m_ours);
	}*/
	if(d->m_types & xplmType_Float || d->m_types & xplmType_Double || d->m_types & xplmType_Int)
	{
		xtluaDefs.XTSetDataf(d->m_dref, value,d->m_ours);
	}
	/*if(d->m_types & xplmType_Int)
	{
		xtluaDefs.XTSetDatai(d->m_dref, value,d->m_ours);
	}*/
}

double			xlua_dref_get_array(xlua_dref * d, int n)
{
	assert(n >= 0);
	/*if(d->m_ours)
	{
		if(n >= d->m_array_storage.size())
		return 0.0;
		//if(n < d->m_array_storage.size())
		//	return d->m_array_storage[n];
		//return 0.0;
	}*/
	if((d->m_types & xplmType_FloatArray)||(d->m_types & xplmType_IntArray))
	{
		float r;
		if(xtluaDefs.XTGetDatavf(d, &r, n, 1,d->m_ours))
			return r;
		return 0.0;
	}
	/*if(d->m_types & xplmType_IntArray)
	{
		int r;
		if(xtluaDefs.XTGetDatavi(d->m_dref, &r, n, 1,d->m_ours))
			return r;
		return 0.0;
	}*/
	return 0.0;
}

void			xlua_dref_set_array(xlua_dref * d, int n, double value)
{
	assert(n >= 0);
	/*(d->m_ours)
	{
		if(n < d->m_array_storage.size())
			d->m_array_storage[n] = value;
		//return;
	}*/
	//printf("set %s to %f types=%d\n",d->m_name.c_str(),value,d->m_types);
	if((d->m_types & xplmType_FloatArray) || (d->m_types & xplmType_IntArray))
	{
		//float r = value;
		//printf("do set %s to %f types=%d\n",d->m_name.c_str(),value,d->m_types);
		xtluaDefs.XTSetDatavf(d, value, n);
	}
	/*if(d->m_types & xplmType_IntArray)
	{
		int r = value;
		xtluaDefs.XTSetDatavi(d->m_dref, &r, n, 1,d->m_ours);
	}*/
}

string			xlua_dref_get_string(xlua_dref * d)
{
	//if(d->m_ours)
	//	return d->m_string_storage;
	
	if(d->m_types & xplmType_Data)
	{
		int l = xtluaDefs.XTGetDatab(d, NULL, 0, 0,d->m_ours);
		if(l > 0)
		{
			vector<char>	buf(l);
			l = xtluaDefs.XTGetDatab(d, &buf[0], 0, l,d->m_ours);
			assert(l <= buf.size());
			if(l == buf.size())
			{
				return string(buf.begin(),buf.end());
			}
		}
	}
	return string();
}

void			xlua_dref_set_string(xlua_dref * d, const string& value)
{
	if(d->m_ours)
	{
		d->m_string_storage = value;
		//return;
	}
	if(d->m_types & xplmType_Data)
	{
		//const char * begin = value.c_str();
		//const char * end = begin + value.size();
		//if(end > begin)
		{
			xtluaDefs.XTSetDatab(d, value);
		}
	}
}

// This attempts to re-establish the name->dref link for any unresolved drefs.  This can be used if we declare
// our dref early and then ANOTHER add-on is loaded that defines it.
void			xlua_relink_all_drefs()
{
#if !MOBILE
	XPLMPluginID dre = XPLMFindPluginBySignature(STAT_PLUGIN_SIG);
	if(dre != XPLM_NO_PLUGIN_ID)
	if(!XPLMIsPluginEnabled(dre))
	{
		printf("WARNING: can't register drefs - DRE is not enabled.\n");
		dre = XPLM_NO_PLUGIN_ID;
	}
#endif
	for(xlua_dref * d = s_drefs; d; d = d->m_next)
	{
		if(d->m_dref == NULL)
		{
			assert(!d->m_ours);
			resolve_dref(d);
		}
#if !MOBILE
		if(d->m_ours)
		if(dre != XPLM_NO_PLUGIN_ID)
		{
			printf("registered: %s\n", d->m_name.c_str());
			XPLMSendMessageToPlugin(dre, MSG_ADD_DATAREF, (void *)d->m_name.c_str());		
		}		
#endif
	}
}

std::vector<XTCmd> runQueue;
std::vector<string> messageQueue;
std::mutex data_mutex;
static int xlua_std_pre_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref)
{

	xlua_cmd * me = (xlua_cmd *) ref;
	if(phase == xplm_CommandBegin)
		me->m_down_time = xtluaDefs.XTGetElapsedTime();
	if(me->m_pre_handler){
		XTCmd command;
		command.runFunc=me->m_pre_handler;
		command.m_func_ref=me->m_pre_ref;
		command.phase=phase;
		
		command.xluaref=me;
		
		command.duration=XPLMGetElapsedTime() - me->m_down_time;
		data_mutex.lock();
		runQueue.push_back(command);
		data_mutex.unlock();
	}
	printf("Pre command %s\n",me->m_name.c_str());

	/*
	if(me->m_pre_handler)
		me->m_pre_handler(me, phase, xtluaDefs.XTGetElapsedTime() - me->m_down_time, me->m_pre_ref);*/
	return 1;
}

static int xlua_std_main_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref)
{
	xlua_cmd * me = (xlua_cmd *) ref;
	if(phase == xplm_CommandBegin)
		me->m_down_time = xtluaDefs.XTGetElapsedTime();
	if(me->m_main_handler){
		XTCmd command;
		command.runFunc=me->m_main_handler;
		command.m_func_ref=me->m_main_ref;
		command.phase=phase;
		
		command.xluaref=me;
		
		command.duration=XPLMGetElapsedTime() - me->m_down_time;
		data_mutex.lock();
		runQueue.push_back(command);
		data_mutex.unlock();
	}
	printf("main command %s\n",me->m_name.c_str());
	/*if(phase == xplm_CommandBegin)
		me->m_down_time = xtluaDefs.XTGetElapsedTime();
	if(me->m_main_handler)
		me->m_main_handler(me, phase, xtluaDefs.XTGetElapsedTime() - me->m_down_time, me->m_main_ref);*/
	return 0;
}

static int xlua_std_post_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref)
{
	xlua_cmd * me = (xlua_cmd *) ref;
	if(phase == xplm_CommandBegin)
		me->m_down_time = xtluaDefs.XTGetElapsedTime();
	if(me->m_post_handler){
		XTCmd command;
		command.runFunc=me->m_post_handler;
		command.m_func_ref=me->m_post_ref;
		command.phase=phase;
		
		command.xluaref=me;
		
		command.duration=XPLMGetElapsedTime() - me->m_down_time;
		data_mutex.lock();
		runQueue.push_back(command);
		data_mutex.unlock();
	}
	printf("post command %s\n",me->m_name.c_str());
	/*if(phase == xplm_CommandBegin)
		me->m_down_time = xtluaDefs.XTGetElapsedTime();
	if(me->m_post_handler)
		me->m_post_handler(me, phase, xtluaDefs.XTGetElapsedTime() - me->m_down_time, me->m_post_ref);*/
	return 1;
}
std::vector<XTCmd> get_runQueue(){
	std::vector<XTCmd> items;
	data_mutex.lock();
	for(XTCmd item:runQueue)
		items.push_back(item);
	runQueue.clear();
	data_mutex.unlock();
	return items;
}
std::vector<string> get_runMessages(){
	std::vector<string> items;
	data_mutex.lock();
	for(string item:messageQueue)
		items.push_back(item);
	messageQueue.clear();
	data_mutex.unlock();
	return items;
}
void xlua_add_callout(string callout){
	printf("xlua_add_callout %s\n",callout.c_str());
	data_mutex.lock();
	messageQueue.push_back(callout);
	data_mutex.unlock();
}
double xlua_get_simulated_time(){
	//printf("get sim time\n");
	data_mutex.lock();
	double retVal=xtluaDefs.XTGetElapsedTime();
	data_mutex.unlock();
	return retVal;
}
int xlua_dref_resolveDREFQueue(){
	int retVal=xtluaDefs.resolveQueue();
	if(retVal>0){
		std::vector<xlua_cmd*> commandstoHandle=xtluaDefs.XTGetHandlers();
		printf("have %d commands to handle registering\n",commandstoHandle.size());
		for(xlua_cmd* cmd: commandstoHandle){
			if(cmd->m_pre_handler){
				printf("XPLMRegisterPreCommandHandler %s\n",cmd->m_name.c_str());
				XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_pre_handler, 1, cmd);
			}
			if(cmd->m_main_handler){
				printf("XPLMRegisterCommandHandler %s\n",cmd->m_name.c_str());
				XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_main_handler, 1, cmd);
			}
			if(cmd->m_post_handler){
				printf("XPLMRegisterPostCommandHandler %s\n",cmd->m_name.c_str());
				XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_post_handler, 0, cmd);
			}
		}
		retVal++;
	}
	return retVal;
}


void			xlua_dref_cleanup()
{
	while(s_drefs)
	{
		xlua_dref *	kill = s_drefs;
		s_drefs = s_drefs->m_next;
		
		if(kill->m_dref && kill->m_ours)
		{
			XPLMUnregisterDataAccessor(kill->m_dref);
		}
		
		delete kill;
	}
	xtluaDefs.cleanup();
}

//
//  merge of xpcommands.cpp
//  xlua
//
//  Created by Benjamin Supnik on 4/12/16.
//
//	Copyright 2016, Laminar Research
//	This source code is licensed under the MIT open source license.
//	See LICENSE.txt for the full terms of the license.

// xTLua
// Merged by Mark Parker on 04/23/2020


static xlua_cmd *		s_cmds = NULL;


static void resolve_cmd(xlua_cmd * d)
{
	xtluaDefs.XTqueueresolve_cmd(d);
}
xlua_cmd * xlua_find_cmd(const char * name)
{
	for(xlua_cmd * i = s_cmds; i; i = i->m_next)
	if(i->m_name == name)
		return i;
		
	/*XPLMCommandRef c = XPLMFindCommand(name);	
	if(c == NULL){
		printf("ERROR: Command %s not found\n",name);
	} return NULL;*/	
		
	xlua_cmd * nc = new xlua_cmd;
	nc->m_next = s_cmds;
	s_cmds = nc;
	nc->m_name = name;
	resolve_cmd(nc);
	//nc->m_cmd = c;
	return nc;
}

xlua_cmd * xlua_create_cmd(const char * name, const char * desc)
{
	printf("ERROR: xTLua cannot create command - %s - use xLua and wrap them here.\n",name);
	return NULL;
	/*
	for(xlua_cmd * i = s_cmds; i; i = i->m_next)
	if(i->m_name == name)
	{
		if(i->m_ours)
		{
			printf("ERROR: command already exists: %s\n", name);
			return NULL;
		}
		i->m_ours = 1;
		return i;
	}

	// Ben says: we used to try to barf on commands taken over from other plugins but
	// we can get spooked by our own shadow - if we had a command last run and the user 
	// bound it to a joystick and saved prefs then...it will ALREADY exist!  So don't
	// FTFO here.

//	if(XPLMFindCommand(name) != NULL)
//	{
//		printf("ERROR: command already in use by other plugin or X-Plane: %s\n", name);
//		return NULL;
//	}

	xlua_cmd * nc = new xlua_cmd;
	nc->m_next = s_cmds;
	s_cmds = nc;
	nc->m_name = name;
	nc->m_cmd = XPLMCreateCommand(name,desc);
	nc->m_ours = 1;
	return nc;*/
}

void xlua_cmd_install_handler(xlua_cmd * cmd, xlua_cmd_handler_f handler, void * ref)
{
	if(cmd->m_main_handler != NULL)
	{
		printf("ERROR: there is already a main handler installed: %s.\n", cmd->m_name.c_str());
		return;
	}
	cmd->m_main_handler = handler;
	cmd->m_main_ref = ref;
	//XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_main_handler, 1, cmd);
	
	xtluaDefs.XTRegisterCommandHandler(cmd);
}


void xlua_cmd_install_pre_wrapper(xlua_cmd * cmd, xlua_cmd_handler_f handler, void * ref)
{
	if(cmd->m_pre_handler != NULL)
	{
		printf("ERROR: there is already a pre handler installed: %s.\n", cmd->m_name.c_str());
		return;
	}
	cmd->m_pre_handler = handler;
	cmd->m_pre_ref = ref;
	xtluaDefs.XTRegisterCommandHandler(cmd);
	//XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_pre_handler, 1, cmd);
}

void xlua_cmd_install_post_wrapper(xlua_cmd * cmd, xlua_cmd_handler_f handler, void * ref)
{
	if(cmd->m_post_handler != NULL)
	{
		printf("ERROR: there is already a post handler installed: %s.\n", cmd->m_name.c_str());
		return;	
	}
	cmd->m_post_handler = handler;
	cmd->m_post_ref = ref;
	xtluaDefs.XTRegisterCommandHandler(cmd); 
	//XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_post_handler, 0, cmd);
}

void xlua_cmd_start(xlua_cmd * cmd)
{
	xtluaDefs.XTCommandBegin(cmd);
	//XPLMCommandBegin(cmd->m_cmd);
}
void xlua_cmd_stop(xlua_cmd * cmd)
{
	xtluaDefs.XTCommandEnd(cmd);
	//XPLMCommandEnd(cmd->m_cmd);
}

void xlua_cmd_once(xlua_cmd * cmd)
{
	xtluaDefs.XTCommandOnce(cmd);
	//XPLMCommandOnce(cmd->m_cmd);
}

void xlua_cmd_cleanup()
{
	while(s_cmds)
	{
		xlua_cmd * k = s_cmds;
		if(k->m_pre_handler)
			XPLMUnregisterCommandHandler(k->m_cmd, xlua_std_pre_handler, 1, k);
		if(k->m_main_handler)
			XPLMUnregisterCommandHandler(k->m_cmd, xlua_std_main_handler, 1, k);
		if(k->m_post_handler)
			XPLMUnregisterCommandHandler(k->m_cmd, xlua_std_post_handler, 0, k);
		s_cmds = s_cmds->m_next;
		delete k;
	}
}




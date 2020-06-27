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

static bool active=false; //local marker to enable/disable dataref read and writes during startup and shutdown
struct	xlua_dref {
	xlua_dref *				m_next;
	string					m_name;
	XPLMDataRef				m_dref;
	int						m_index;	// -1 if index is NOT bound.
	XPLMDataTypeID			m_types;
	int						m_ours;		// 1 if we made, 0 if system
	xlua_dref_notify_f		m_notify_func;
	void *					m_notify_ref;
	//bool 					changed;
	// IF we made the dataref, this is where our storage is!
	double					m_number_storage;
	vector<double>			m_array_storage;
	string					m_string_storage;
};
std::unordered_map<xlua_dref*,bool> changedDrefs;
static xlua_dref *		l_drefs = NULL;


static xtlua_dref *		s_drefs = NULL;
static std::mutex xlua_data_mutex;
static std::mutex xlua_change_mutex;
//for change locking
static void	xlua_dref_changed(xlua_dref * r){
	xlua_change_mutex.lock();
	changedDrefs[r]=true;
	xlua_change_mutex.unlock();
}

// For nunmbers
static int	xlua_geti(void * ref)
{
	if(!active)
		return 0;
	
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	xlua_data_mutex.lock();	
	int retVal=r->m_number_storage;
	xlua_data_mutex.unlock();
	return retVal;
}

static void	xlua_seti(void * ref, int v)
{
	if(!active)
		return;
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	double vv = v;
	//r->changed=false;
	xlua_data_mutex.lock();	
	if(r->m_number_storage != vv)
	{
		r->m_number_storage = vv;
		xlua_dref_changed(r);
	}
	xlua_data_mutex.unlock();
	//if(changed && r->m_notify_func)
	//	r->m_notify_func(r,r->m_notify_ref);	
}

static float xlua_getf(void * ref)
{
	if(!active)
		return 0;
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	xlua_data_mutex.lock();	
	float retVal=r->m_number_storage;
	xlua_data_mutex.unlock();
	return retVal;
}

static void	xlua_setf(void * ref, float v)
{
	if(!active)
		return;
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	double vv = v;
	//r->changed=false;
	xlua_data_mutex.lock();	
	if(r->m_number_storage != vv)
	{
		r->m_number_storage = vv;
		xlua_dref_changed(r);
	}
	xlua_data_mutex.unlock();
	//if(changed && r->m_notify_func)
	//	r->m_notify_func(r,r->m_notify_ref);
}

static double xlua_getd(void * ref)
{
	if(!active)
		return 0;
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	xlua_data_mutex.lock();	
	double retVal=r->m_number_storage;
	xlua_data_mutex.unlock();
	return retVal;
}

static void	xlua_setd(void * ref, double v)
{
	if(!active)
		return;
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	double vv = v;
	//bool changed = false;
	xlua_data_mutex.lock();
	if(r->m_number_storage != vv)
	{
		r->m_number_storage = vv;
		xlua_dref_changed(r);
	}
	
	xlua_data_mutex.unlock();
	//if(changed && r->m_notify_func)
	//	r->m_notify_func(r,r->m_notify_ref);
}

// For arrays
static int xlua_getvi(void * ref, int * values, int offset, int max)
{
	if(!active)
		return 0;
	xlua_dref * r = (xlua_dref *) ref;
	xlua_data_mutex.lock();
	int count=0;
	assert(r->m_ours);
	if(values == NULL){
		count= r->m_array_storage.size();
		xlua_data_mutex.unlock();
		return count;
	}
	//if(offset >= r->m_array_storage.size())
	//	return 0;
	count = min(max, (int) r->m_array_storage.size() - offset);
	for(int i = 0; i < count; ++i)
		values[i] = r->m_array_storage[i + offset];
	xlua_data_mutex.unlock();
	return count;
}

static void xlua_setvi(void * ref, int * values, int offset, int max)
{
	if(!active)
		return;
	assert(values);
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	xlua_data_mutex.lock();
	//if(offset >= r->m_array_storage.size()){
	//	return;
	//}
	int count = min(max, (int) r->m_array_storage.size() - offset);
	//bool changed = false;
	for(int i = 0; i < count; ++i)
	{
		double vv = values[i];
		if(r->m_array_storage[i + offset] != vv)
		{
			r->m_array_storage[i + offset] = vv;
			xlua_dref_changed(r);
		}
	}
	xlua_data_mutex.unlock();
	//if(changed && r->m_notify_func)
	//	r->m_notify_func(r,r->m_notify_ref);
	
}

static int xlua_getvf(void * ref, float * values, int offset, int max)
{
	if(!active)
		return 0;
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	xlua_data_mutex.lock();
	int count=0;
	if(values == NULL){
		count=r->m_array_storage.size();
		xlua_data_mutex.unlock();
		return count;
	}
	//if(offset >= r->m_array_storage.size())
	//	return 0;
	count = min(max, (int) r->m_array_storage.size() - offset);
	for(int i = 0; i < count; ++i)
		values[i] = r->m_array_storage[i + offset];
	xlua_data_mutex.unlock();
	return count;
}

static void xlua_setvf(void * ref, float * values, int offset, int max)
{
	if(!active)
		return;
	assert(values);
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	//if(offset >= r->m_array_storage.size())
	//	return;
	xlua_data_mutex.lock();
	//bool changed = false;
	int count = min(max, (int) r->m_array_storage.size() - offset);
	for(int i = 0; i < count; ++i)
	{
		double vv = values[i];
		if(r->m_array_storage[i + offset] != vv)
		{
			r->m_array_storage[i + offset] = vv;
			xlua_dref_changed(r);
		}
	}
	xlua_data_mutex.unlock();
	//if(changed && r->m_notify_func)
	//	r->m_notify_func(r,r->m_notify_ref);
	
}

// For strings
static int xlua_getvb(void * ref, void * values, int offset, int max)
{
	if(!active)
		return 0;
	char * dst = (char *) values;
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	xlua_data_mutex.lock();
	int count = 0;
	if(values == NULL){
		count=r->m_string_storage.size();
		xlua_data_mutex.unlock();
		return count;
	}
	//if(offset >= r->m_string_storage.size())
	//	return 0;
	count = min(max, (int) r->m_string_storage.size() - offset);
	for(int i = 0; i < count; ++i)
		dst[i] = r->m_string_storage[i + offset];
	xlua_data_mutex.unlock();
	return count;
}

static void xlua_setvb(void * ref, void * values, int offset, int max)
{
	if(!active)
		return;
	assert(values);
	const char * src = (const char *) values;
	int new_len = offset + max;
	xlua_dref * r = (xlua_dref *) ref;
	assert(r->m_ours);
	xlua_data_mutex.lock();
	string orig(r->m_string_storage);
	r->m_string_storage.resize(new_len);
	for(int i = 0; i < max; ++i)
		r->m_string_storage[i + offset] = src[i];
	if(r->m_string_storage != orig)
		xlua_dref_changed(r);
	xlua_data_mutex.unlock();
	//if(r->m_notify_func && changed)
	//	r->m_notify_func(r,r->m_notify_ref);
	
}
void	grabLocal(xtlua_dref * who){
	xlua_dref * f;
	for(f = l_drefs; f; f = f->m_next)
	if(f->m_name == who->m_name&&f->m_ours==1)
	{
		who->local_dref=f;
		who->m_index=f->m_index;
		who->m_types=f->m_types;
		who->m_ours=1;
		return;
		
	}

}

static void resolve_lua_dref(xlua_dref * d)
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
				printf("found arr dref %s",d->m_name.c_str());
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
}

static void resolve_dref(xtlua_dref * d)
{
	xtluaDefs.XTqueueresolve_dref(d);
}
static void resolve_xp_dref(xlua_dref * d)
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
}
//moved to xpmtdatatypes.cpp
/*static void do_resolve_dref(xtlua_dref * d)
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
	for(xtlua_dref * f = s_drefs; f; f = f->m_next)
	{
	#if MOBILE
		assert(f->m_dref != NULL);
	#else
		if(f->m_dref == NULL)
			printf("WARNING: xtlua dataref %s is used but not defined.\n", f->m_name.c_str());
	#endif


	}
	for(xlua_dref * f = l_drefs; f; f = f->m_next)
	{
	#if MOBILE
		assert(f->m_dref != NULL);
	#else
		if(f->m_dref == NULL)
			printf("WARNING: xlua dataref %s is used but not defined.\n", f->m_name.c_str());
	#endif
	}
	xtluaDefs.refreshAllDataRefs();


}

xlua_dref *		xlua_find_dref(const char * name)
{
	for(xlua_dref * f = l_drefs; f; f = f->m_next)
	if(f->m_name == name)
	{
		TRACE_DATAREFS("Found %s as %p\n", name,f);
		return f;
	}
	// We have never tried to find this dref before - make a new record
	xlua_dref * d = new xlua_dref;
	d->m_next = l_drefs;
	l_drefs = d;
	d->m_name = name;
	d->m_dref = NULL;
	d->m_index = -1;
	d->m_types = 0;
	d->m_ours = 0;
	d->m_notify_func = NULL;
	d->m_notify_ref = NULL;
	d->m_number_storage = 0;
	
	resolve_lua_dref(d);

	TRACE_DATAREFS("Speculating %s as %p\n", name,d);

	return d;
}
xtlua_dref *		xtlua_find_dref(const char * name)
{
	for(xtlua_dref * f = s_drefs; f; f = f->m_next)
	if(f->m_name == name)
	{
		TRACE_DATAREFS("Found %s as %p\n", name,f);
		return f;
	}
	// We have never tried to find this dref before - make a new record
	xtlua_dref * d = new xtlua_dref;
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
xlua_dref *		xlua_create_dref(const char * name, xtlua_dref_type type, int dim, int writable, xlua_dref_notify_f func, void * ref)
{
	//printf("ERROR: xTLua cannot create datarefs - us xLua.\n");
	//return NULL;
	
	assert(type != xlua_none);
	assert(name);
	assert(type != xlua_array || dim > 0);
	assert(writable || func == NULL);
	
	string n(name);
	xlua_dref * f;
	for(f = l_drefs; f; f = f->m_next)
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
		d->m_next = l_drefs;
		l_drefs = d;
		TRACE_DATAREFS("Creating %s as %p\n", name,d);		
	}
	d->m_name = name;
	d->m_index = -1;
	d->m_ours = 1;
	d->m_notify_func = func;
	d->m_notify_ref = ref;
	d->m_number_storage = 0;

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
		//printf("create array %s %d",name,dim);
		d->m_types = xplmType_FloatArray|xplmType_IntArray;
		d->m_dref = XPLMRegisterDataAccessor(name, d->m_types, writable,
						NULL, NULL,
						NULL, NULL,
						NULL, NULL,
						xlua_getvi, xlua_setvi,
						xlua_getvf, xlua_setvf,
						NULL, NULL,
						d, d);
		d->m_array_storage.resize(dim);
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

	return d;
}

xtlua_dref_type	xtlua_dref_get_type(xtlua_dref * who)
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
xtlua_dref_type	xlua_dref_get_type(xlua_dref * who)
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
void			xtlua_dref_preUpdate(){
	std:vector< xlua_dref *> changes;
	xlua_change_mutex.lock();
	for (auto x : changedDrefs) {
        //int i=0;
        xlua_dref * r=x.first;
		changes.push_back(r);
	}
	changedDrefs.clear();
	xlua_change_mutex.unlock();
	for ( xlua_dref * r: changes) {
		if(r->m_notify_func)
			r->m_notify_func(r,r->m_notify_ref);
	}
	
}
// meat of setting drefs in here - lock the lua thread and update all datarefs to/from X-Plane
void			xtlua_dref_postUpdate(){

	//xtluaDefs.ShowDataRefs();
	xtluaDefs.updateDataRefs();

}
int	xtlua_dref_get_dim(xtlua_dref * who)
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
void	xlua_dref_ours(xlua_dref * who)
{
	assert(who->m_ours==1);
}
int	xlua_dref_get_dim(xlua_dref * who)
{
	int retVal=0;
	xlua_data_mutex.lock();
	if(who->m_ours)
		retVal=who->m_array_storage.size();
	else if(who->m_types & xplmType_Data)
		retVal=0;
	else if(who->m_index >= 0)
		retVal=1;
	else if(who->m_types & xplmType_FloatArray)
	{
		retVal=XPLMGetDatavf(who->m_dref, NULL, 0, 0);
	}
	else if(who->m_types & xplmType_IntArray)
	{
		retVal=XPLMGetDatavi(who->m_dref, NULL, 0, 0);
	}
	else if(who->m_types & (xplmType_Int|xplmType_Float|xplmType_Double))
		retVal=1;
	xlua_data_mutex.unlock();
	return 0;
}
double			xlua_dref_get_number(xlua_dref * d)
{
	double retVal=0.0;
	xlua_data_mutex.lock();
	if(d->m_ours)
		retVal=d->m_number_storage;
	else if(d->m_index >= 0)
	{
		if(d->m_types & xplmType_FloatArray)
		{
			float r;
			if(XPLMGetDatavf(d->m_dref, &r, d->m_index, 1))
				retVal=r;
			else
				retVal=0.0;
		}
		else if(d->m_types & xplmType_IntArray)
		{
			int r;
			if(XPLMGetDatavi(d->m_dref, &r, d->m_index, 1))
				retVal=r;
			else
				retVal=0.0;
		}
		else
			retVal=0.0;
	}
	else if(d->m_types & xplmType_Double)
	{
		retVal=XPLMGetDatad(d->m_dref);
	}
	else if(d->m_types & xplmType_Float)
	{
		retVal=XPLMGetDataf(d->m_dref);
	}
	else if(d->m_types & xplmType_Int)
	{
		retVal=XPLMGetDatai(d->m_dref);
	}
	xlua_data_mutex.unlock();
	return retVal;
}

void			xlua_dref_set_number(xlua_dref * d, double value)
{
	if(d->m_ours)
	{
		xlua_data_mutex.lock();
		if(value!=d->m_number_storage)
			xlua_dref_changed(d);
		d->m_number_storage = value;
		xlua_data_mutex.unlock();
		return;
	}

	if(d->m_index >= 0)
	{
		if(d->m_types & xplmType_FloatArray)
		{
			float r = value;
			XPLMSetDatavf(d->m_dref, &r, d->m_index, 1);
		}
		if(d->m_types & xplmType_IntArray)
		{
			int r = value;
			XPLMSetDatavi(d->m_dref, &r, d->m_index, 1);
		}
	}
	if(d->m_types & xplmType_Double)
	{
		XPLMSetDatad(d->m_dref, value);
	}
	if(d->m_types & xplmType_Float)
	{
		XPLMSetDataf(d->m_dref, value);
	}
	if(d->m_types & xplmType_Int)
	{
		XPLMSetDatai(d->m_dref, value);
	}
}
double			xlua_dref_get_array(xlua_dref * d, int n)
{
	assert(n >= 0);
	if(d->m_ours)
	{
		xlua_data_mutex.lock();
		double retVal=0.0;
		if(n < d->m_array_storage.size())
			retVal=d->m_array_storage[n];
		//printf("get array %d=%f\n",n,retVal);	
		xlua_data_mutex.unlock();
		return retVal;
	}
	if(d->m_types & xplmType_FloatArray)
	{
		float r;
		if(XPLMGetDatavf(d->m_dref, &r, n, 1))
			return r;
		return 0.0;
	}
	if(d->m_types & xplmType_IntArray)
	{
		int r;
		if(XPLMGetDatavi(d->m_dref, &r, n, 1))
			return r;
		return 0.0;
	}
	return 0.0;
}

void			xlua_dref_set_array(xlua_dref * d, int n, double value)
{
	assert(n >= 0);
	if(d->m_ours)
	{
		xlua_data_mutex.lock();
		if(n < d->m_array_storage.size()){
			if(value!=d->m_array_storage[n])
				xlua_dref_changed(d);
			//printf("set array %d=%f\n",n,value);	
			d->m_array_storage[n] = value;
		}
		xlua_data_mutex.unlock();
		return;
	}
	if(d->m_types & xplmType_FloatArray)
	{
		float r = value;
		XPLMSetDatavf(d->m_dref, &r, n, 1);
	}
	if(d->m_types & xplmType_IntArray)
	{
		int r = value;
		XPLMSetDatavi(d->m_dref, &r, n, 1);
	}
}

string			xlua_dref_get_string(xlua_dref * d)
{
	if(d->m_ours){
		xlua_data_mutex.lock();
		string retVal=string(d->m_string_storage);
		xlua_data_mutex.unlock();
		return retVal;
	}
	
	if(d->m_types & xplmType_Data)
	{
		int l = XPLMGetDatab(d->m_dref, NULL, 0, 0);
		if(l > 0)
		{
			vector<char>	buf(l);
			l = XPLMGetDatab(d->m_dref, &buf[0], 0, l);
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
		xlua_data_mutex.lock();
		if(value!=d->m_string_storage)
			xlua_dref_changed(d);
		d->m_string_storage = value;
		xlua_data_mutex.unlock();
		return;
	}
	if(d->m_types & xplmType_Data)
	{
		const char * begin = value.c_str();
		const char * end = begin + value.size();
		if(end > begin)
		{
			XPLMSetDatab(d->m_dref, (void *) begin, 0, end - begin);
		}
	}
}
double	xtlua_dref_get_number(xtlua_dref * d)
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
		/*if(d->m_types & xplmType_IntArray)
		{
			int r;
			if(xtluaDefs.XTGetDatavi(d, &r, d->m_index, 1,d->m_ours))
				return r;
			return 0.0;
		}*/
		return 0.0;
	}
	/*if(d->m_types & xplmType_Double)
	{
		return xtluaDefs.XTGetDatad(d->m_dref,d->m_ours);
	}*/
	if(d->m_types & xplmType_Float||d->m_types & xplmType_Int||d->m_types & xplmType_Double)
	{
		return xtluaDefs.XTGetDataf(d,d->m_ours);
	}
	/*if(d->m_types & xplmType_Int)
	{
		return xtluaDefs.XTGetDatai(d->m_dref,d->m_ours);
	}*/
	return 0.0;
}

void			xtlua_dref_set_number(xtlua_dref * d, double value)
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
		xtluaDefs.XTSetDataf(d, value,d->m_ours);
	}
	/*if(d->m_types & xplmType_Int)
	{
		xtluaDefs.XTSetDatai(d->m_dref, value,d->m_ours);
	}*/
}

double			xtlua_dref_get_array(xtlua_dref * d, int n)
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

void			xtlua_dref_set_array(xtlua_dref * d, int n, double value)
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

string			xtlua_dref_get_string(xtlua_dref * d)
{
	/*if(d->m_ours){
		xlua_data_mutex.lock();
		string lVal=xlua_dref_get_string(d->local_dref);
		int l = lVal.size();
		if(l > 0)
		{
			vector<char>	buf(l);
			const char * charArray=lVal.c_str();
			for(int i=0;i<lVal.length()&&i<l;i++){
                    buf[i]=charArray[i];
			}
			xlua_data_mutex.unlock();
			return string(buf.begin(),buf.end());
		}
		xlua_data_mutex.unlock();
		return string();
	}*/
	//printf("get string %s %d\n",d->m_name.c_str(),d->m_types);
	if(d->m_types & xplmType_Data||d->m_name.rfind("xtlua/", 0) == 0)
	{
		int l = xtluaDefs.XTGetDatab(d, NULL, 0, 0,d->m_ours);
		//printf("get string local %d\n",l);
		if(l > 0)
		{
			vector<char>	buf(l);
			l = xtluaDefs.XTGetDatab(d, &buf[0], 0, l,d->m_ours);
			assert(l <= buf.size());
			//printf("get string local returned %d\n",l);
			if(l == buf.size())
			{
				string retVal=string(buf.begin(),buf.end());
				//printf("returning %s\n",retVal.c_str());
				return retVal;
			}
		}
	}
	return string();
}

void			xtlua_dref_set_string(xtlua_dref * d, const string& value)
{
	/*if(d->m_ours)
	{
		xlua_data_mutex.lock();
		int l = value.size();
		if(l > 0)
		{
			vector<char>	buf(l);
			const char * charArray=value.c_str();
			for(int i=0;i<value.length()&&i<l;i++){
                    buf[i]=charArray[i];
			}
			
			d->m_string_storage = string(buf.begin(),buf.end());
		}
		else
		{
			d->m_string_storage =string();
		}
		
		 
		xlua_data_mutex.unlock();
		//return;
	}*/
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
	xtluaDefs.paused_ref=NULL;
	for(xtlua_dref * d = s_drefs; d; d = d->m_next)
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
			//printf("registered: %s\n", d->m_name.c_str());
			XPLMSendMessageToPlugin(dre, MSG_ADD_DATAREF, (void *)d->m_name.c_str());		
		}		
#endif
	}
	for(xlua_dref * d = l_drefs; d; d = d->m_next)
	{
		if(d->m_dref == NULL)
		{
			assert(!d->m_ours);
			resolve_xp_dref(d);
		}
#if !MOBILE
		if(d->m_ours)
		if(dre != XPLM_NO_PLUGIN_ID)
		{
			//printf("registered: %s\n", d->m_name.c_str());
			XPLMSendMessageToPlugin(dre, MSG_ADD_DATAREF, (void *)d->m_name.c_str());		
		}		
#endif
	}
	active=true;
}

std::vector<XTCmd> runQueue;
std::vector<string> messageQueue;
std::mutex data_mutex;
static int xlua_std_pre_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref)
{

	xtlua_cmd * me = (xtlua_cmd *) ref;
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
	xtlua_cmd * me = (xtlua_cmd *) ref;
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
	xtlua_cmd * me = (xtlua_cmd *) ref;
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
void xtlua_localNavData(){
	data_mutex.lock();
	xtluaDefs.update_localNavData();//runs on xtlua thread, but lat/lon come from sim
	data_mutex.unlock();
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
	xtluaDefs.refreshAllDataRefs();
	data_mutex.lock();
	messageQueue.push_back(callout);
	data_mutex.unlock();
}
bool xlua_ispaused(){
	int retVal=xtluaDefs.isPaused;
	return retVal==1;
}
double xlua_get_simulated_time(){
	//printf("get sim time\n");
	data_mutex.lock();
	double retVal=xtluaDefs.XTGetElapsedTime();
	data_mutex.unlock();
	return retVal;
}
int xtlua_dref_resolveDREFQueue(){
	int retVal=xtluaDefs.resolveQueue();
	if(retVal>0){
		std::vector<xtlua_cmd*> commandstoHandle=xtluaDefs.XTGetHandlers();
		printf("have %d commands to handle registering\n",(int)commandstoHandle.size());
		for(xtlua_cmd* cmd: commandstoHandle){
			if(cmd->m_pre_handler){
				//printf("XPLMRegisterPreCommandHandler %s\n",cmd->m_name.c_str());
				XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_pre_handler, 1, cmd);
			}
			if(cmd->m_main_handler){
				//printf("XPLMRegisterCommandHandler %s\n",cmd->m_name.c_str());
				XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_main_handler, 1, cmd);
			}
			if(cmd->m_post_handler){
				//printf("XPLMRegisterPostCommandHandler %s\n",cmd->m_name.c_str());
				XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_post_handler, 0, cmd);
			}
		}
		retVal++;
	}
	return retVal;
}


void			xtlua_dref_cleanup()
{
	active=false;//stop writing to our local drefs
	while(s_drefs)
	{
		xtlua_dref *	kill = s_drefs;
		s_drefs = s_drefs->m_next;
		
		/*if(kill->m_dref && kill->m_ours)
		{
			XPLMUnregisterDataAccessor(kill->m_dref);
		}*/ //never
		
		delete kill;
	}
	xlua_dref * pdrefs=l_drefs;
	while(pdrefs)
	{
		xlua_dref *	kill = pdrefs;
		pdrefs = pdrefs->m_next;
	
		if(kill->m_ours)
		{
			if(kill->m_dref)
				XPLMUnregisterDataAccessor(kill->m_dref);	
		}

	}//try the old fashioned way
	while(l_drefs)
	{
		xlua_dref *	kill = l_drefs;
		l_drefs = l_drefs->m_next;
		
		//if(kill->m_dref && 
		if(kill->m_ours)
		{
			

			XPLMDataRef other = XPLMFindDataRef(kill->m_name.c_str());
			if(other)
				printf("Forcibly Unregistering %s\n",kill->m_name.c_str());
			int i=0;
			while(other&&i<4){
				XPLMUnregisterDataAccessor(other);
				other = XPLMFindDataRef(kill->m_name.c_str());
				i++;
			}
			
		}
		
		delete kill;
	}
	printf("XLua Cleanup\n");
	
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


static xtlua_cmd *		s_cmds = NULL;
static xlua_cmd *		l_cmds = NULL;

static void resolve_cmd(xtlua_cmd * d)
{
	xtluaDefs.XTqueueresolve_cmd(d);
}
xtlua_cmd * xtlua_find_cmd(const char * name)
{
	for(xtlua_cmd * i = s_cmds; i; i = i->m_next)
	if(i->m_name == name)
		return i;
		
	/*XPLMCommandRef c = XPLMFindCommand(name);	
	if(c == NULL){
		printf("ERROR: Command %s not found\n",name);
	} return NULL;*/	
		
	xtlua_cmd * nc = new xtlua_cmd;
	nc->m_next = s_cmds;
	s_cmds = nc;
	nc->m_name = name;
	resolve_cmd(nc);
	//nc->m_cmd = c;
	return nc;
}
xlua_cmd * xlua_find_cmd(const char * name)
{
	for(xlua_cmd * i = l_cmds; i; i = i->m_next)
	if(i->m_name == name)
		return i;
		
	XPLMCommandRef c = XPLMFindCommand(name);	
	if(c == NULL){
		printf("ERROR: Command %s not found\n",name);
	} 
	return NULL;	
		

}
static int xlua_null_main_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref){
	return 0;
}
xlua_cmd * xlua_create_cmd(const char * name, const char * desc)
{
	//printf("ERROR: xTLua cannot create command - %s - use xLua and wrap them here.\n",name);
	//return NULL;
	
	

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
	nc->m_next = l_cmds;
	l_cmds = nc;
	nc->m_name = name;
	nc->m_cmd = XPLMCreateCommand(name,desc);
	//printf("NULLCommandHandler %s\n",nc->m_name.c_str());
	
	return nc;
}

void xtlua_cmd_install_handler(xtlua_cmd * cmd, xtlua_cmd_handler_f handler, void * ref)
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
void xlua_cmd_install_handler(xlua_cmd * cmd, xlua_cmd_handler_f handler, void * ref)
{
	XPLMRegisterCommandHandler(cmd->m_cmd, xlua_null_main_handler, 1, cmd);
}

void xtlua_cmd_install_pre_wrapper(xtlua_cmd * cmd, xtlua_cmd_handler_f handler, void * ref)
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

void xtlua_cmd_install_post_wrapper(xtlua_cmd * cmd, xtlua_cmd_handler_f handler, void * ref)
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

void xtlua_cmd_start(xtlua_cmd * cmd)
{
	xtluaDefs.XTCommandBegin(cmd);
	//XPLMCommandBegin(cmd->m_cmd);
}
void xtlua_cmd_stop(xtlua_cmd * cmd)
{
	xtluaDefs.XTCommandEnd(cmd);
	//XPLMCommandEnd(cmd->m_cmd);
}

void xtlua_cmd_once(xtlua_cmd * cmd)
{
	xtluaDefs.XTCommandOnce(cmd);
	//XPLMCommandOnce(cmd->m_cmd);
}

void xtlua_cmd_cleanup()
{
	while(s_cmds)
	{
		xtlua_cmd * k = s_cmds;
		if(k->m_pre_handler)
			XPLMUnregisterCommandHandler(k->m_cmd, xlua_std_pre_handler, 1, k);
		if(k->m_main_handler)
			XPLMUnregisterCommandHandler(k->m_cmd, xlua_std_main_handler, 1, k);
		if(k->m_post_handler)
			XPLMUnregisterCommandHandler(k->m_cmd, xlua_std_post_handler, 0, k);
		s_cmds = s_cmds->m_next;
		delete k;
	}
	while(l_cmds)
	{
		xlua_cmd * k = l_cmds;
		
		l_cmds = l_cmds->m_next;
		delete k;
	}
}




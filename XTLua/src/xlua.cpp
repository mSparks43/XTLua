//	Copyright 2016, Laminar Research
//	This source code is licensed under the MIT open source license.
//	See LICENSE.txt for the full terms of the license.
// xTLua
// Modified by Mark Parker on 04/19/2020



#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <chrono>
#define XTVERSION "2.0.5"
#include <thread>
#ifndef XPLM200
#define XPLM200
#endif

#ifndef XPLM210
#define XPLM210
#endif

#include <XPLMPlugin.h>
#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>
#include <XPLMProcessing.h>

#include "module.h"
#include "xpdatarefs.h"
#include "xpcommands.h"
#include "xptimers.h"

using std::vector;

/*

	TODO: get good errors on compile error.
	TODO: pipe output somewhere useful.






 */

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#if !MOBILE
//static void *			g_alloc = NULL;
#endif
static vector<module *>g_modules; //modules in a thread
static vector<module *>xp_modules; //xp safe modules
static XPLMFlightLoopID	g_pre_loop = NULL;
static XPLMFlightLoopID	g_post_loop = NULL;
static int				g_is_acf_inited = 0;
XPLMDataRef				g_replay_active = NULL;
XPLMDataRef				g_sim_period = NULL;
int myID=0;
struct lua_alloc_request_t {
			void *	ud;
			void *	ptr;
			size_t	osize;
			size_t	nsize;
};

#define		ALLOC_OPEN		0x00A110C1
#define		ALLOC_REALLOC	0x00A110C2
#define		ALLOC_CLOSE		0x00A110C3
#define		ALLOC_LOCK		0x00A110C4
#define		ALLOC_UNLOCK	0x00A110C5



/*static void lua_lock()
{
	XPLMSendMessageToPlugin(XPLM_PLUGIN_XPLANE, ALLOC_LOCK, NULL);
}
static void lua_unlock()
{
	XPLMSendMessageToPlugin(XPLM_PLUGIN_XPLANE, ALLOC_UNLOCK, NULL);
}*/

/*static void *lj_alloc_create(void)
{
	struct lua_alloc_request_t r = { 0 };
	XPLMSendMessageToPlugin(XPLM_PLUGIN_XPLANE, ALLOC_OPEN,&r);
	return r.ud;	
}

static void  lj_alloc_destroy(void *msp)
{
	struct lua_alloc_request_t r = { 0 };
	r.ud = msp;
	XPLMSendMessageToPlugin(XPLM_PLUGIN_XPLANE, ALLOC_CLOSE,&r);
}

static void *lj_alloc_f(void *msp, void *ptr, size_t osize, size_t nsize)
{
	struct lua_alloc_request_t r = { 0 };
	r.ud = msp;
	r.ptr = ptr;
	r.osize = osize;
	r.nsize = nsize;
	XPLMSendMessageToPlugin(XPLM_PLUGIN_XPLANE, ALLOC_REALLOC,&r);
	return r.ptr;
}*/
bool ready=false;
bool loadedModules=false;
bool liveThread=false;
bool run=true;
bool active=false;
bool dirtyXTScripts=false;
bool sleeping=false;
static float xlua_pre_timer_master_cb(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon)
{
	xlua_do_timers_for_time(xlua_get_simulated_time(),xlua_ispaused());
	
	/*if(XPLMGetDatai(g_replay_active) == 0)
	if(XPLMGetDataf(g_sim_period) > 0.0f)	
	for(vector<module *>::iterator m = g_modules.begin(); m != g_modules.end(); ++m)	
		(*m)->pre_physics();*/
	if(loadedModules&&xtlua_dref_resolveDREFQueue()==0&&!ready){
		ready=true;
		printf("x(t)lua set ready\n");
	}
	if(ready)
		xtlua_dref_preUpdate();
	return -1;
}

static float xlua_post_timer_master_cb(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon)
{
	if(XPLMGetDatai(g_replay_active) == 0)
	{
		if(XPLMGetDataf(g_sim_period) > 0.0f)
		for(vector<module *>::iterator m = xp_modules.begin(); m != xp_modules.end(); ++m)		
			(*m)->post_physics();
	}
	/*else
	for(vector<module *>::iterator m = g_modules.begin(); m != g_modules.end(); ++m)		
		(*m)->post_replay();*/
	
	xtlua_dref_postUpdate();

	liveThread=true;
	return -1;
}
std::vector<string> script_paths;
std::vector<string> mod_paths;
string init_script_path;
string plugin_base_path;
void registerFlightLoop(){
	XPLMCreateFlightLoop_t pre = { 0 };
	XPLMCreateFlightLoop_t post = { 0 };
	pre.structSize = sizeof(pre);
	post.structSize = sizeof(post);
	pre.phase = xplm_FlightLoop_Phase_BeforeFlightModel;
	post.phase = xplm_FlightLoop_Phase_AfterFlightModel;
	pre.callbackFunc = xlua_pre_timer_master_cb;
	post.callbackFunc = xlua_post_timer_master_cb;

	g_pre_loop = XPLMCreateFlightLoop(&pre);
	g_post_loop = XPLMCreateFlightLoop(&post);
	XPLMScheduleFlightLoop(g_pre_loop, -1, 0);
	XPLMScheduleFlightLoop(g_post_loop, -1, 0);
}
static void findXTScripts(){
	string scripts_dir_path(plugin_base_path);
	
	//begin xtlua
	init_script_path=plugin_base_path;
	
	init_script_path += "init.lua";
	scripts_dir_path=plugin_base_path;
	
	scripts_dir_path += "scripts";

	int offset = 0;
	int mf, fcount;
	while(1)
	{
		char fname_buf[2048];
		char * fptr;
		XPLMGetDirectoryContents(
								scripts_dir_path.c_str(),
								offset,
								fname_buf,
								sizeof(fname_buf),
								&fptr,
								1,
								&mf,
								&fcount);
		if(fcount == 0)
			break;
		
		if(strcmp(fptr, ".DS_Store") != 0)
		{		
			string mod_path(scripts_dir_path);
			mod_path += "/";
			mod_path += fptr;
			mod_path += "/";
			mod_paths.push_back(mod_path);
			string script_path(mod_path);
			script_path += fptr;
			script_path += ".lua";

			script_paths.push_back(script_path);
		}
			
		++offset;
		if(offset == mf)
			break;
	}

	dirtyXTScripts=true;
}
static void loadXTScripts(){
	printf("begin loading scripts %d\n",myID);
	printf("%s\n",init_script_path.c_str());

	
	for(int i=0;i<script_paths.size();i++)
	{
		
			printf(" loading %s\n",script_paths[i].c_str());
#if !MOBILE
			g_modules.push_back(new module(
							mod_paths[i].c_str(),
							init_script_path.c_str(),
							script_paths[i].c_str(),
							true));
#else
			g_modules.push_back(new module(
				mod_paths[i].c_str(),
				init_script_path.c_str(),
				script_paths[i].c_str(),
				lj_alloc_f,
				NULL));
#endif
		
	}
	dirtyXTScripts=false;
}
static void loadXPScripts(){
	string scripts_dir_path(plugin_base_path);
	init_script_path=plugin_base_path;
	init_script_path += "init/init.lua";
	scripts_dir_path += "init/scripts";
	int offset = 0;
	int mf, fcount;
	while(1)
	{
		char fname_buf[2048];
		char * fptr;
		XPLMGetDirectoryContents(
								scripts_dir_path.c_str(),
								offset,
								fname_buf,
								sizeof(fname_buf),
								&fptr,
								1,
								&mf,
								&fcount);
		if(fcount == 0)
			break;
		
		if(strcmp(fptr, ".DS_Store") != 0)
		{		
			string mod_path(scripts_dir_path);
			mod_path += "/";
			mod_path += fptr;
			mod_path += "/";
			string script_path(mod_path);
			script_path += fptr;
			script_path += ".lua";
			xp_modules.push_back(new module(
							mod_path.c_str(),
							init_script_path.c_str(),
							script_path.c_str(),
							false));
			//printf("Do init script %s\n",script_path.c_str());				

		}
			
		++offset;
		if(offset == mf)
			break;
	}
}

static void cleanupScripts(){
	for(vector<module *>::iterator m = xp_modules.begin(); m != xp_modules.end(); ++m)
		delete (*m);
	xp_modules.clear();
	for(vector<module *>::iterator m = g_modules.begin(); m != g_modules.end(); ++m)
		delete (*m);
	g_modules.clear();
	script_paths.clear();
	
	if(g_pre_loop!=NULL)
	XPLMDestroyFlightLoop(g_pre_loop);
	if(g_post_loop!=NULL)
	XPLMDestroyFlightLoop(g_post_loop);
	g_pre_loop = NULL;
	g_post_loop = NULL;	
	g_is_acf_inited = 0;
	xtlua_dref_cleanup();
	xtlua_cmd_cleanup();
	xtlua_timer_cleanup();
}

int reloadScripts(XPLMCommandRef c, XPLMCommandPhase phase, void * ref){
	if(phase ==0){
		//pause XT thread
		printf("XTLua going to sleep for scripts reload\n");
		active=false;
		while(!sleeping)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		printf("XTLua sleeping for scripts reload\n");
		loadedModules=false;
		ready=false;
		//clean everything up
		/*XPLMFlightLoopID	l_g_pre_loop = g_pre_loop;
		XPLMFlightLoopID	l_g_post_loop = g_post_loop;
		g_pre_loop = NULL;
		g_post_loop = NULL;*/
		cleanupScripts();
		/*g_pre_loop = l_g_pre_loop;
		g_post_loop = l_g_post_loop;*/
		//registerFlightLoop();
		//load XP Scripts
		loadXPScripts();
		xlua_relink_all_drefs();
		//find XT scripts
		findXTScripts();
		
		while(dirtyXTScripts)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//init scripts
		
		
		//xlua_relink_all_drefs();
		//active=true;
		//xtlua_dref_resolveDREFQueue();
		xlua_add_callout("flight_start");
		xlua_add_callout("aircraft_load");
		xlua_setLoadStatus(1);
		registerFlightLoop();
		active=true;
		printf("XLua active with new scripts\n");
		
	}
	
	return 0;
}
void do_during_physics(){
	while(myID==0){
		std::this_thread::sleep_for(std::chrono::seconds(1));
		printf("during_physics thread sleeping %d\n",myID);
	}
	printf("during_physics thread open %d\n",myID);
	while(!liveThread&&run){
		std::this_thread::sleep_for(std::chrono::seconds(1));
		printf("during_physics thread sleeping %d\n",myID);
		sleeping=true;
	}
	sleeping=false; 
	printf("during_physics thread woke up %d\n",myID);
	loadXTScripts();
	
	loadedModules=true;
	while(liveThread&&run&&!ready){
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	while(liveThread&&run){
		bool waitSleep=false;

		if(active&&!dirtyXTScripts){
			sleeping=false;
			auto start = std::chrono::high_resolution_clock::now();
			std::vector<XTCmd> runItems=get_runQueue();
			for(XTCmd item:runItems){
				item.runFunc(item.xluaref, item.phase, item.duration, item.m_func_ref);
			}
			xtlua_do_timers_for_time(xlua_get_simulated_time(),xlua_ispaused());
			std::vector<string> msgItems=get_runMessages();
			waitSleep=false;
			for(string item:msgItems){
					printf("XTLua:do threaded callout %s\n",item.c_str());
					waitSleep=true;
					for(vector<module *>::iterator m = g_modules.begin(); m != g_modules.end(); ++m)
						(*m)->do_callout(item.c_str());
				}
			if(!xlua_ispaused()){
				for(vector<module *>::iterator m = g_modules.begin(); m != g_modules.end(); ++m)		
					(*m)->pre_physics();
					
				for(vector<module *>::iterator m = g_modules.begin(); m != g_modules.end(); ++m)		
					(*m)->post_physics();
			}
			xtlua_localNavData();
			auto finish = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> elapsed = finish - start;
			int diff=round(elapsed.count());
			if(diff<20)
				std::this_thread::sleep_for(std::chrono::milliseconds(20-diff));//100fps or less
			else if(diff>30)
			{
				printf("warn: xtlua time overflow!=%d\n",diff);
			}
				
		}
		else{
			sleeping=true;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if(dirtyXTScripts){
				printf("XTLua:do Load Scripts\n");
				loadXTScripts();
				loadedModules=true;
				printf("XTLua:Load Scripts complete, waiting for ready signal\n");
				while(liveThread&&run&&!ready){
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				printf("XTLua:exit sleep\n");
				/*while(liveThread&&run){
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}*/
			}
		}
	}
	
	if(g_is_acf_inited)
	{
		for(vector<module *>::iterator m = g_modules.begin(); m != g_modules.end(); ++m)
			(*m)->acf_unload();
		g_is_acf_inited = 0;
	}
	sleeping=true;
	
    //sprintf(gBob_debstr2,"simulation thread stopped\n");
    //XPLMDebugString(gBob_debstr2);
	printf("XTLua:new during_physics thread stopped %d\n",myID);
}
//std::thread m_thread;
std::vector<std::thread> threads;

char * thisSig;
int XTLuaXPluginStart(char * outSig)
{

	ready=false;
	loadedModules=false;
	liveThread=false;
	run=true;
	active=false;
	dirtyXTScripts=false;
	sleeping=false;
	g_is_acf_inited = 0;
	g_replay_active = XPLMFindDataRef("sim/time/is_in_replay");
	g_sim_period = XPLMFindDataRef("sim/operation/misc/frame_rate_period");
	myID=XPLMGetMyID();

	
	registerFlightLoop();
	
	XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);
	
	char path_to_me_c[2048];
	XPLMGetPluginInfo(XPLMGetMyID(), NULL, path_to_me_c, NULL, NULL);
	
	// Plugin base path: pop off two dirs from the plugin name to get the base path.
	plugin_base_path=string(path_to_me_c);
	string::size_type lp = plugin_base_path.find_last_of("/\\");
	plugin_base_path.erase(lp);
	lp = plugin_base_path.find_last_of("/\\");
	plugin_base_path.erase(lp+1);
	//strcpy(outSig, "com.x-plane.xtlua." VERSION);
	if(outSig!=NULL)
		sprintf(outSig,"com.x-plane.xtlua.%s.%s",plugin_base_path.c_str(),XTVERSION);
	
	thisSig=outSig;
	//do create datarefs on thread
	loadXPScripts();
	
	findXTScripts();
	//reload_cmd=XPLMCreateCommand("xtlua/reloadScripts","Reload all xtlua scripts");
	//XPLMRegisterCommandHandler(reload_cmd, reloadScripts, 1,  (void *)0);
	printf("XTLua XTLuaXPluginStart %s\n",outSig);
	threads.push_back(std::thread(do_during_physics));
	
	return 1;
}

void	XTLuaXPluginStop(void)
{
	run=false;
	for (auto& th : threads)
		if(th.joinable())
			th.join();
	g_is_acf_inited = 0;
	cleanupScripts();
	printf("XTLua XTLuaXPluginStop %d\n",myID);
}

void XTLuaXPluginDisable(void)
{
	printf("XTLua going to sleep in XTLuaXPluginDisable\n");
	active=false;
	while(!sleeping&&liveThread&&run){
		//xtlua_dref_preUpdate();
		//xtlua_dref_postUpdate();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//xtlua_dref_postUpdate();//make double sure all calls are applied, race, but meh.
	}
	printf("XTLua sleeping\n");
}

int XTLuaXPluginEnable(void)
{
	printf("XTLua active %d\n", XPLMGetMyID());
	
	xlua_relink_all_drefs();
	active=true;
	return 1;
}

void XTLuaXPluginReceiveMessage(
					XPLMPluginID	inFromWho,
					int				inMessage,
					void *			inParam)
{
	//printf("XPLM_MSG_PLANE %d\n",inMessage);
	if(inFromWho != XPLM_PLUGIN_XPLANE)
		return;
	//printf("XPLM_MSG %d\n",inMessage);	
	switch(inMessage) {
	case XPLM_MSG_LIVERY_LOADED:
		xlua_add_callout("livery_load");
		break;
	case XPLM_MSG_PLANE_LOADED:
		if(inParam == 0)
			g_is_acf_inited = 0;
		xlua_relink_all_drefs();
		xlua_validate_drefs();
		xlua_setLoadStatus(1);	
		xlua_add_callout("aircraft_load");
		//printf("XPLM_MSG_PLANE_LOADED\n");
		break;
	case XPLM_MSG_PLANE_UNLOADED:
		if(g_is_acf_inited){
			//xlua_add_callout("aircraft_unload");
			for(vector<module *>::iterator m = xp_modules.begin(); m != xp_modules.end(); ++m)		
				(*m)->acf_unload();

		}
		xlua_setLoadStatus(0);
		g_is_acf_inited = 0;
		//printf("XPLM_MSG_PLANE_UNLOADED\n");
		break;
	case XPLM_MSG_AIRPORT_LOADED:
		if(!g_is_acf_inited)
		{
			// Pick up any last stragglers from out-of-order load and then validate our datarefs!
			xlua_relink_all_drefs();
			xlua_validate_drefs();
			xlua_setLoadStatus(1);
			xlua_add_callout("aircraft_load");
			for(vector<module *>::iterator m = xp_modules.begin(); m != xp_modules.end(); ++m)
				(*m)->acf_load();
			g_is_acf_inited = 1;							
		}
		
		xlua_add_callout("flight_start");
		//printf("XPLM_MSG_AIRPORT_LOADED\n");
		for(vector<module *>::iterator m = xp_modules.begin(); m != xp_modules.end(); ++m)
			(*m)->flight_init();
		break;
	case XPLM_MSG_PLANE_CRASHED:
		//assert(g_is_acf_inited);
		xlua_add_callout("flight_crash");
		//printf("XPLM_MSG_PLANE_CRASHED\n");
		for(vector<module *>::iterator m = xp_modules.begin(); m != xp_modules.end(); ++m)
			(*m)->flight_crash();		
		break;
	}
}


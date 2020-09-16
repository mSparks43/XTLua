//
//  xpmtdatarefs.h
//  xTLua
//
//  Created by Mark Parker on 04/19/2020
//
//	Copyright 2020
//	This source code is licensed under the MIT open source license.
//	See LICENSE.txt for the full terms of the license.
#include <XPLMDataAccess.h>
#include <unordered_map>
#define XPLM200 1
#include "xpcommands.h"
#include "xpmtdatatypes.h"
#include <XPLMProcessing.h> //XPLMGetElapsedTime
#include <XPLMUtilities.h>
#include <string>
#include <mutex>
#include "xpdatarefs.h"
struct	xtlua_dref {
	xtlua_dref *				m_next;
	std::string				m_name;
	XPLMDataRef				m_dref;
	int						m_index;	// -1 if index is NOT bound.
	XPLMDataTypeID			m_types;
	int						m_ours;		// 1 if we made, 0 if system
    xlua_dref *				local_dref;
	//xtlua_dref_notify_f		m_notify_func;
	//void *					m_notify_ref;
	
	// IF we made the dataref, this is where our storage is!
	//double					m_number_storage;
	//vector<double>			m_array_storage;
	std::string					m_string_storage;
};

struct xtlua_cmd {
	xtlua_cmd() : m_next(NULL),m_cmd(NULL),m_ours(0),
		m_pre_handler(NULL),m_pre_ref(NULL),
		m_main_handler(NULL),m_main_ref(NULL),
		m_post_handler(NULL),m_post_ref(NULL) { }

	xtlua_cmd *			m_next;
	std::string				m_name;
	XPLMCommandRef		m_cmd;
	int					m_ours;
	xtlua_cmd_handler_f	m_pre_handler;
	void *				m_pre_ref;
	xtlua_cmd_handler_f	m_main_handler;
	void *				m_main_ref;
	xtlua_cmd_handler_f	m_post_handler;
	void *				m_post_ref;
	float				m_down_time;
};
struct xlua_cmd {
	xlua_cmd() : m_next(NULL),m_cmd(NULL),
    m_main_handler(NULL),m_main_ref(NULL) { }

	xlua_cmd *			m_next;
	std::string				m_name;
	XPLMCommandRef		m_cmd;
    xtlua_cmd_handler_f	m_main_handler;
	void *				m_main_ref;

};
/*static int xlua_std_pre_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref);
static int xlua_std_main_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref);
static int xlua_std_post_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref);*/
class NavAid
{
    public:
    int    id;
    int    type;
    float  latitude;    /* Can be NULL */
    float  longitude;  
    int    frequency;
    float  heading;
    std::string name;
    std::string ident;
    NavAid * next;
};
class XTLuaDataRefs
{
private:
    std::unordered_map<std::string,std::vector<XTLuaArrayFloat*> > floatdataRefs;
    std::unordered_map<std::string,std::vector<XTLuaArrayFloat*> > changeddataRefs;
    //std::unordered_map<std::string, XTLuaDouble> doubledataRefs;
    //std::unordered_map<std::string, XTLuaInteger> intdataRefs;
    //std::unordered_map<std::string, XTLuaChars> stringdataRefs;
    std::unordered_map<std::string, XTLuaCharArray*> stringdataRefs;
    std::vector<xtlua_dref*> drefResolveQueue;
    std::vector<xtlua_cmd*> cmdResolveQueue;
    std::unordered_map<std::string, xtlua_cmd*> cmdHandlerResolveQueue;
    std::unordered_map<int, NavAid*> localNavaids;
    std::string incomingNavaidString;
    std::string localNavaidString;
    std::string incomingFMSString;
    std::string localFMSString;
    //std::unordered_map<std::string, XTCmd> startCmds;
    //std::unordered_map<std::string, XTCmd> stopCmds;
    //std::unordered_map<std::string, XTCmd> fireCmds;
    std::vector<XTCmd*> commandQueue;
    std::vector<XTCmd> runQueue;
    double timeT=0;
    NavAid * navaids=NULL;
    NavAid * lastnavaid=NULL;
    XPLMDataRef latR;
    XPLMDataRef lonR;
    double lat;
    double lon;
    NavAid * current_navaid=NULL;
    bool skipNaviads=true;
    int updateRoll=0;
public:
    int isPaused=true;
    void *paused_ref=NULL;
    void XTCommandBegin(xtlua_cmd * cmd);
    void XTCommandEnd(xtlua_cmd * cmd);
    void XTCommandOnce(xtlua_cmd * cmd);
    void XTRegisterCommandHandler(xtlua_cmd * cmd);
    double XTGetElapsedTime();
    //void ShowDataRefs();
    void updateDataRefs();
    void refreshAllDataRefs();
    void updateStringDataRefs();
    void updateFloatDataRefs();
    void updateNavDataRefs();
    void update_localNavData();
    void addNavData(int    id,
        int    type,
        float  latitude,
        float  longitude, 
        int    frequency,
        float  heading,
        char * name,
        char * ident);
    void updateCommands();
    void cleanup();
    void                 XTqueueresolve_dref(xtlua_dref * d);//can be called from anywhere
    void                 XTqueueresolve_cmd(xtlua_cmd * d);//can be called from anywhere
    int                 resolveQueue();//only to be called from flight loop thread
    std::vector<xtlua_cmd*> XTGetHandlers();
    int                  XTGetDatavf(
                                   xtlua_dref * d,    
                                   float *              outValues,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMax,bool local);
    void                 XTSetDatavf(
                                   xtlua_dref * d,    
                                   float              inValue,    
                                   int                  index);                               
                                  
    float                XTGetDataf(
                                   xtlua_dref * d,bool local);
    void                 XTSetDataf(
                                   xtlua_dref * d,    
                                   float                inValue,bool local);    
    
    
    int                  XTGetDatab(
                                   xtlua_dref * d,    
                                   void *               outValue,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMaxBytes,bool local);
    void                 XTSetDatab(
                                   xtlua_dref * d,    
                                   std::string value);                                                             
};

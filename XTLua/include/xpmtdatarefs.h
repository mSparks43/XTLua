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
struct	xlua_dref {
	xlua_dref *				m_next;
	std::string				m_name;
	XPLMDataRef				m_dref;
	int						m_index;	// -1 if index is NOT bound.
	XPLMDataTypeID			m_types;
	int						m_ours;		// 1 if we made, 0 if system
	//xlua_dref_notify_f		m_notify_func;
	//void *					m_notify_ref;
	
	// IF we made the dataref, this is where our storage is!
	//double					m_number_storage;
	//vector<double>			m_array_storage;
	std::string					m_string_storage;
};

struct xlua_cmd {
	xlua_cmd() : m_next(NULL),m_cmd(NULL),m_ours(0),
		m_pre_handler(NULL),m_pre_ref(NULL),
		m_main_handler(NULL),m_main_ref(NULL),
		m_post_handler(NULL),m_post_ref(NULL) { }

	xlua_cmd *			m_next;
	std::string				m_name;
	XPLMCommandRef		m_cmd;
	int					m_ours;
	xlua_cmd_handler_f	m_pre_handler;
	void *				m_pre_ref;
	xlua_cmd_handler_f	m_main_handler;
	void *				m_main_ref;
	xlua_cmd_handler_f	m_post_handler;
	void *				m_post_ref;
	float				m_down_time;
};

/*static int xlua_std_pre_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref);
static int xlua_std_main_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref);
static int xlua_std_post_handler(XPLMCommandRef c, XPLMCommandPhase phase, void * ref);*/

class XTLuaDataRefs
{
private:
    std::unordered_map<std::string,std::vector<XTLuaArrayFloat*>> floatdataRefs;
    //std::unordered_map<std::string, XTLuaDouble> doubledataRefs;
    //std::unordered_map<std::string, XTLuaInteger> intdataRefs;
    //std::unordered_map<std::string, XTLuaChars> stringdataRefs;
    std::unordered_map<std::string, XTLuaCharArray*> stringdataRefs;
    std::vector<xlua_dref*> drefResolveQueue;
    std::vector<xlua_cmd*> cmdResolveQueue;
    std::unordered_map<std::string, xlua_cmd*> cmdHandlerResolveQueue;
    std::unordered_map<std::string, XTCmd> startCmds;
    std::unordered_map<std::string, XTCmd> stopCmds;
    std::unordered_map<std::string, XTCmd> fireCmds;
    std::vector<XTCmd> commandQueue;
    std::vector<XTCmd> runQueue;
    double timeT=0;
    int updateRoll=0;
public:

    void XTCommandBegin(xlua_cmd * cmd);
    void XTCommandEnd(xlua_cmd * cmd);
    void XTCommandOnce(xlua_cmd * cmd);
    void XTRegisterCommandHandler(xlua_cmd * cmd);
    double XTGetElapsedTime();
    //void ShowDataRefs();
    void updateDataRefs();
    void updateStringDataRefs();
    void updateFloatDataRefs();
    void updateCommands();
    void cleanup();
    void                 XTqueueresolve_dref(xlua_dref * d);//can be called from anywhere
    void                 XTqueueresolve_cmd(xlua_cmd * d);//can be called from anywhere
    int                 resolveQueue();//only to be called from flight loop thread
    std::vector<xlua_cmd*> XTGetHandlers();
    int                  XTGetDatavf(
                                   xlua_dref * d,    
                                   float *              outValues,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMax,bool local);
    void                 XTSetDatavf(
                                   xlua_dref * d,    
                                   float              inValue,    
                                   int                  index);                               
    int                  XTGetDatavi(
                                   XPLMDataRef          inDataRef,    
                                   int *                outValues,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMax,bool local);
    void                 XTSetDatavi(
                                   XPLMDataRef          inDataRef,    
                                   int *                inValues,    
                                   int                  inoffset,    
                                   int                  inCount,bool local);                                 
    float                XTGetDataf(
                                   XPLMDataRef          inDataRef,bool local);
    void                 XTSetDataf(
                                   XPLMDataRef          inDataRef,    
                                   float                inValue,bool local);    
    double               XTGetDatad(
                                   XPLMDataRef          inDataRef,bool local); 
    void                 XTSetDatad(
                                   XPLMDataRef          inDataRef,    
                                   double               inValue,bool local); 
    int                  XTGetDatai(
                                   XPLMDataRef          inDataRef,bool local);    

    void                 XTSetDatai(
                                   XPLMDataRef          inDataRef,    
                                   int                  inValue,bool local); 
    int                  XTGetDatab(
                                   xlua_dref * d,    
                                   void *               outValue,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMaxBytes,bool local);
    void                 XTSetDatab(
                                   xlua_dref * d,    
                                   std::string value);                                                             
};
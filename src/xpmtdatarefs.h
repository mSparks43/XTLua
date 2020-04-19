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
#include "xpmtdatatypes.h"
#include <string>
#include <mutex>
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
class XTLuaDataRefs
{
private:
    std::unordered_map<std::string, XTLuaFloat> floatdataRefs;
    std::unordered_map<std::string, XTLuaDouble> doubledataRefs;
    std::unordered_map<std::string, XTLuaInteger> intdataRefs;
    std::unordered_map<std::string, XTLuaChars> stringdataRefs;
    std::vector<xlua_dref*> drefResolveQueue;
    
public:
    void ShowDataRefs();
    void updateDataRefs();
    void                 XTqueueresolve_dref(xlua_dref * d);//can be called from anywhere
    int                 resolveQueue();//only to be called from flight loop thread
    int                  XTGetDatavf(
                                   XPLMDataRef          inDataRef,    
                                   float *              outValues,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMax,bool local);
    void                 XTSetDatavf(
                                   XPLMDataRef          inDataRef,    
                                   float *              inValues,    
                                   int                  inoffset,    
                                   int                  inCount,bool local);                               
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
                                   XPLMDataRef          inDataRef,    
                                   void *               outValue,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMaxBytes,bool local);
    void                 XTSetDatab(
                                   XPLMDataRef          inDataRef,    
                                   void *               inValue,    
                                   int                  inOffset,    
                                   int                  inLength,bool local);                                                             
};
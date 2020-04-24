//
//  xpmtdatarefs.cpp
//  xTLua
//
//  Created by Mark Parker on 04/19/2020
//
//	Copyright 2020
//	This source code is licensed under the MIT open source license.
//	See LICENSE.txt for the full terms of the license.

#include <XPLMDataAccess.h>
#include "xpmtdatarefs.h"
#include <stdio.h>
#include <assert.h>

static std::mutex data_mutex;
void XTLuaDataRefs::XTPreCMD(xlua_cmd * cmd,int phase){
    //printf("Pre command %s\n",cmd.m_name.c_str());
    data_mutex.lock();
    /*char namec[32]={0};
    sprintf(namec,"%p",cmd);
    std::string name=namec;*/

    data_mutex.unlock();
}
void XTLuaDataRefs::XTCommandBegin(xlua_cmd * cmd){
    data_mutex.lock();
    printf("Command start %s\n",cmd->m_name.c_str());

    data_mutex.unlock();
}

void XTLuaDataRefs::XTCommandEnd(xlua_cmd * cmd){
    data_mutex.lock();
    printf("Command stop %s\n",cmd->m_name.c_str());

    data_mutex.unlock();
}

void XTLuaDataRefs::XTCommandOnce(xlua_cmd * cmd){
    data_mutex.lock(); 
    char namec[32]={0};
    sprintf(namec,"%p",cmd);
    std::string name=namec;
                                     
    if(fireCmds.find(name)==fireCmds.end()){
        //printf("Command once %s\n",cmd->m_name.c_str());
       
        XTCmd xtcmd;
        xtcmd.xluaref=cmd;
        xtcmd.fire=true;
        commandQueue.push_back(xtcmd);
        fireCmds[name]=xtcmd;
    }
    else
    {
        printf("skip Command once %s - in queue\n",cmd->m_name.c_str());
    }    
    data_mutex.unlock();
}


void XTLuaDataRefs::XTRegisterCommandHandler(xlua_cmd * cmd){
    char namec[32]={0};
    sprintf(namec,"%p",cmd);
    std::string name=namec;
    //use pointer name to only register each handler once
    printf("Will register command handlers for %s\n",cmd->m_name.c_str());
    data_mutex.lock(); 
    cmdHandlerResolveQueue[name]=cmd;
    data_mutex.unlock();

}
float XTLuaDataRefs::XTGetElapsedTime(){
    return time;
}
//begin DataRefs section
void XTLuaDataRefs::updateDataRefs(){
    data_mutex.lock();
    time = XPLMGetElapsedTime();
    std::unordered_map<std::string, XTLuaFloat> incomingFloatdataRefs;
    std::unordered_map<std::string, XTLuaChars> incomingStringdataRefs;
    for (auto x : floatdataRefs) {
        int i=0;
        XTLuaFloat val=x.second;
        if(!val.isArray){
            float v=val.values[0];
            if(val.get&&!val.set){

                
                float newVal=XPLMGetDataf(val.ref);
                //printf("get float %s[%d] = %f(%d) was %f\n",x.first.c_str(),i,newVal,x.second.values.size(),v);
                
                val.get=false;
                val.values[0]=newVal;
                incomingFloatdataRefs[x.first]=val;
                //x.second.values[0]=XPLMGetDataf(x.second.ref);
                //floatdataRefs[x.first]=x.second;
            }
            else if(val.set){ 
                //printf("set float %p %s[%d] = %f(%d)\n",x.second.ref,x.first.c_str(),i,v,x.second.values.size());

                XPLMSetDataf(x.second.ref,v);
                XTLuaFloat val=x.second;
                val.set=false;
                incomingFloatdataRefs[x.first]=val;
            }
        }
        else{
            if(val.get&&!val.set){
                float inVals[val.values.size()];
                int retVal=XPLMGetDatavf(val.ref,inVals,0,val.values.size());
                for(int i=0;i<retVal;i++){
                    float v=val.values[i];
                    //printf("get float %s[%d] = %f(%d) was %f\n",x.first.c_str(),i,inVals[i],x.second.values.size(),v);
                    val.get=false;
                    val.values[v]=inVals[i];
                }
                incomingFloatdataRefs[x.first]=val;
            }
            else if(val.set){ 
                float outVals[val.values.size()];
                for(int i=0;i<val.values.size();i++){
                    outVals[i]=val.values[i];
                    float v=val.values[i];
                    //printf("set float %p %s[%d] = %f(%d)\n",x.second.ref,x.first.c_str(),i,v,x.second.values.size());
                }
                XPLMSetDatavf(val.ref,outVals,0,val.values.size());
                XTLuaFloat val=x.second;
                val.set=false;
                incomingFloatdataRefs[x.first]=val;
            }
        }
    }
    for (auto x : stringdataRefs) {
        int i=0;
        XTLuaChars val=x.second;
        
        if(val.get&&!val.set){
           char inVals[val.values.size()];
           int retVal=XPLMGetDatab(val.ref,inVals,0,val.values.size());
           for(int i=0;i<retVal;i++){
                char v=val.values[i];
                //printf("get char %s[%d] = %s(%d) was %f\n",x.first.c_str(),i,inVals[i],x.second.values.size(),v);
                val.get=false;
                val.values[v]=inVals[i];
            }
            incomingStringdataRefs[x.first]=val;
        }
        else if(val.set){ 
            char outVals[val.values.size()];
            for(int i=0;i<val.values.size();i++){
                outVals[i]=val.values[i];
                char v=val.values[i];
                //printf("set char %p %s[%d] = %s(%d)\n",x.second.ref,x.first.c_str(),i,v,x.second.values.size());
            }
            XPLMSetDatab(val.ref,outVals,0,val.values.size());
            XTLuaChars val=x.second;
            val.set=false;
            incomingStringdataRefs[x.first]=val;
        }
        
    }
    for (auto x : incomingFloatdataRefs) {
        XTLuaFloat val=x.second;
        floatdataRefs[x.first]=val;
    }
    for (auto x : incomingStringdataRefs) {
        XTLuaChars val=x.second;
        stringdataRefs[x.first]=val;
    }
    for(XTCmd c:commandQueue){
        xlua_cmd* cmd=c.xluaref;
        if(c.fire){
            //printf("Do Command once %s %p\n",cmd->m_name.c_str(),cmd->m_cmd);
            XPLMCommandOnce(cmd->m_cmd);
        }
    }
    commandQueue.clear();
    fireCmds.clear();
    data_mutex.unlock();
}
void XTLuaDataRefs::ShowDataRefs(){
    data_mutex.lock();
    for (auto x : floatdataRefs) {
        int i=0;
        for(float v:x.second.values){
            if(x.second.get&&!x.second.set)
                printf("get float %s[%d] = %f(%d)\n",x.first.c_str(),i,v,x.second.values.size());
            if(x.second.set) 
                printf("set float %s[%d] = %f(%d)\n",x.first.c_str(),i,v,x.second.values.size());
            i++;
        }
    }
    for (auto x : doubledataRefs) {
        int i=0;
        for(double v:x.second.values){
            if(x.second.get&&!x.second.set)
                printf("get double %s[%d] = %f(%d)\n",x.first.c_str(),i,v,x.second.values.size());
            if(x.second.set)     
                printf("set double %s[%d] = %f(%d)\n",x.first.c_str(),i,v,x.second.values.size());
            i++;
        }
    }
    for (auto x : intdataRefs) {
        int i=0;
        for(int v:x.second.values){
            if(x.second.get&&!x.second.set)
                printf("get int %s[%d] = %d(%d)\n",x.first.c_str(),i,v,x.second.values.size());
            if(x.second.set)
                printf("set int %s[%d] = %d(%d)\n",x.first.c_str(),i,v,x.second.values.size());       
            i++;
        }
    }
    data_mutex.unlock();
}

void XTLuaDataRefs::XTqueueresolve_dref(xlua_dref * d){
    drefResolveQueue.push_back(d);//this needs to be done on another thread
}
void XTLuaDataRefs::XTqueueresolve_cmd(xlua_cmd * d){
    cmdResolveQueue.push_back(d);//this needs to be done on another thread
}
 



int XTLuaDataRefs::resolveQueue(){
    int retVal=0;
    for(xlua_dref * d:drefResolveQueue){
        assert(d->m_dref == NULL);
        assert(d->m_types == 0);
        assert(d->m_index == -1);
        assert(d->m_ours == 0);
        d->m_dref = XPLMFindDataRef(d->m_name.c_str());
        
        if(d->m_dref)
        {
            d->m_index = -1;
            d->m_types = XPLMGetDataRefTypes(d->m_dref);
            printf("Resolved dref %s to %p\n",d->m_name.c_str(),d->m_dref);
            retVal++;
        }
        else
        {
            std::string::size_type obrace = d->m_name.find('[');
            std::string::size_type cbrace = d->m_name.find(']');
            if(obrace != d->m_name.npos && cbrace != d->m_name.npos)			// Gotta have found the braces
            if(obrace > 0)														// dref name can't be empty
            if(cbrace > obrace)													// braces must be in-order - avoids unsigned math insanity
            if((cbrace - obrace) > 1)											// Gotta have SOMETHING in between the braces
            {
                std::string number = d->m_name.substr(obrace+1,cbrace - obrace - 1);
                std::string refname = d->m_name.substr(0,obrace);
                
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

    drefResolveQueue.clear();

    for(xlua_cmd * d:cmdResolveQueue){
        XPLMCommandRef c = XPLMFindCommand(d->m_name.c_str());	
	    if(c == NULL){
		    printf("ERROR: Command %s not found\n",d->m_name.c_str());
	    }
        else
        {
            printf("Resolved Command %s\n",d->m_name.c_str());
        }
        d->m_cmd = c;
    }
    cmdResolveQueue.clear();
    /*for (auto x : cmdHandlerResolveQueue) {
        xlua_cmd * cmd=x.second;
        if(cmd->m_pre_handler)
			XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_pre_handler, 1, cmd);
		if(cmd->m_main_handler)
			XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_main_handler, 1, cmd);
		if(cmd->m_post_handler)
			XPLMRegisterCommandHandler(cmd->m_cmd, xlua_std_post_handler, 0, cmd);
    }
    cmdHandlerResolveQueue.clear();*/
    return retVal;
}
std::vector<xlua_cmd*> XTLuaDataRefs::XTGetHandlers(){
    std::vector<xlua_cmd*> retval;
    for (auto x : cmdHandlerResolveQueue) {
        xlua_cmd * cmd=x.second;
        retval.push_back(cmd);
    }
    return retval;
}
float XTLuaDataRefs::XTGetDataf(
                                   XPLMDataRef          inDataRef,bool local){
    data_mutex.lock();
    float retVal;
   // if(!local)
     //   retVal=XPLMGetDataf(inDataRef);
    char namec[32]={0};
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
                                     
    if(floatdataRefs.find(name)!=floatdataRefs.end()){
        XTLuaFloat val=floatdataRefs[name];
        if(retVal!=0.0||!local)
            val.get=true;
        retVal=val.values[0];
        floatdataRefs[name]=val;
    }
    else{
        XTLuaFloat val;
        val.isArray=false;
        val.end=1;
        val.start=0;
        if(retVal!=0.0||!local)
            val.get=true;
        val.ref=inDataRef;
        val.values.push_back(0.0);
        retVal=val.values[0];
        floatdataRefs[name]=val;

        

    }
    
    data_mutex.unlock();
    return retVal;                                   
}
void  XTLuaDataRefs::XTSetDataf(
                                   XPLMDataRef          inDataRef,    
                                   float                inValue,bool local)
{
    //printf("set %p=%f\n",inDataRef,inValue) ;
    //if(!local)
     //   XPLMSetDataf(inDataRef,inValue);
     data_mutex.lock();
    char namec[32]={0};
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    if(floatdataRefs.find(name)!=floatdataRefs.end()){
        XTLuaFloat val=floatdataRefs[name];
        
        if(inValue!=0.0||!local)
            val.set=true;
        val.values[0]=inValue;
        floatdataRefs[name]=val;
       
    }
    else{
        XTLuaFloat val;
        val.isArray=false;
        val.end=1;
        val.start=0;
        if(inValue!=0.0||!local)
            val.set=true;
        val.ref=inDataRef;
        val.values.push_back(inValue);
        floatdataRefs[name]=val;
        //printf("new set %p=%f\n",inDataRef,inValue) ;
    }
     XTLuaFloat val=floatdataRefs[name];
    
    data_mutex.unlock();
}

int XTLuaDataRefs::XTGetDatab(
                                   XPLMDataRef          inDataRef,    
                                   void *               outValue,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMaxBytes,bool local)
{
    data_mutex.lock();
    //outValue will be an array of chars
    char *outValues =(char *)outValue;
    char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    int retVal=0;//=XPLMGetDatab(inDataRef,outValue,inOffset,inMaxBytes);
    {
        if(outValues!=NULL){
            if(stringdataRefs.find(name)!=stringdataRefs.end()){
                XTLuaChars val=stringdataRefs[name];
                for(int i=inOffset;i<val.values.size();i++){
                    outValues[i]=val.values[i];
                    retVal++;
                }
                val.get=true;
                stringdataRefs[name]=val;
            }
        }
        else
        {
            if(stringdataRefs.find(name)!=stringdataRefs.end()){
                XTLuaChars val=stringdataRefs[name];
                val.get=true;
                for(int i=inOffset;i<(inOffset+inMaxBytes);i++){
                    if(i>=val.values.size())
                        val.values.push_back(outValues[i]);
                    else    
                        outValues[i]=val.values[i];   
                }
                stringdataRefs[name]=val;
                return val.values.size();
            }
            else {
                XTLuaChars val;
                //val.isArray=true;
                val.end=inOffset+inMaxBytes;
                val.start=inOffset;
                
                val.ref=inDataRef;
                val.size=-1;//we dont know this here
                for(int i=0;i<inOffset;i++){
                    val.values.push_back(0);
                }
                for(int i=inOffset;i<inOffset+inMaxBytes;i++){
                    val.values.push_back(outValues[i]);
                }
                val.get=true;
                val.size=val.end;
                stringdataRefs[name]=val;
            }
        }
        
    }
    data_mutex.unlock();
    return retVal;
}

void XTLuaDataRefs::XTSetDatab(
                                   XPLMDataRef          inDataRef,    
                                   void *               inValue,    
                                   int                  inOffset,    
                                   int                  inLength,bool local)
{
    data_mutex.lock();
    char *inValues =(char *)inValue;
    char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    if(inValues!=NULL){
        /*if(stringdataRefs.find(name)!=stringdataRefs.end()){
            //already have it
            XTLuaChars val=stringdataRefs[name];
            if((inOffset+inLength)>=val.values.size()){
                 for(int i=val.values.size();i<(inOffset+inLength);i++){
                     val.values.push_back(0);
                 }
            }
            for(int i=inOffset;i<(inOffset+inLength);i++){
                val.values[i]=inValues[i-inOffset];
                if(inValues[i]!=0||!local)
                    val.set=true;  
                //printf("apply set float %p %s[%d] = %f(%d)\n",val.ref,name.c_str(),i,inValues[i-inoffset],val.values.size());
                 
            }
            stringdataRefs[name]=val; 
            //val.get=true;
        }
        else {*/
            XTLuaChars val;
            val.end=inOffset+inLength;
            val.start=inOffset;
            
            val.ref=inDataRef;
            val.size=-1;//we dont know this here
            for(int i=0;i<inOffset;i++){
                val.values.push_back(0);
            }
            for(int i=inOffset;i<inOffset+inLength;i++){
                val.values.push_back(inValues[i]);
                if(inValues[i]!=0||!local)
                    val.set=true;

            }
            val.values.push_back(0);//null terminate
            val.size=val.end;
            stringdataRefs[name]=val;
//}
    }
    data_mutex.unlock();
}      
int XTLuaDataRefs::XTGetDatavf(
                                   XPLMDataRef          inDataRef,    
                                   float *              outValues,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMax,bool local)
{
    data_mutex.lock();
    char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    int retVal=0;
    
    {
        if(outValues!=NULL){
            if(floatdataRefs.find(name)!=floatdataRefs.end()){
                XTLuaFloat val=floatdataRefs[name];
                for(int i=inOffset;i<val.values.size();i++){
                    outValues[i]=val.values[i];
                    retVal++;
                }
                val.get=true;
                floatdataRefs[name]=val;
            }
        }
        else
        {
            if(floatdataRefs.find(name)!=floatdataRefs.end()){
                XTLuaFloat val=floatdataRefs[name];
                val.get=true;
                for(int i=inOffset;i<(inOffset+inMax);i++){
                    if(i>=val.values.size())
                        val.values.push_back(outValues[i]);
                    else    
                        outValues[i]=val.values[i];   
                }
                floatdataRefs[name]=val;
                return val.values.size();
            }
            else {
                XTLuaFloat val;
                val.isArray=true;
                val.end=inOffset+inMax;
                val.start=inOffset;
                
                val.ref=inDataRef;
                val.size=-1;//we dont know this here
                for(int i=0;i<inOffset;i++){
                    val.values.push_back(0.0);
                }
                for(int i=inOffset;i<inOffset+inMax;i++){
                    val.values.push_back(outValues[i]);
                }
                val.get=true;
                val.size=val.end;
                floatdataRefs[name]=val;
            }
        }
        
    }
    data_mutex.unlock(); 
    return retVal;
}

void XTLuaDataRefs::XTSetDatavf(
                                   XPLMDataRef          inDataRef,    
                                   float *              inValues,    
                                   int                  inoffset,    
                                   int                  inCount,bool local)
{
    data_mutex.lock();
    char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    if(inValues!=NULL){
        if(floatdataRefs.find(name)!=floatdataRefs.end()){
            //already have it
            XTLuaFloat val=floatdataRefs[name];
            if((inoffset+inCount)>=val.values.size()){
                 for(int i=val.values.size();i<(inoffset+inCount);i++){
                     val.values.push_back(0.0);
                 }
            }
            for(int i=inoffset;i<(inoffset+inCount);i++){
                val.values[i]=inValues[i-inoffset];
                if(inValues[i]!=0.0||!local)
                    val.set=true;  
                //printf("apply set float %p %s[%d] = %f(%d)\n",val.ref,name.c_str(),i,inValues[i-inoffset],val.values.size());
                 
            }
            floatdataRefs[name]=val; 
            //val.get=true;
        }
        else {
            XTLuaFloat val;
            val.isArray=true;
            val.end=inoffset+inCount;
            val.start=inoffset;
            
            val.ref=inDataRef;
            val.size=-1;//we dont know this here
            for(int i=0;i<inoffset;i++){
                val.values.push_back(0.0);
            }
            for(int i=inoffset;i<inoffset+inCount;i++){
                val.values.push_back(inValues[i]);
                if(inValues[i]!=0.0||!local)
                    val.set=true;

            }
            val.size=val.end;
            floatdataRefs[name]=val;
        }
    }
    //if(!local)
     //   XPLMSetDatavf(inDataRef,inValues,inoffset,inCount);
    data_mutex.unlock();
}    

//TODO
int  XTLuaDataRefs::XTGetDatavi(
                                   XPLMDataRef          inDataRef,    
                                   int *                outValues,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMax,bool local){
   /* data_mutex.lock();
    if(!local)
        return XPLMGetDatavi(inDataRef,outValues,inOffset,inMax);
    data_mutex.unlock();
     printf("dont do this?\n") ;    */ 
    return 0.0;    
} 
void XTLuaDataRefs::XTSetDatavi(
                                   XPLMDataRef          inDataRef,    
                                   int *                inValues,    
                                   int                  inoffset,    
                                   int                  inCount,bool local)
{
    /*data_mutex.lock();
    if(!local)
        XPLMSetDatavi(inDataRef,inValues,inoffset,inCount);
    data_mutex.unlock();
    printf("dont do this?\n") ; */
}                                   

double XTLuaDataRefs::XTGetDatad(
                                   XPLMDataRef          inDataRef,bool local){
    double retVal=0;
    data_mutex.lock();
    //if(!local)
     //   retVal=XPLMGetDatad(inDataRef);
    //printf("get %p=%f\n",inDataRef,retVal) ;                                  
    char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    //printf("get %p=%f\n",inDataRef,retVal) ;                                  
    if(doubledataRefs.find(name)!=doubledataRefs.end()){
        XTLuaDouble val=doubledataRefs[name];
        val.get=true;
        retVal=val.values[0];
        
    }
    else{
        XTLuaDouble val;
        val.isArray=false;
        val.end=1;
        val.start=0;
        val.get=true;
        val.ref=inDataRef;
        val.values.push_back(retVal);
        doubledataRefs[name]=val;
    }
    data_mutex.unlock();
    return retVal;                                   
}
void  XTLuaDataRefs::XTSetDatad(
                                   XPLMDataRef          inDataRef,    
                                   double                inValue,bool local)
{
    //printf("set %p=%f\n",inDataRef,inValue) ;
    data_mutex.lock();
    //if(!local)
     //   XPLMSetDatad(inDataRef,inValue);
    char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    if(doubledataRefs.find(name)!=doubledataRefs.end()){
        XTLuaDouble val=doubledataRefs[name];
        if(inValue!=0.0||!local)
            val.set=true;
        val.values[0]=inValue;
        doubledataRefs[name]=val;
    }
    else{
        XTLuaDouble val;
        val.isArray=false;
        val.end=1;
        val.start=0;
        if(inValue!=0.0||!local)
            val.set=true;
        val.ref=inDataRef;
        val.values.push_back(inValue);
        doubledataRefs[name]=val;
    }
    data_mutex.unlock();

}
int XTLuaDataRefs::XTGetDatai(
                                   XPLMDataRef          inDataRef,bool local)

{
    data_mutex.lock();
    int retVal=0;
   // if(!local)
    //    retVal=XPLMGetDatai(inDataRef);
    //printf("get %p=%f\n",inDataRef,retVal) ;                                  
    char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    //printf("get %p=%f\n",inDataRef,retVal) ;                                  
    if(intdataRefs.find(name)!=intdataRefs.end()){
        XTLuaInteger val=intdataRefs[name];
        if(retVal!=0.0||!local)
            val.get=true;
        retVal=val.values[0];
    }
    else{
        XTLuaInteger val;
        val.isArray=false;
        val.end=1;
        val.start=0;
        if(retVal!=0.0||!local)
            val.get=true;
        val.ref=inDataRef;
        val.values.push_back(retVal);
        intdataRefs[name]=val;
    }
    data_mutex.unlock();
    return retVal; 
}
void XTLuaDataRefs::XTSetDatai(
                                   XPLMDataRef          inDataRef,    
                                   int                  inValue,bool local) 
 {
     data_mutex.lock();
     //if(!local)
      //  XPLMSetDatai(inDataRef,inValue);
     char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    if(intdataRefs.find(name)!=intdataRefs.end()){
        XTLuaInteger val=intdataRefs[name];
        if(inValue!=0.0||!local)
            val.set=true;
        val.values[0]=inValue;
        intdataRefs[name]=val;
    }
    else{
        XTLuaInteger val;
        val.isArray=false;
        val.end=1;
        val.start=0;
        if(inValue!=0.0||!local)
            val.set=true;
        val.ref=inDataRef;
        val.values.push_back(inValue);
        intdataRefs[name]=val;
    }
    data_mutex.unlock();
 }

                        
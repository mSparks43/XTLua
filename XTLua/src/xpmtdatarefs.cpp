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

#include <vector>

static std::mutex data_mutex;

void XTLuaDataRefs::XTCommandBegin(xlua_cmd * cmd){
    data_mutex.lock();
    printf("Command start %s\n",cmd->m_name.c_str());
    XTCmd xtcmd;
    xtcmd.xluaref=cmd;
    xtcmd.start=true;
    commandQueue.push_back(xtcmd);
    data_mutex.unlock();
}

void XTLuaDataRefs::XTCommandEnd(xlua_cmd * cmd){
    data_mutex.lock();

    printf("Command stop %s\n",cmd->m_name.c_str());
       
    XTCmd xtcmd;
    xtcmd.xluaref=cmd;
    xtcmd.stop=true;
    commandQueue.push_back(xtcmd);

    
    data_mutex.unlock();
}

void XTLuaDataRefs::XTCommandOnce(xlua_cmd * cmd){
    data_mutex.lock(); 
    char namec[32]={0};
    sprintf(namec,"%p",cmd);
    std::string name=namec;
                                     
    if(fireCmds.find(name)==fireCmds.end()){
        printf("Command once %s\n",cmd->m_name.c_str());
       
        XTCmd xtcmd;
        xtcmd.xluaref=cmd;
        xtcmd.fire=true;
        commandQueue.push_back(xtcmd);
        fireCmds[name]=xtcmd;
    }
    else
    {
        //printf("skip Command once %s - in queue\n",cmd->m_name.c_str());
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
/*float XTLuaDataRefs::getsim_period(){
    return 0.02;	   
}*/
//begin DataRefs section
void XTLuaDataRefs::updateStringDataRefs(){
    
    for (auto x : stringdataRefs) {
        XTLuaCharArray* val=x.second;
        if(val->get&&!val->set){
           int size=XPLMGetDatab(val->ref,NULL,0,0);
           if(size>0) {
            std::vector<char> inVals(size);
            XPLMGetDatab(val->ref,&inVals[0],0,size);
            val->value=std::string(inVals.begin(),inVals.end());
           }
            val->get=false;
            
        }
        else if(val->set){ 
            const char * begin = val->value.c_str();
		    const char * end = begin + val->value.size();
		    if(end > begin)
		    {
			    XPLMSetDatab(val->ref, (void *) begin, 0, end - begin);
		    }
            val->set=false;

        }
        
    }
    
    /*std::unordered_map<std::string, XTLuaChars> incomingStringdataRefs;
    for (auto x : stringdataRefs) {
       // int i=0;
        XTLuaChars val=x.second;
        
        if(val.get&&!val.set){
           int size=XPLMGetDatab(val.ref,NULL,0,0);
            
           std::vector<char> inVals(size + 1);
           XPLMGetDatab(val.ref,inVals.data(),0,size);
           //printf("get char %s = %s(%d)\n",x.first.c_str(),inVals,size);
           //val.values.clear();
           XTLuaChars newval;
           newval.ref=val.ref;
           newval.get=false;
             newval.set=false;
           for(int i=0;i<size;i++){
                newval.values.push_back(inVals[i]);
                
            }
            if(newval.values[newval.values.size()-1]!=0)
                newval.values.push_back(0);
            
            incomingStringdataRefs[x.first]=newval;
        }
        else if(val.set){ 
            
             XTLuaChars newval;
             newval.ref=val.ref;
             newval.get=false;
             newval.set=false;
             
             std::vector<char> outVals(val.values.size() + 1);
              for(int i=0;i<val.values.size();i++){
                outVals[i]=val.values[i];
                newval.values.push_back(outVals[i]);//.push_back(outVals[i]);
            }
            //newval.values.push_back(0);
            //printf("set char %s to %s(%d)\n",x.first.c_str(),outVals,val.values.size());
            incomingStringdataRefs[x.first]=newval;
            XPLMSetDatab(val.ref,outVals.data(),0,val.values.size());

        }
        
    }
    for (auto x : incomingStringdataRefs) {
        XTLuaChars val=x.second;
        stringdataRefs[x.first]=val;
    }*/
}
void XTLuaDataRefs::updateCommands(){
    for(XTCmd c:commandQueue){
        xlua_cmd* cmd=c.xluaref;
        if(c.fire){
            //printf("Do Command once %s %p\n",cmd->m_name.c_str(),cmd->m_cmd);
            XPLMCommandOnce(cmd->m_cmd);
        
        }
        if(c.start){
            //printf("Do Command Begin %s %p\n",cmd->m_name.c_str(),cmd->m_cmd);
            XPLMCommandBegin(cmd->m_cmd);
        }
        if(c.stop){
            //printf("Do Command End %s %p\n",cmd->m_name.c_str(),cmd->m_cmd);
            XPLMCommandEnd(cmd->m_cmd);
        }
    }
    commandQueue.clear();
    fireCmds.clear();
}
void XTLuaDataRefs::updateFloatDataRefs(){
    //std::unordered_map<std::string, XTLuaFloat> incomingFloatdataRefs;
    for (auto x : floatdataRefs) {
        int i=0;
        string name=x.first;
        std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
        if(val.size()==1){
            float v=val[0]->value;
            if(val[0]->get&&!val[0]->set){

                
                float newVal=XPLMGetDataf(val[0]->ref);
                if(val[0]->type == xplmType_Double){
                    newVal=XPLMGetDatad(val[0]->ref);
                    
                }
                else if(val[0]->type == xplmType_Int){
                    newVal=XPLMGetDatai(val[0]->ref);
                }
                //printf("get float %s[%d] = %f(%d) was %f\n",x.first.c_str(),i,newVal,val.size(),v);
                
                val[0]->get=false;
                val[0]->value=newVal;
                
            }
            else if(val[0]->set){ 
                //printf("set float %s[%d] = %f(%d)\n",x.first.c_str(),i,v,val.size());
                if(val[0]->type == xplmType_Double){
                    XPLMSetDatad(val[0]->ref,v);
                    
                }else if(val[0]->type == xplmType_Float){
                    XPLMSetDataf(val[0]->ref,v);
                }
                else if(val[0]->type == xplmType_Int){
                    XPLMSetDatai(val[0]->ref,v);
                }
                val[0]->set=false;
            }
        }
        else{
            bool hasSetUpdate=false;
            bool hasGetUpdate=false;
            int size=val.size();
            if(val[0]->type == xplmType_Int)
            {
                std::vector<int> inVals(size); 
                std::vector<int> outVals(val.size());
                for(int i=0;i<val.size();i++){
                    if(val[i]->get){
                        hasGetUpdate=true;
                    }
                    val[i]->get=false;
                    inVals[i]=val[i]->value;
                }
                //int size=XPLMGetDatavf(val[0]->ref,NULL,0,0);
                //printf("update array int %p get = %d\n",val[i]->ref,hasGetUpdate);
                if(hasGetUpdate)
                    XPLMGetDatavi(val[0]->ref,inVals.data(),0,size);           
                
                for(int i=0;i<size;i++){
                    if(val[i]->set){
                            outVals[i]=val[i]->value;
                            val[i]->set=false;
                            hasSetUpdate=true;
                    }
                    else
                    {
                       
                       if(hasGetUpdate){
                            val[i]->value=inVals[i];
                            outVals[i]=inVals[i];
                       }
                       else{
                            outVals[i]=val[i]->value;
                       }
                    }
                    //if(hasSetUpdate)
                    //    printf("set array int %p %s[%d] = %f(%d=%d)\n",val[i]->ref,x.first.c_str(),i,val[i]->value,val.size(),val[i]->value);    
                }
                if(hasSetUpdate){
                    XPLMSetDatavi(val[0]->ref,outVals.data(),0,val.size());
                    
                }
            }
            else
            {
                std::vector<float> inVals(size); 
                std::vector<float> outVals(val.size());

                for(int i=0;i<val.size();i++){
                    if(val[i]->get){
                        hasGetUpdate=true;
                    }
                    val[i]->get=false;
                    inVals[i]=val[i]->value;
                }
                //int size=XPLMGetDatavf(val[0]->ref,NULL,0,0);
                if(hasGetUpdate)
                    XPLMGetDatavf(val[0]->ref,inVals.data(),0,size);           
                
                for(int i=0;i<size;i++){
                    if(val[i]->set){
                            outVals[i]=val[i]->value;
                            val[i]->set=false;
                            hasSetUpdate=true;
                        }
                        else
                        {
                            
                            if(hasGetUpdate){
                                val[i]->value=inVals[i];
                                outVals[i]=inVals[i];
                            }else{
                                outVals[i]=val[i]->value;
                            }
                        }
                        
                       // printf("set array float %p %s[%d] = %f(%d=%d)\n",val[i]->ref,x.first.c_str(),i,val[i]->value,val.size(), val[i]->value);
                }
                if(hasSetUpdate)
                    XPLMSetDatavf(val[0]->ref,outVals.data(),0,val.size());
            }
            
            
            

        }
    }
    

}

double XTLuaDataRefs::XTGetElapsedTime(){
    //data_mutex.lock();
    double retVal=timeT;
    //data_mutex.unlock();
    return retVal;

}
void XTLuaDataRefs::updateDataRefs(){
    data_mutex.lock();
    //printf("updateDataRefs\n");
        timeT = XPLMGetElapsedTime();
        if(updateRoll==0)
        {
            updateStringDataRefs();
        }
        else if(updateRoll==1)
        {
            updateFloatDataRefs();
            updateRoll=-1;//will go back to 0 next line
        }
        //else if(updateRoll==2)
        {
            updateCommands(); //always do command queue
            
        } 
        updateRoll++;
    data_mutex.unlock();
}
void XTLuaDataRefs::cleanup(){
    data_mutex.lock();
    drefResolveQueue.clear();
    cmdResolveQueue.clear();
     for (auto x : floatdataRefs) {
        int i=0;
        string name=x.first;
        std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
        for(XTLuaArrayFloat* k:val){
            delete k;
        }
     }
     floatdataRefs.clear();
     for (auto x : stringdataRefs) {
        int i=0;
        string name=x.first;
        XTLuaCharArray* val=stringdataRefs[name];
        delete val;
        
     }
     stringdataRefs.clear();
     printf("XTLua:Cleaned up data\n");
    data_mutex.unlock();
}


void XTLuaDataRefs::XTqueueresolve_dref(xlua_dref * d){
    data_mutex.lock();
    drefResolveQueue.push_back(d);//this needs to be done on another thread
    data_mutex.unlock();
}
void XTLuaDataRefs::XTqueueresolve_cmd(xlua_cmd * d){
    data_mutex.lock();
    cmdResolveQueue.push_back(d);//this needs to be done on another thread
    data_mutex.unlock();
}
 



int XTLuaDataRefs::resolveQueue(){
    int retVal=0;
    data_mutex.lock();
    for(xlua_dref * d:drefResolveQueue){
        assert(d->m_dref == NULL);
        assert(d->m_types == 0);
        assert(d->m_index == -1);
        assert(d->m_ours == 0);
        d->m_dref = XPLMFindDataRef(d->m_name.c_str());
        //initialise our datasets
        if(d->m_dref)
        {
            d->m_index = -1;
            d->m_types = XPLMGetDataRefTypes(d->m_dref);
            printf("Resolved dref %s to %p as %d\n",d->m_name.c_str(),d->m_dref,d->m_types);
            if(d->m_types & (xplmType_FloatArray | xplmType_IntArray))			// an array type
            {
                char namec[32];
                sprintf(namec,"%p",d->m_dref);
                std::string name=namec;

                int size=0;
                int type=xplmType_Float;
                if(!(d->m_types & xplmType_FloatArray)){
                    type=xplmType_Int;
                    
                    size=XPLMGetDatavi(d->m_dref,NULL,0,0);
                    printf("Resolved int array dref %s to %p with %d\n",d->m_name.c_str(),d->m_dref,size);
                }
                else{
                    
                    size=XPLMGetDatavf(d->m_dref,NULL,0,0);
                    printf("Resolved float array dref %s to %p with %d\n",d->m_name.c_str(),d->m_dref,size);
                }
                
                std::vector<float> inVals(size);
                std::vector<int> inIVals(size);
                if(!(d->m_types & xplmType_FloatArray))
                    XPLMGetDatavi(d->m_dref,inIVals.data(),0,size);
                else
                    XPLMGetDatavf(d->m_dref,inVals.data(),0,size);
                //XTLuaFloat newval;
               // newval.ref=d->m_dref;
                //newval.get=false;
               // newval.set=false;
                
                
                for(int i=0;i<size;i++){
                    XTLuaArrayFloat* v=new XTLuaArrayFloat;
                    if(!(d->m_types & xplmType_FloatArray))
                        v->value=inIVals[i];
                    else
                        v->value=inVals[i];
                    printf("defaulted array dref %s to %f with %d\n",d->m_name.c_str(),v->value,size);  
                    v->ref=d->m_dref;
                    v->type=type;
                    v->get=true;
                    v->index=i;
                    //printf("%d=%f\n",i,inVals[i]);
                    //newval.values.push_back(v);
                    floatdataRefs[name].push_back(v);
                }
                //floatdataRefs[name]=newval;
            }
            else if(d->m_types & xplmType_Float || d->m_types & xplmType_Double || d->m_types & xplmType_Int){
                char namec[32];
                sprintf(namec,"%p",d->m_dref);
                std::string name=namec;
                float val=0.0;
                XTLuaArrayFloat* v=new XTLuaArrayFloat;
                if(d->m_types & xplmType_Double){
                    val=XPLMGetDatad(d->m_dref);
                    v->type=xplmType_Double;
                }
                else if(d->m_types & xplmType_Float){
                    val=XPLMGetDataf(d->m_dref);
                    v->type=xplmType_Float;
                }
                else if(d->m_types & xplmType_Int){
                    val=XPLMGetDatai(d->m_dref);
                    v->type=xplmType_Int;
                }
                v->get=true;
                v->value=val;
                v->ref=d->m_dref;
                //newval.values.push_back(v);
                //printf(" =%f\n",val);
                floatdataRefs[name].push_back(v);

            }else if(d->m_types & xplmType_Data){
                //its a string!
                char namec[32];
                sprintf(namec,"%p",d->m_dref);
                std::string name=namec;
                XTLuaCharArray* v=new XTLuaCharArray;
                v->get=true;
                v->value="";
                v->ref=d->m_dref;
                stringdataRefs[name]=v;
            }
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
                            char namec[32];
                            sprintf(namec,"%p",d->m_dref);
                            std::string name=namec;
                            printf("Resolved singleton dref %s to %p\n",d->m_name.c_str(),d->m_dref);
                            /*float val=0.0;
                            XTLuaArrayFloat* v=new XTLuaArrayFloat;
                            if(d->m_types & xplmType_Double){
                                val=XPLMGetDatad(d->m_dref);
                                v->type=xplmType_Double;
                            }
                            else if(d->m_types & xplmType_Float){
                                val=XPLMGetDataf(d->m_dref);
                                v->type=xplmType_Float;
                            }
                            else if(d->m_types & xplmType_Int){
                                val=XPLMGetDatai(d->m_dref);
                                v->type=xplmType_Int;
                            }
                
                            v->value=val;
                            v->ref=d->m_dref;*/

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

    data_mutex.unlock();
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
    float retVal=0;
   // if(!local)
     //   retVal=XPLMGetDataf(inDataRef);
    char namec[32]={0};
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
                                 
    if(floatdataRefs.find(name)!=floatdataRefs.end()){
        std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
        val[0]->get=true;
        retVal=val[0]->value;
        //floatdataRefs[name]=val;
    }
    else{
        printf("didn't initialise %s",name.c_str());
        /*XTLuaFloat val;
        //val.isArray=false;
        //val.end=1;
        //val.start=0;
        //if(retVal!=0.0||!local)
        val.ref=inDataRef;
        XTLuaArrayFloat fval;
        fval.value=0.0;
        fval.get=true;
        val.values.push_back(fval);
        retVal=val.values[0].value;
        floatdataRefs[name]=val;*/

        

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
        std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
        if(val[0]->value!=inValue){
            val[0]->set=true;
            val[0]->value=inValue;
        }
    }
    else{
        printf("didn't initialise %s",name.c_str());
       /* XTLuaFloat val;
        //val.isArray=false;
        //val.end=1;
        //val.start=0;
        if(inValue!=0.0||!local)
            val.set=true;
        val.ref=inDataRef;
        XTLuaArrayFloat fval;
        fval.value=inValue;
        fval.set=true;
        val.values.push_back(fval);
        floatdataRefs[name]=val;
        //printf("new set %p=%f\n",inDataRef,inValue) ;*/
    }
     //XTLuaFloat val=floatdataRefs[name];
    
    data_mutex.unlock();
}

int XTLuaDataRefs::XTGetDatab(
                                   xlua_dref * d,    
                                   void *               outValue,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMaxBytes,bool local)
{
    data_mutex.lock();
    //outValue will be an array of chars
    XPLMDataRef  inDataRef=d->m_dref;
    char *outValues =(char *)outValue;
    char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
   // printf("apply XTGetDatab %s[%d] %s\n",name.c_str(),inMaxBytes,outValues!=NULL?"values":"size");
     int retVal=0;
    
    {
        if(outValues!=NULL){
            /*if(stringdataRefs.find(name)!=stringdataRefs.end()){
                XTLuaChars val=stringdataRefs[name];
                for(int i=inOffset;i<val.values.size()&&i-inOffset<inMaxBytes;i++){
                    outValues[i-inOffset]=val.values[i];
                    retVal++;
                }
                val.get=true;
                stringdataRefs[name]=val;
            }*/
            if(stringdataRefs.find(name)!=stringdataRefs.end()){
                XTLuaCharArray* val=stringdataRefs[name];
                const char * charArray=val->value.c_str();
                for(int i=inOffset;i<val->value.length()&&i-inOffset<inMaxBytes;i++){
                    outValues[i-inOffset]=charArray[i];
                    //val[i]->get=true;
                    //retVal++;
                    //printf("apply XTGetDatavf %s %s[%d/%d] %s = %f\n",d->m_name.c_str(),name.c_str(),inOffset,inMax,outValues!=NULL?"values":"size",val[i]->value);
                }
            }
        }
        else
        {
            /*if(stringdataRefs.find(name)!=stringdataRefs.end()){
                XTLuaChars val=stringdataRefs[name];
                val.get=true;
                for(int i=inOffset;i<(inOffset+inMaxBytes);i++){
                    if(i>=val.values.size())
                        val.values.push_back(outValues[i]);
                        //outValues is NULL here
                    
                }
                stringdataRefs[name]=val;
                data_mutex.unlock();
                return val.values.size();
            }*/
            if(stringdataRefs.find(name)!=stringdataRefs.end()){
                XTLuaCharArray* val=stringdataRefs[name];
                retVal=val->value.size();
                //data_mutex.unlock();
                //return retVal;
            }
            
        }
        
    }
    data_mutex.unlock();
    return retVal;
}

void XTLuaDataRefs::XTSetDatab(
                                   xlua_dref * d,    
                                   std::string value)
{
    
    //char *inValues =(char *)inValue;
    char namec[32];
    XPLMDataRef  inDataRef=d->m_dref;
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    data_mutex.lock();
    //printf("apply XTSetDatab %s[%d]\n",d->m_name.c_str(),inLength);
    //if(inValues!=NULL)
    {
            /*XTLuaChars val;
            val.ref=inDataRef;
            for(int i=inOffset;i<inOffset+inLength;i++){
                 if(i>=val.values.size()){
                     val.values.push_back(inValues[i]);
                     val.set=true;
                 }
                 else if(i<val.values.size()&&val.values[i]!=inValues[i]){ 
                     val.values.erase(val.values.begin()+i,val.values.end());//its a new string, clear the old one  
                     val.values[i]=inValues[i]; 
                     val.set=true;
                 }  
            }
            
            if(val.values[val.values.size()-1]!=0)
                val.values.push_back(0);//null terminate
            stringdataRefs[name]=val;*/
            XTLuaCharArray* val=stringdataRefs[name];
            val->value=value;
            val->set=true;
            /*if(inLength<val.size()){
                if(val[inLength]->value!=inValue)
                    val[inLength]->set=true;
                val[inLength]->value=inValue;
 
            }*/

    }
    data_mutex.unlock();
}      
int XTLuaDataRefs::XTGetDatavf(
                                   xlua_dref * d,    
                                   float *              outValues,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMax,bool local)
{

    data_mutex.lock();
    //outValue will be set to an array of floats
    char namec[32];
    XPLMDataRef  inDataRef=d->m_dref;
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    int retVal=0;
    
    
     
    
    {
        if(outValues!=NULL){
            if(floatdataRefs.find(name)!=floatdataRefs.end()){
                std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
                for(int i=inOffset;i<val.size()&&i-inOffset<inMax;i++){
                    outValues[i-inOffset]=val[i]->value;
                    val[i]->get=true;
                    retVal++;
                    //printf("apply XTGetDatavf %s %s[%d/%d] %s = %f\n",d->m_name.c_str(),name.c_str(),inOffset,inMax,outValues!=NULL?"values":"size",val[i]->value);
                }
            }
        }
        else
        {
            if(floatdataRefs.find(name)!=floatdataRefs.end()){
                std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
                retVal=val.size();
                //data_mutex.unlock();
                //return retVal;
            }
            //printf("apply XTGetDatavf %s %s[%d/%d] %s = %d\n",d->m_name.c_str(),name.c_str(),inOffset,inMax,outValues!=NULL?"values":"size",retVal);
            /*else {
                XTLuaFloat val;               
                val.ref=inDataRef;

                for(int i=0;i<inOffset+inMax;i++){
                    XTLuaArrayFloat fval;
                    fval.value=0.0;
                    fval.get=true;
                    val.values.push_back(fval);
                }

                val.get=true;
                floatdataRefs[name]=val;
            }*/
        }
        
    }
    
    data_mutex.unlock();
    return retVal;

}

void XTLuaDataRefs::XTSetDatavf(
                                   xlua_dref * d,    
                                   float              inValue,    
                                   int                  index)
{
    
    data_mutex.lock();
    char namec[32];
    XPLMDataRef  inDataRef=d->m_dref;
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
  
    if(floatdataRefs.find(name)!=floatdataRefs.end()){
        std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
        if(index<val.size()){
            if(val[index]->value!=inValue)
                val[index]->set=true;
            val[index]->value=inValue;
            
          //printf("apply XTSetDatavf single %s %s[%d] = %f\n",d->m_name.c_str(),name.c_str(),index,val[index]->value); 
        }
    }
    
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
    data_mutex.unlock();*/ 
     printf("dont do this getvi?\n") ; 
    /*(int i=inOffset;i<inOffset+inMax;i++){
            outValues[i]=0;

    }*/
    return 0;    
} 
void XTLuaDataRefs::XTSetDatavi(
                                   XPLMDataRef          inDataRef,    
                                   int *                inValues,    
                                   int                  inOffset,    
                                   int                  inLength,bool local)
{
    
}                                   

double XTLuaDataRefs::XTGetDatad(
                                   XPLMDataRef          inDataRef,bool local){
    double retVal=0.0;
    printf("dont do this getd?\n") ;
    /*data_mutex.lock();
    printf("dont do this getd?\n") ;
                                
    char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
                                 
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
    data_mutex.unlock();*/
    return retVal;                                   
}
void  XTLuaDataRefs::XTSetDatad(
                                   XPLMDataRef          inDataRef,    
                                   double                inValue,bool local)
{
    printf("dont do this setd?\n") ;
    /*data_mutex.lock();


     
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
    data_mutex.unlock();*/

}
int XTLuaDataRefs::XTGetDatai(
                                   XPLMDataRef          inDataRef,bool local)

{
    int retVal=0;
    /*data_mutex.lock();                                
    char namec[32];
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;                                 
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
    data_mutex.unlock();*/
    return retVal; 
}
void XTLuaDataRefs::XTSetDatai(
                                   XPLMDataRef          inDataRef,    
                                   int                  inValue,bool local) 
 {
     /*data_mutex.lock();
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
    data_mutex.unlock();*/
 }

                        
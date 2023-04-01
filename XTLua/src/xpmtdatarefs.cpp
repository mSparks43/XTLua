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
#include <XPLMNavigation.h>
#include "xpmtdatarefs.h"
#include <stdio.h>
#include <assert.h>
#include "SerialWidget.h"
#include "json/json.hpp"
#include <vector>
using nlohmann::json;
static std::mutex data_mutex;

void XTLuaDataRefs::XTCommandBegin(xtlua_cmd * cmd){
    data_mutex.lock();
    printf("Command start %s\n",cmd->m_name.c_str());
    XTCmd* xtcmd=new XTCmd();
    xtcmd->xluaref=cmd;
    xtcmd->start=true;
    xtcmd->stop=false;
    xtcmd->fire=false;
    commandQueue.push_back(xtcmd);
    data_mutex.unlock();
}

void XTLuaDataRefs::XTCommandEnd(xtlua_cmd * cmd){
    data_mutex.lock();

    printf("Command stop %s\n",cmd->m_name.c_str());
       
    XTCmd* xtcmd=new XTCmd();
    xtcmd->xluaref=cmd;
    xtcmd->stop=true;
    xtcmd->fire=false;
    xtcmd->start=false;
    commandQueue.push_back(xtcmd);

    
    data_mutex.unlock();
}

void XTLuaDataRefs::XTCommandOnce(xtlua_cmd * cmd){
    data_mutex.lock(); 
    char namec[32]={0};
    sprintf(namec,"%p",cmd);
    std::string name=namec;
                                     
    //if(fireCmds.find(name)==fireCmds.end()){
        printf("Command once %s\n",cmd->m_name.c_str());
       
        XTCmd* xtcmd=new XTCmd();
        xtcmd->xluaref=cmd;
        xtcmd->fire=true;
        xtcmd->stop=false;
        xtcmd->start=false;
        commandQueue.push_back(xtcmd);
        //fireCmds[name]=xtcmd;
    
  
    data_mutex.unlock();
}


void XTLuaDataRefs::XTRegisterCommandHandler(xtlua_cmd * cmd){
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
            //printf("getting %d is %d\n",val->ref,size);
           if(size>0) {
            std::vector<char> inVals(size);
            XPLMGetDatab(val->ref,&inVals[0],0,size);
            val->value=std::string(inVals.begin(),inVals.end());
             //printf("got %s is %d\n",val->value.c_str(),size);
           }
            val->get=false;
            
        }
        else if(val->set){ 
            const char * begin = val->value.c_str();
		    const char * end = begin + val->value.size();
		    if(end > begin)
		    {
			    XPLMSetDatab(val->ref, (void *) begin, 0, (int)(end - begin));
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
float scale(XTControlObject* c){
    float x=XPLMGetDataf(c->srcDref);
    float scale=c->scale;
    if(c->scaleDref){
        scale=XPLMGetDataf(c->scaleDref);
        c->scale=scale;
    }  
    if (x < c->minin)
        return c->minout*scale;
    if (x > c->maxin)
        return c->maxout*scale;
     
     
    return (c->minout + (c->maxout - c->minout) * (x - c->minin) / (c->maxin - c->minin))*scale;

}
void XTLuaDataRefs::updateCommands(){
    //externally locked
    for(XTCmd* c:commandQueue){
        xtlua_cmd* cmd=c->xluaref;
        if(c->fire){
            printf("Do Command once %s %p\n",cmd->m_name.c_str(),cmd->m_cmd);
            XPLMCommandOnce(cmd->m_cmd);
        
        }
        if(c->start){
            printf("Do Command Begin %s %p\n",cmd->m_name.c_str(),cmd->m_cmd);
            XPLMCommandBegin(cmd->m_cmd);
        }
        if(c->stop){
            printf("Do Command End %s %p\n",cmd->m_name.c_str(),cmd->m_cmd);
            XPLMCommandEnd(cmd->m_cmd);
        }
        delete c;
    }
    commandQueue.clear();
    std::unordered_map<XPLMDataRef,float> newValues;
    for(XTControlObject* c:controlOverrides){
         
         if(c->srcDref==NULL){ //needs init
            printf("Do Create Override %s\n",c->data.c_str());
            json jData =json::parse(c->data);
            printf("Find src %s\n",jData["srcDref"].get<std::string>().c_str());
            c->srcDref=XPLMFindDataRef(jData["srcDref"].get<std::string>().c_str());
            printf("Find dst %s\n",jData["dstDref"].get<std::string>().c_str());
            c->dstDref=XPLMFindDataRef(jData["dstDref"].get<std::string>().c_str());
            c->dstIndex=-1;
            if(c->dstDref)
			{
				XPLMDataTypeID tid = XPLMGetDataRefTypes(c->dstDref);
                if(tid & (xplmType_FloatArray | xplmType_IntArray))			// AND are array type
				{
                   c->dstIndex =jData["dstIndex"].get<int>();
                }
            }
            if(jData.contains("scale"))
                c->scale=jData["scale"].get<float>();
            else
                c->scale=1.0;
            if(jData.contains("scaledref"))
            {
                 c->scaleDref=XPLMFindDataRef(jData["scaledref"].get<std::string>().c_str());
            } 
            if(jData.contains("min"))
            {
                c->min=jData["min"].get<float>();
            }
            if(jData.contains("max"))
            {
                c->max=jData["max"].get<float>();
            }
            c->minin=jData["minin"].get<float>();
            c->maxin=jData["maxin"].get<float>();
            c->minout=jData["minout"].get<float>();
            c->maxout=jData["maxout"].get<float>();
            //c->srcDref = xtlua_find_dref(jData["srcDref"].get<std::string>().c_str());
            //c->dstDref = xtlua_find_dref(jData["dstDref"].get<std::string>().c_str());
            
         }
         
         if(c->srcDref!=NULL && c->dstDref!=NULL){
            float newVal=scale(c);
            if(newValues.find(c->dstDref)!=newValues.end()){
                float oldVal=newValues[c->dstDref];
                float minVal=c->minout;
                float maxVal=c->maxout;
                if(c->scale<0)
                {
                    float oldMin=minVal;
                    minVal=maxVal*-1;
                    maxVal=oldMin*-1;
                }
                if(oldVal<minVal)
                    minVal=oldVal;
                if(oldVal>maxVal)
                    maxVal=oldVal; 
                if(c->min>-9999)
                    minVal=c->min;
                if(c->max<9999)
                    maxVal=c->max;           
                newVal+=oldVal;
                if(newVal>maxVal)
                    newVal=maxVal;
                if(newVal<minVal)
                    newVal=minVal;    
                
                newValues[c->dstDref]=newVal;
                //printf("Do Override %f %f %s \n",oldVal,newVal,c->data.c_str());
            }
            else
                newValues[c->dstDref]=newVal;
            //printf("Do Override %f %s \n",newVal,c->data.c_str());
            
            
         }
         else
            printf("Cant Override %s\n",c->data.c_str());
    }
    for (auto x : newValues) {
        XPLMDataRef c=x.first;
        float val=newValues[c];
       /*if(c->dstIndex>=0){
            XPLMSetDatavf(c->dstDref, &val, c->dstIndex, 1);
        }
        else*/
        XPLMSetDataf(c, val);
    }
   // fireCmds.clear();
}
void XTLuaDataRefs::addNavData(int    id,
        int    type,
        float  latitude,
        float  longitude, 
        int    frequency,
        float  heading,
        char * name,char * ident){

        NavAid* navaid=new NavAid(); 
        navaid->id=id;
        navaid->type=type;
        navaid->latitude=latitude;
        navaid->longitude=longitude;
        navaid->frequency=frequency;
        navaid->heading=heading;
        navaid->name=std::string(name);
         navaid->ident=std::string(ident);
        navaid->next=NULL;
        if(lastnavaid==NULL)
            navaids=navaid;
        else 
            lastnavaid->next=navaid;
        lastnavaid=navaid;
}
void XTLuaDataRefs::updateNavDataRefs(){
    int entries=XPLMCountFMSEntries();
    int currentIndex=XPLMGetDestinationFMSEntry();
    json nVdata =json::array();
    int count=0;
    for(int i=0;i<entries;i++){
          XPLMNavType         outType;
          char                outID[32]={0}; 
          XPLMNavRef          outRef=XPLM_NAV_NOT_FOUND;
          int                 outAltitude;
          float               outLat;
          float               outLon;
          XPLMGetFMSEntryInfo(i,&outType,outID,&outRef,&outAltitude,&outLat,&outLon); 
          if(outRef!=XPLM_NAV_NOT_FOUND){
              XPLMNavType         outType2;   
              float               outLatitude;    
              float               outLongitude;   
              float               outHeight;    
              int                 outFrequency=0;   
              float               outHeading;    
                 
              char                outName[256]={0};    
              char                outReg[1]={0};
              XPLMGetNavAidInfo(outRef,&outType2,&outLatitude,&outLongitude,&outHeight,&outFrequency,&outHeading,outID,outName,outReg);
              //printf("getting %d=%d,%d ,%s\n",i,outType,outFrequency,outID); 
              double latDiff=outLatitude-outLat;
              if(latDiff>180)
                latDiff-=360;
              if(latDiff<-180)
                latDiff+=360;  
              double lonDiff=outLongitude-outLon;
              if(lonDiff>180)
                lonDiff-=360;
              if(lonDiff<-180)
                lonDiff+=360; 
              if(latDiff>-1 && latDiff < 1 && lonDiff>-1 && lonDiff < 1) { 
                nVdata[count]=json::array({outRef,outType,outFrequency,outHeading,outLatitude,outLongitude,string(outName),string(outID),outAltitude,(i==currentIndex)});
              }
              else
              {
                 char val[256];
                sprintf(val,"%f %f",latDiff,lonDiff);
                 nVdata[count]=json::array({outRef,outType,0,0,outLat,outLon,string("latlon"),string("latlon"),outAltitude,(i==currentIndex)});
              }
              
              count++;
          }
          else{
              nVdata[count]=json::array({outRef,outType,0,0,outLat,outLon,string("latlon"),string("latlon"),outAltitude,(i==currentIndex)});
              count++;
          }
    }
    
    
    
    incomingFMSString=nVdata.dump();
    
    if(navaids==NULL){
        XPLMNavRef nAid=XPLMGetFirstNavAid();
        latR = XPLMFindDataRef("sim/flightmodel/position/latitude");
        lonR = XPLMFindDataRef("sim/flightmodel/position/longitude");
        while(nAid!=XPLM_NAV_NOT_FOUND){
                XPLMNavType         outType;    /* Can be NULL */
                float               outLatitude=-200;    /* Can be NULL */
                float               outLongitude=-200;    /* Can be NULL */
                float               outHeight;    /* Can be NULL */
                int                 outFrequency=0;    /* Can be NULL */
                float               outHeading=0;    /* Can be NULL */
                char                outID[32];    /* Can be NULL */
                char                outName[256];    /* Can be NULL */
                char                outReg[1];
                XPLMGetNavAidInfo(nAid,&outType,&outLatitude,&outLongitude,&outHeight,&outFrequency,&outHeading,outID,outName,outReg);
                /*double latDif=outLatitude-lat;
                double lonDif=outLongitude-lon;
                if(outType!=512&&latDif<2&&latDif>-2&&lonDif<2&&lonDif>-2)
                //if(outType!=512)
                    printf("%d=%d,%d,%f,%f,%f,%s\n",nAid,outType,outFrequency,latDif,lonDif,outHeading,outName); */
                if(outType!=512)
                    addNavData(nAid,outType,outLatitude,outLongitude,outFrequency,outHeading,outName,outID);
                nAid=XPLMGetNextNavAid(nAid); 
            }
    }
    lat=XPLMGetDatad(latR);
    lon=XPLMGetDatad(lonR);
}
bool firstPass=true;
void XTLuaDataRefs::update_localNavData(){
    if(skipNaviads)
        return;
    if(current_navaid==NULL){
        current_navaid=navaids;
         //printf("Nav Rollover\n");
         
         if(localNavaids.size()>0){
            std::vector<int> left;
            int count=0;
            json nVdata =json::array();
            for (auto x : localNavaids) {
                NavAid* val=x.second;
                double latDif=val->latitude-lat;
                
                double lonDif=val->longitude-lon;
                if(lonDif>180)
                    lonDif-=360;
                if(lonDif<-180)
                    lonDif+=360; 
                if(latDif>180)
                    latDif-=360;
                if(latDif<-180)
                    latDif+=360;     
                if(((latDif>2||latDif<-2||lonDif<-2||lonDif>2)&&val->type!=8)||(val->type==8&&(latDif>10||latDif<-10||lonDif<-10||lonDif>10)))
                    left.push_back(val->id);//localNavaids.erase(val->id);
                else{
                    nVdata[count]=json::array({val->id,val->type,val->frequency,val->heading,val->latitude,val->longitude,val->name,val->ident});
                    count++;
                }
                
            }
            

                incomingNavaidString=nVdata.dump();//printf("erasing %d\n",left.size());
            for (int id:left)
                localNavaids.erase(id);
            }
    }
    double latDif=lastUpdatelat-lat;
    double lonDif=lastUpdatelon-lon;
    if(lonDif>180)
        lonDif-=360;
    if(lonDif<-180)
        lonDif+=360; 
    if(latDif>180)
        latDif-=360;
    if(latDif<-180)
        latDif+=360; 
    if(latDif<0.5&&latDif>-0.5&&lonDif>-0.5&&lonDif<0.5)
        return;
    //printf("update_localNavData\n");    
    int count=0;
    //int cSize=localNavaids.size();
    while(current_navaid!=NULL&&(count<40||firstPass)){
        double latDif=current_navaid->latitude-lat;
        double lonDif=current_navaid->longitude-lon;
        if((current_navaid->type==8&&(latDif<10&&latDif>-10&&lonDif<10&&lonDif>-10))||(latDif<2&&latDif>-2&&lonDif<2&&lonDif>-2)){
            localNavaids[current_navaid->id]=current_navaid;
            //printf("%d=%d,%d,%f,%f,%f,%s\n",current_navaid->id,current_navaid->type,current_navaid->frequency,latDif,lonDif,current_navaid->heading,current_navaid->name.c_str());
        }
        current_navaid=current_navaid->next;
        count++;
    }
    if(current_navaid==NULL){
        if(localNavaids.size()>0){
            lastUpdatelat=lat;
            lastUpdatelon=lon;
            skipNaviads=true;
            firstPass=false;
            //printf("completed pass\n");
        }
    }
    
    /*
   
    for (auto x : localNavaids) {
        NavAid* val=x.second;
        nVdata[count]=json::array({val->id,val->type,val->frequency,val->heading,val->latitude,val->longitude,val->name});
        count++;
    }

    
    localNavaidString=nVdata.dump();*/
}
void XTLuaDataRefs::updateFloatDataRefs(){
    //std::unordered_map<std::string, XTLuaFloat> incomingFloatdataRefs;
    bool allGet=((simTime-beginFlightTime)<10);
    std::unordered_map<std::string,std::vector<XTLuaArrayFloat*> > changedList=changeddataRefs;
    if(allGet){
        printf("all get active\n");
        changedList=floatdataRefs;
    }
    for (auto x : changedList) {
        //int i=0;
        string name=x.first;
        std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
        if(val.size()==1){
            float v=val[0]->value;
            if((val[0]->get||allGet)&&!val[0]->set){

                
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
                //printf("set float %s[0] = %f(%d)\n",x.first.c_str(),v,val.size());
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
            long size=val.size();
            if(val[0]->type == xplmType_Int)
            {
                std::vector<int> inVals(size); 
                std::vector<int> outVals(val.size());
                for(unsigned int i=0;i<val.size();i++){
                    if(val[i]->get||allGet){
                        hasGetUpdate=true;
                    }
                    val[i]->get=false;
                    inVals[i]=val[i]->value;
                }
                //int size=XPLMGetDatavf(val[0]->ref,NULL,0,0);
                //printf("update array int %p get = %d\n",val[i]->ref,hasGetUpdate);
                if(hasGetUpdate)
                    XPLMGetDatavi(val[0]->ref,inVals.data(),0,(int)size);
                
                for(long i=0;i<size;i++){
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
                    XPLMSetDatavi(val[0]->ref,outVals.data(),0,(int)val.size());
                    
                }
            }
            else
            {
                for(long nz=0;nz<size;nz+=32){
                    long start=nz;
                    long end=nz+32;
                    if(end>size)
                        end=(long)size;
                        
                    long length=end-start;   
                    std::vector<float> inVals(length); 
                    std::vector<float> outVals(length);

                    for(long i=start;i<end;i++){
                        if(val[i]->get||allGet){
                            hasGetUpdate=true;
                        }
                        val[i]->get=false;
                        inVals[i-start]=val[i]->value;
                    }
                    //int size=XPLMGetDatavf(val[0]->ref,NULL,0,0);
                    if(hasGetUpdate)
                        XPLMGetDatavf(val[0]->ref,inVals.data(),(int)start,(int)length);
                    
                    for(long i=start;i<end;i++){
                        if(val[i]->set){
                                outVals[i-start]=val[i]->value;
                                val[i]->set=false;
                                hasSetUpdate=true;
                        }
                        else
                        {
                             if(hasGetUpdate){
                                    val[i]->value=inVals[i-start];
                                    outVals[i-start]=inVals[i-start];
                            }else{
                                outVals[i-start]=val[i]->value;
                            }
                        }
                            
                        // printf("set array float %p %s[%d] = %f(%d=%d)\n",val[i]->ref,x.first.c_str(),i,val[i]->value,val.size(), val[i]->value);
                    }
                    if(hasSetUpdate)
                        XPLMSetDatavf(val[0]->ref,outVals.data(),(int)start,(int)length);
                }
            }
            
            
            

        }
    }
    changeddataRefs.clear();

}

double XTLuaDataRefs::XTGetElapsedTime(){
    //data_mutex.lock();
    double retVal=timeT;
    //data_mutex.unlock();
    return retVal;

}
void XTLuaDataRefs::refreshAllDataRefs(){
    //printf("refreshAllDataRefs\n");
    for (auto x : stringdataRefs) {
        XTLuaCharArray* val=x.second;    
           int size=XPLMGetDatab(val->ref,NULL,0,0);
            //printf("getting %d is %d\n",val->ref,size);
           if(size>0) {
            std::vector<char> inVals(size);
            XPLMGetDatab(val->ref,&inVals[0],0,size);
            val->value=std::string(inVals.begin(),inVals.end());
             //printf("got %s is %d\n",val->value.c_str(),size);
           }
            val->get=false;     
    }
    for (auto x : floatdataRefs) {
        //int i=0;
        string name=x.first;
        std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
        if(val.size()==1){
            //float v=val[0]->value;          
            float newVal=XPLMGetDataf(val[0]->ref);
            if(val[0]->type == xplmType_Double){
                 newVal=XPLMGetDatad(val[0]->ref);
                 
            }
            else if(val[0]->type == xplmType_Int){
                 newVal=XPLMGetDatai(val[0]->ref);
            }
            val[0]->get=false;
            val[0]->value=newVal;
  
        }
        else{
            bool hasSetUpdate=false;
            bool hasGetUpdate=false;
            int size=(int)val.size();
            if(val[0]->type == xplmType_Int)
            {
                std::vector<int> inVals(size); 
                std::vector<int> outVals(val.size());
                hasGetUpdate=true;
                for(unsigned int i=0;i<val.size();i++){
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
                    XPLMSetDatavi(val[0]->ref,outVals.data(),0,(int)val.size());
                    
                }
            }
            else
            {
                std::vector<float> inVals(size); 
                std::vector<float> outVals(val.size());
                hasGetUpdate=true;
                for(unsigned int i=0;i<val.size();i++){
                    val[i]->get=false;
                    inVals[i]=val[i]->value;
                }
                //int size=XPLMGetDatavf(val[0]->ref,NULL,0,0);
                if(hasGetUpdate)
                    XPLMGetDatavf(val[0]->ref,inVals.data(),0,size);           
                
                for(int i=0;i<size;i++){
                            if(hasGetUpdate){
                                val[i]->value=inVals[i];
                                outVals[i]=inVals[i];
                            }else{
                                outVals[i]=val[i]->value;
                            }
                        
                }
                if(hasSetUpdate)
                    XPLMSetDatavf(val[0]->ref,outVals.data(),0,(int)val.size());
            }
        }
    }
}
void XTLuaDataRefs::updateDataRefs(){
    data_mutex.lock();
    //printf("updateDataRefs\n");
        //timeT = XPLMGetElapsedTime();
        isPaused=XPLMGetDatai(paused_ref);
        simTime=XPLMGetDataf(sim_time_ref);
        //printf("time now %f %f\n",timeT,simTime);
        timeT = simTime;
        if(updateRoll==0&&(XPLMGetDatai(replay_ref) == 0))
        {
            updateNavDataRefs();
            
        }
        else if(updateRoll==5)
        {
            updateStringDataRefs();
            updateRoll=-1;//will go back to 0 next line
        }
        //else if(updateRoll==2)
        {
            updateFloatDataRefs();
            updateCommands(); //always do command queue
            
        } 
        updateRoll++;
        serialWindow.show();
    data_mutex.unlock();
}
void XTLuaDataRefs::cleanup(){
    data_mutex.lock();
    drefResolveQueue.clear();
    cmdResolveQueue.clear();
    cmdHandlerResolveQueue.clear();
     for (auto x : floatdataRefs) {
        //int i=0;
        string name=x.first;
        std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
        for(XTLuaArrayFloat* k:val){
            delete k;
        }
     }
     floatdataRefs.clear();
     for (auto x : stringdataRefs) {
       // int i=0;
        string name=x.first;
        XTLuaCharArray* val=stringdataRefs[name];
        delete val;
        
     }
     stringdataRefs.clear();
     NavAid* cNav=navaids;
     while(cNav!=NULL){
         navaids=navaids->next;
         delete cNav;
         cNav=navaids;
     }
     for(XTControlObject* c:controlOverrides){
         delete c;
     }

     navaids=NULL;
     lastnavaid=NULL;
     current_navaid=NULL;
     changeddataRefs.clear();
     localNavaids.clear();
     controlOverrides.clear();
     updateRoll=0;
     printf("XTLua:Cleaned up data\n");
    data_mutex.unlock();
}


void XTLuaDataRefs::XTqueueresolve_dref(xtlua_dref * d){
    data_mutex.lock();
    //printf("will resolve %s\n",d->m_name.c_str());
    drefResolveQueue.push_back(d);//this needs to be done on another thread
    data_mutex.unlock();
}
void XTLuaDataRefs::XTqueueresolve_cmd(xtlua_cmd * d){
    data_mutex.lock();
    cmdResolveQueue.push_back(d);//this needs to be done on another thread
    data_mutex.unlock();
}
 



int XTLuaDataRefs::resolveQueue(){
    int retVal=0;
    data_mutex.lock();
    if(paused_ref==NULL)
        paused_ref=XPLMFindDataRef("sim/time/paused");
    if(replay_ref==NULL)
        replay_ref=XPLMFindDataRef("sim/time/is_in_replay");    
    if(sim_time_ref==NULL){
        sim_time_ref = XPLMFindDataRef("sim/time/total_running_time_sec");  
        beginFlightTime=XPLMGetDataf(sim_time_ref);
    }
    //printf("XTLua:Resolving queue\n");
    for(xtlua_dref * d:drefResolveQueue){
        //printf("Resolving dref %s\n",d->m_name.c_str());
        if(d->m_name.rfind("xtlua/", 0) == 0){
            d->m_types =xplmType_Data;
            continue;
        }
        assert(d->m_dref == NULL);
        assert(d->m_types == 0);
        assert(d->m_index == -1);
        assert(d->m_ours == 0);
        grabLocal(d);
        /*if(d->m_ours){
            printf("Resolved local dref %s\n",d->m_name.c_str());
            continue;
        }*/
       // if(!d->m_ours)
        //    printf("Not local dref %s\n",d->m_name.c_str());
        d->m_dref = XPLMFindDataRef(d->m_name.c_str());
        //initialise our datasets
        if(d->m_dref)
        {
            d->m_index = -1;
            d->m_types = XPLMGetDataRefTypes(d->m_dref);
            //printf("Resolved dref %s to %p as %d\n",d->m_name.c_str(),d->m_dref,d->m_types);
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
                    //printf("Resolved int array dref %s to %p with %d\n",d->m_name.c_str(),d->m_dref,size);
                }
                else{
                    
                    size=XPLMGetDatavf(d->m_dref,NULL,0,0);
                    //printf("Resolved float array dref %s to %p with %d\n",d->m_name.c_str(),d->m_dref,size);
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
                   // printf("defaulted array dref %s to %f with %d\n",d->m_name.c_str(),v->value,size);  
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

                        }
                    }
                }
            }
        }

    }

    drefResolveQueue.clear();

    for(xtlua_cmd * d:cmdResolveQueue){
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
std::vector<xtlua_cmd*> XTLuaDataRefs::XTGetHandlers(){
    std::vector<xtlua_cmd*> retval;
    for (auto x : cmdHandlerResolveQueue) {
        xtlua_cmd * cmd=x.second;
        retval.push_back(cmd);
    }
    return retval;
}
float XTLuaDataRefs::XTGetDataf(
                                   xtlua_dref * d,bool local){

    float retVal=0;
    
    if(d->m_ours){
        xlua_dref_ours(d->local_dref);
        retVal=xlua_dref_get_number(d->local_dref);
        //data_mutex.unlock();
        //return retVal; 
    }
    
    
    XPLMDataRef  inDataRef=d->m_dref;
    
   // if(!local)
     //   retVal=XPLMGetDataf(inDataRef);
    char namec[32]={0};
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    data_mutex.lock();                       
    if(floatdataRefs.find(name)!=floatdataRefs.end()){
        std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
        if(!d->m_ours){
            val[0]->get=true;
            changeddataRefs[name]=val;
            retVal=val[0]->value;
        }
        else{
            val[0]->value=retVal;
        }
        //floatdataRefs[name]=val;
    }
    else{
        printf("didn't initialise %s\n",name.c_str());


        

    }
    
    data_mutex.unlock();
    return retVal;                                   
}
void  XTLuaDataRefs::XTSetDataf(
                                   xtlua_dref * d,    
                                   float                inValue,bool local)
{
    //printf("set %p=%f\n",inDataRef,inValue) ;
    //if(!local)
     //   XPLMSetDataf(inDataRef,inValue);
     //data_mutex.lock();
     /*if(d->m_ours){
        xlua_dref_ours(d->local_dref);
        xlua_dref_set_number(d->local_dref,inValue);
        //data_mutex.unlock();
        return; 
    }*/
     data_mutex.lock();
     
     XPLMDataRef          inDataRef=d->m_dref;
    char namec[32]={0};
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    if(floatdataRefs.find(name)!=floatdataRefs.end()){
        std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
        if(val[0]->value!=inValue){
            if(!d->m_ours){
                val[0]->set=true;
                changeddataRefs[name]=val;
                val[0]->value=inValue;
            }
            else{
                xlua_dref_set_number(d->local_dref,inValue);
                val[0]->value=inValue;
            }
        }
    }
    else{
        printf("didn't initialise %s\n",name.c_str());

    }

    
    data_mutex.unlock();
}

int XTLuaDataRefs::XTGetDatab(
                                   xtlua_dref * d,    
                                   void *               outValue,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMaxBytes,bool local)
{
    char *outValues =(char *)outValue;
    data_mutex.lock();
    if(d->m_name.rfind("xtlua/controlObject", 0) == 0){
        printf("cant read control object\n");
        data_mutex.unlock();
        return 0;
    }
    if(d->m_name.rfind("xtlua/navaids", 0) == 0){
       // std::string tS="testString";
        //printf("reading navaids %d\n",localNavaidString.length());
         skipNaviads=false;
         if(outValues!=NULL){
             const char * charArray=localNavaidString.c_str();
                for(unsigned int i=inOffset;i<localNavaidString.length()&&i-inOffset<(unsigned int)inMaxBytes;i++){
                    outValues[i-inOffset]=charArray[i];
               }
         }
         else{
             localNavaidString=incomingNavaidString;
         }
         int retVal=(int)localNavaidString.length();
        data_mutex.unlock();
        return retVal;
    }
    if(d->m_name.rfind("xtlua/fms", 0) == 0){
       // std::string tS="testString";
        //printf("reading navaids %d\n",localNavaidString.length());
        skipNaviads=false;
         if(outValues!=NULL){
             
             
             const char * charArray=localFMSString.c_str();
                for(unsigned int i=inOffset;i<localFMSString.length()&&i-inOffset<(unsigned int)inMaxBytes;i++){
                    outValues[i-inOffset]=charArray[i];
               }
         }
         else{
             //if(localFMSString!=incomingFMSString)
             //   printf("FMS=%s\n",incomingFMSString.c_str());
             localFMSString=incomingFMSString;
         }
         int retVal=(int)localFMSString.length();
        data_mutex.unlock();
        return retVal;
    }
    
    /*if(d->m_ours){
        //data_mutex.unlock();
        printf("Our ref XTGetDatab not implimented\n ");
        data_mutex.unlock();
        return 0; 
    }*/
    
    //outValue will be an array of chars
    XPLMDataRef  inDataRef=d->m_dref;
    
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
                if(!d->m_ours){
                    val->get=true;
                   
                }
                for(unsigned int i=inOffset;i<val->value.length()&&i-inOffset<(unsigned int)inMaxBytes;i++){
                    outValues[i-inOffset]=charArray[i];
                    retVal++;
                    //printf("apply XTGetDatavf %s %s[%d/%d] %s = %f\n",d->m_name.c_str(),name.c_str(),inOffset,inMax,outValues!=NULL?"values":"size",val[i]->value);
                }
            }
        }
        else
        {
             if(stringdataRefs.find(name)!=stringdataRefs.end()){
                XTLuaCharArray* val=stringdataRefs[name];
                retVal=(int)val->value.size();
                if(!d->m_ours)
                    val->get=true;

            }
            
        }
        
    }
    data_mutex.unlock();
    return retVal;
}

void XTLuaDataRefs::XTSetDatab(
                                   xtlua_dref * d,    
                                   std::string value)
{
    
    //char *inValues =(char *)inValue;
    if(d->m_name.rfind("xtlua/", 0) == 0){
        if(d->m_name.rfind("xtlua/getserial", 0) == 0){
                printf("get serial %s\n",value.c_str());
                serialWindow.init(value);//open a serial entry window
                return;
        }
        else if(d->m_name.rfind("xtlua/controlObject", 0) == 0){
            printf("creating control override object %s\n",value.c_str());
            XTControlObject* override=new XTControlObject();
            override->data=value;
            data_mutex.lock();
            controlOverrides.push_back(override);
            data_mutex.unlock();
            return;
        }
        else if(d->m_name.rfind("xtlua/fltpln", 0) == 0){
            printf("setting flight plan %s\n",value.c_str());
            json fpData=json::parse(value.c_str());
            std::vector<json> waypoints=fpData.get<std::vector<json>>();
            
            int currentCount=XPLMCountFMSEntries();
            printf("%d existing entries\n",currentCount);
            printf("becoming %d entries\n",(int)waypoints.size());
            for (int i=0;i<currentCount&&XPLMCountFMSEntries()>0;i++){
                XPLMClearFMSEntry(i);
                //printf("clear to %d existing entries\n",currentCount);
            }
            for(unsigned int i=0;i<waypoints.size();i++){
                std::vector<double> waypoint=waypoints[i].get<std::vector<double>>();
                printf("%d got waypoint %d\n",i,(int)waypoint.size());
                XPLMSetFMSEntryLatLon(i,(float)waypoint[0],(float)waypoint[1],(float)waypoint[2]);
            }
            printf("got flight plan %d\n",(int)waypoints.size());
            return;
        }
    }
    if(d->m_ours){
        //data_mutex.lock();
        xlua_dref_ours(d->local_dref);
        xlua_dref_set_string(d->local_dref,string(value));
        //data_mutex.unlock();
        //printf("set string %s\n",value.c_str());
        //return; 
    }
    char namec[32];
    XPLMDataRef  inDataRef=d->m_dref;
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    data_mutex.lock();
    //printf("apply XTSetDatab %s[%d]\n",d->m_name.c_str(),inLength);
    //if(inValues!=NULL)
    {

            XTLuaCharArray* val=stringdataRefs[name];
            val->value=value;
            if(!d->m_ours){
                val->set=true;
                 
            }


    }
    data_mutex.unlock();
}      
int XTLuaDataRefs::XTGetDatavf(
                                   xtlua_dref * d,    
                                   float *              outValues,    /* Can be NULL */
                                   int                  inOffset,    
                                   int                  inMax,bool local)
{
    int retVal=0;
    /*if(d->m_ours){
        if(inMax>1)
            printf("Our ref XTGetDatavf >1 not implimented\n ");

        xlua_dref_ours(d->local_dref);
        if(outValues!=NULL){
            //data_mutex.lock();
            outValues[0]=xlua_dref_get_array(d->local_dref,inOffset);
            //data_mutex.unlock();
            //return 1;
        }
        
        //return xlua_dref_get_dim(d->local_dref);
    }*/
    data_mutex.lock();
    //outValue will be set to an array of floats
    char namec[32];
    XPLMDataRef  inDataRef=d->m_dref;
    sprintf(namec,"%p",inDataRef);
    std::string name=namec;
    
    
    
     
    
    {
        if(outValues!=NULL){
            //if(!d->m_ours)
            if(floatdataRefs.find(name)!=floatdataRefs.end()){
                std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
                bool got=false;
                for(unsigned int i=inOffset;i<val.size()&&i-inOffset<(unsigned int)inMax;i++){
                     if(!d->m_ours){
                        outValues[i-inOffset]=val[i]->value;
                        val[i]->get=true;
                        got=true;
                     }
                     else{
                         outValues[i-inOffset]=xlua_dref_get_array(d->local_dref,i);
                         val[i]->value=outValues[i-inOffset];
                     }
                    retVal++;
                    //printf("apply XTGetDatavf %s %s[%d/%d] %s = %f\n",d->m_name.c_str(),name.c_str(),inOffset,inMax,outValues!=NULL?"values":"size",val[i]->value);
                }
                if(got)
                 changeddataRefs[name]=val;
            }
        }
        else
        {
            if(floatdataRefs.find(name)!=floatdataRefs.end()){
                std::vector<XTLuaArrayFloat*> val=floatdataRefs[name];
                retVal=(int)val.size();
                //data_mutex.unlock();
                //return retVal;
            }
 
        }
        
    }
    
    data_mutex.unlock();
    return retVal;

}

void XTLuaDataRefs::XTSetDatavf(
                                   xtlua_dref * d,    
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
            if(!d->m_ours){
                if(val[index]->value!=inValue)
                    val[index]->set=true;
                    changeddataRefs[name]=val;
            }
            else{
                xlua_dref_set_array(d->local_dref,index,inValue);
            }
            val[index]->value=inValue;
            
          //printf("apply XTSetDatavf single %s %s[%d] = %f\n",d->m_name.c_str(),name.c_str(),index,val[index]->value); 
        }
        else if(!d->m_ours){
            //printf("apply XTSetDatavf overflow %s %s[%d] = %f\n",d->m_name.c_str(),name.c_str(),index,inValue); 
            int n=(int)val.size();
            for(;n<index;n++){
                XTLuaArrayFloat* v=new XTLuaArrayFloat;
                v->ref=d->m_dref;
                v->type=xplmType_Float;
                v->value=0.0;
                v->get=true;
                v->index=n;
                floatdataRefs[name].push_back(v);
            }
            XTLuaArrayFloat* v=new XTLuaArrayFloat;
            v->ref=d->m_dref;
            v->type=xplmType_Float;
            v->value=inValue;
            v->set=true;
            //printf("did apply XTSetDatavf overflow %s %s[%d] = %f\n",d->m_name.c_str(),name.c_str(),n,inValue); 
            v->index=n;
            floatdataRefs[name].push_back(v);
        }
    }
    
    data_mutex.unlock();
    
}    


                        

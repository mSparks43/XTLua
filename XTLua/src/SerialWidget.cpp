
#include "XPStandardWidgets.h"
#include "XPWidgets.h"
#include "SerialWidget.h"
#include "XPLMDataAccess.h"
#include "XPLMPlugin.h"
static  XPWidgetID	w_window;
static XPWidgetID setButton;
static XPWidgetID notificationCaption;
XPWidgetID serialField[6];
std::string windowsetup;
SerialWidget::SerialWidget(){

}

/*static float	displayserialwindow(
                                   float                inElapsedSinceLastCall,    
                                   float                inElapsedTimeSinceLastFlightLoop,    
                                   int                  inCounter,    
                                   void *               inRefcon); */

static int serialwindowHandler(
						XPWidgetMessage			inMessage,
						XPWidgetID				inWidget,
						intptr_t				inParam1,
						intptr_t				inParam2);
void SerialWidget::init(std::string value){
    windowsetup=value;
    
    this->windowsettings=json::parse(value.c_str());
   // 
    //printf("windowsettings=%s\n",this->windowsettings.dump().c_str());
    //printf("dRef=%s\n",this->windowsettings.value("dref","unknown").c_str());
    doShow=true;
}
std::string SerialWidget::getdRef(){
    //printf("parse=%s\n",windowsetup.c_str());
    this->windowsettings=json::parse(windowsetup.c_str());
    return this->windowsettings["dref"].get<std::string>();
} 
std::string SerialWidget::getTitle(){
    //printf("parse=%s\n",windowsetup.c_str());
    this->windowsettings=json::parse(windowsetup.c_str());
    return this->windowsettings["title"].get<std::string>();
}  
std::string SerialWidget::getKey(){
    //printf("parse=%s\n",windowsetup.c_str());
    this->windowsettings=json::parse(windowsetup.c_str());
    return this->windowsettings["key"].get<std::string>();
}                   
void SerialWidget::show(){
    if(doShow==false)
        return;
    doShow=false; 
    XPLMDataRef sDref = XPLMFindDataRef (serialWindow.getdRef().c_str());
    if(sDref!=NULL){
        std::string serial=serialWindow.getKey();
        XPLMSetDatab(sDref,(void *)serial.c_str(),0,(int)serial.size());
        char activation_Text[1024]={0};
        //nt size=
        XPLMGetDatab(sDref,activation_Text,0,1024);
        //printf("startup is activated text=%s\n",activation_Text);
        if((std::string(activation_Text)).compare("true")==0){
             return;
        }
    }
    else
        return;    
    int x2 = x + w;
	int y2 = y - h;
    w_window = XPCreateWidget(x, y, x2, y2,
					1,	// Visible
					"Enter Serial Number",	// desc
					1,		// root
					NULL,	// no container
					xpWidgetClass_MainWindow);
    XPSetWidgetProperty(w_window, xpProperty_MainWindowHasCloseBoxes, 1);
    XPSetWidgetProperty(w_window, xpProperty_MainWindowType, xpMainWindowStyle_Translucent);
    XPAddWidgetCallback(w_window, serialwindowHandler);  
    setButton = XPCreateWidget(x2-80, y-70, x2-20, y-90,
					1, "Save", 0, w_window,
					xpWidgetClass_Button);
    XPSetWidgetProperty(setButton, xpProperty_ButtonType, xpPushButton); 

    for(int i=0;i<6;i++){
        serialField[i] = XPCreateWidget(x+10+i*50, y-45, x+50+i*50, y-65,
                        1, "", 0, w_window,
                        xpWidgetClass_TextField);
        XPSetWidgetProperty(serialField[i], xpProperty_TextFieldType, xpTextEntryField);
        XPSetWidgetProperty(serialField[i], xpProperty_Enabled, 1);  
    } 
    std::string notifyText=getTitle()+" Not Activated";
    notificationCaption =XPCreateWidget(x+10, y-70, x+160, y-100,
					1, notifyText.c_str(), 0, w_window,
					xpWidgetClass_Caption);  
    XPSetWidgetProperty(notificationCaption, xpProperty_CaptionLit, 1);                                    
}

int serialwindowHandler(
						XPWidgetMessage			inMessage,
						XPWidgetID				inWidget,
						intptr_t				inParam1,
						intptr_t				inParam2){

    if (inMessage == xpMessage_CloseButtonPushed)
	{
		XPDestroyWidget(w_window, 1);

		return 1;
	}
    if (inMessage == xpMsg_PushButtonPressed)
	{
        std::string serial;
        for(int i=0;i<6;i++){
            char buffer[255]={0};//,jvmBuffer[255];
            XPGetWidgetDescriptor(serialField[i], buffer, sizeof(buffer));
            std::string str(buffer);
            serial+=str+"-";
        }
        //printf("%s\n%s\n",serialWindow.getdRef().c_str(),serial.c_str());
        XPLMDataRef sDref = XPLMFindDataRef (serialWindow.getdRef().c_str());
        if(sDref!=NULL){
            XPLMSetDatab(sDref,(void *)serial.c_str(),0,(int)serial.size());
            char activation_Text[1024]={0};
            //int size=
            XPLMGetDatab(sDref,activation_Text,0,1024);
            //printf("is activated text=%s\n",activation_Text);
            if((std::string(activation_Text)).compare("true")==0){
                XPDestroyWidget(w_window, 1);
                FILE *fptr;
                char path_to_me_c[2048];
                XPLMGetPluginInfo(XPLMGetMyID(), NULL, path_to_me_c, NULL, NULL);
                
                // Plugin base path: pop off two dirs from the plugin name to get the base path.
                std::string plugin_base_path=std::string(path_to_me_c);
                std::string::size_type lp = plugin_base_path.find_last_of("/\\");
                plugin_base_path.erase(lp);
                lp = plugin_base_path.find_last_of("/\\");
                plugin_base_path.erase(lp+1);
                plugin_base_path+="serial.bin";
                printf("save serial to %s\n",plugin_base_path.c_str());
                fptr=fopen(plugin_base_path.c_str(),"w");
                fprintf(fptr,"%s",serial.c_str());
                fclose (fptr);
                return 1;
            }
        }
        
        XPSetWidgetDescriptor(notificationCaption,"Failed");
        
        /*char serialbuffer[255]={0};//,jvmBuffer[255];
        XPGetWidgetDescriptor(ipField, ipbuffer, sizeof(ipbuffer));
        if (inParam1 == (intptr_t)setButton)
        {
            settings.setIP(ipbuffer);
            int tmp = (int)XPGetWidgetProperty(audioDeviceField, xpProperty_ScrollBarSliderPosition, NULL);
            settings.testAudioDevice(tmp);
            return 1;
        }*/
    }
     return 0;                  
}

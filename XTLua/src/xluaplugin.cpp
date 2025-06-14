//not for use inside a module

#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include "xluaplugin.h"

#include "XPLMMenus.h"
#include <string>

XPLMMenuID				PluginMenu = 0;
XPLMCommandRef			reload_cmd = nullptr;

bool file_exists(const std::string &name)
	{
    if (FILE *file = fopen(name.c_str(), "r"))
    {
        fclose(file);
        return true;
    }
    else
    {
        return false;
    }
	}
bool isDebugInstance(){

    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);
	char buf[2048];
	char dirchar = *XPLMGetDirectorySeparator();
	XPLMGetPluginInfo(XPLMGetMyID(), NULL, buf, NULL, NULL);
	printf("XTLua: I am: %s\n", buf);
	char* p = buf;
	char* slash = p;
	while (*p)
	{
		if (*p == dirchar) slash = p;
		++p;
	}
	++slash;
	*slash = 0;
	printf("XTLua: buf now: %s\n", buf);
	strcat(buf, "xtlua_debugging.txt");
	return  file_exists(buf);
}
	
#if IBM
	#include <Windows.h>
	#include <Wincon.h>
	
	
	BOOL APIENTRY DllMain(IN HINSTANCE dll_handle, IN DWORD call_reason, IN LPVOID reserved)
	{
			if (isDebugInstance())
			{
				BOOL chk = AllocConsole();
				if (chk)
				{
					freopen("CONOUT$", "w", stdout);
					printf("XTLua: printing to console\n");
					//ShowWindow(GetConsoleWindow(), SW_MINIMIZE);
				}
			}	
		//}
		return TRUE;
	}
#endif
#ifdef __linux__
#include <X11/Xlib.h>
static bool checkdisplayServer(){
	Display *display;
	int opcode, event,error;
	display = XOpenDisplay(NULL);
	if(!display)
		return true;//probably headless
	bool retVal=true;
	if(XQueryExtension(display,"XWAYLAND",&opcode, &event,&error)){
		XPLMDebugString("ERROR: XWayland support depreciated, please switch to a full xserver\n");
		retVal=false;
	}
	XCloseDisplay(display);
	return retVal;
}
#endif
int id=1;
enum eMenuItems : int
{
	MI_ResetState,
};
static void MenuHandler(void* menuRef, void* itemRef)
{
	switch ((eMenuItems)(size_t)itemRef)
	{
		case MI_ResetState:
			reloadScripts(reload_cmd, xplm_CommandBegin, nullptr);
			break;
	}
}
PLUGIN_API int XPluginStart(
						char *		outName,
						char *		outSig,
						char *		outDesc){

    //strcpy(outName, "XTLua " PLUGINVERSION);
	//char path_to_me_c[2048];
	//XPLMGetPluginInfo(XPLMGetMyID(), NULL, path_to_me_c, NULL, NULL);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	sprintf(outName, "XTLua %s id%d", PLUGINVERSION, XPLMGetMyID());
#pragma GCC diagnostic pop
    //strcpy(outSig, "com.x-plane.xtlua." VERSION);
    strcpy(outDesc, "A minimal scripting environment for aircraft authors with multithreading.");
	bool isDebugMode=isDebugInstance();	
	printf("XTLua being started %d %d\n", XPLMGetMyID(),isDebugMode);
#ifdef __linux__
	if(!checkdisplayServer()){
		return 0;
	}
#endif
    if(isDebugMode){
		reload_cmd = XPLMCreateCommand("xtlua/reload_all_scripts", "Reload scripts and state for ");
		if (reload_cmd != nullptr)
		{
			XPLMRegisterCommandHandler(reload_cmd, reloadScripts, 1,  (void *)0);
		}
		int item = XPLMAppendMenuItem(XPLMFindPluginsMenu(), outName, nullptr, 0);
		PluginMenu = XPLMCreateMenu(outName, XPLMFindPluginsMenu(), item, MenuHandler, nullptr);
		XPLMAppendMenuItem(PluginMenu, "Reload Scripts", (void*)MI_ResetState, 0);
	}
    return XTLuaXPluginStart(outSig);


}

PLUGIN_API void	XPluginStop(void)
{
    XTLuaXPluginStop();
}

PLUGIN_API void XPluginDisable(void)
{
    XTLuaXPluginDisable();
}

PLUGIN_API int XPluginEnable(void)
{
    return XTLuaXPluginEnable();
}

PLUGIN_API void XPluginReceiveMessage(
					XPLMPluginID	inFromWho,
					int				inMessage,
					void *			inParam)
{
    XTLuaXPluginReceiveMessage(inFromWho,inMessage,inParam);
}

//not for use inside a module
#include "xluaplugin.h"
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#if IBM
	#include <Windows.h>
	#include <Wincon.h>
	#include <string>
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
	BOOL APIENTRY DllMain(IN HINSTANCE dll_handle, IN DWORD call_reason, IN LPVOID reserved)
	{
		
		char buf[2048];
		char dirchar = *XPLMGetDirectorySeparator();
		XPLMGetPluginInfo(XPLMGetMyID(), NULL, buf, NULL, NULL);
		//printf("XTLua: I am: %s\n", buf);
		char* p = buf;
		char* slash = p;
		while (*p)
		{
			if (*p == dirchar) slash = p;
			++p;
		}
		++slash;
		*slash = 0;
		//printf("XTLua: buf now: %s\n", buf);
		strcat(buf, "xtlua_debugging.txt");
		//if (file_exists(buf)){
			
			
			if (file_exists(buf))
			{
				BOOL chk = AllocConsole();
				if (chk)
				{
					freopen("CONOUT$", "w", stdout);
					printf("XTLua: printing to console %s\n", buf);
					//ShowWindow(GetConsoleWindow(), SW_MINIMIZE);
				}
			}
			
		//}
		return TRUE;
	}
#endif
PLUGIN_API int XPluginStart(
						char *		outName,
						char *		outSig,
						char *		outDesc){

    //strcpy(outName, "XTLua " PLUGINVERSION);
	sprintf(outName, "XTLua %s id%d", PLUGINVERSION, rand() % 100 + 1);
    //strcpy(outSig, "com.x-plane.xtlua." VERSION);
    strcpy(outDesc, "A minimal scripting environment for aircraft authors with multithreading.");
	printf("XTLua being started\n");
    
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
//not for use inside a module
#include "xluaplugin.h"
#include <stdio.h>
#include <string.h>

#if IBM
	#include <Windows.h>
	#include <Wincon.h>
	char CONFIG_DEBUG[] = "xtlua_debugging.txt";
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
		if (file_exists(CONFIG_DEBUG)){
			BOOL chk = AllocConsole();
			if (chk)
			{
				freopen("CONOUT$", "w", stdout);
				printf("XTLua: printing to console\n");
				//ShowWindow(GetConsoleWindow(), SW_MINIMIZE);
			}
		}
		return TRUE;
	}
#endif
PLUGIN_API int XPluginStart(
						char *		outName,
						char *		outSig,
						char *		outDesc){

    strcpy(outName, "XTLua " PLUGINVERSION);
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
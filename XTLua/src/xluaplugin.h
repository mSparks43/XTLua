//not for use inside a module
#include <XPLMPlugin.h>
#include <XPLMUtilities.h>
#define PLUGINVERSION "2.0.3b1"
int     XTLuaXPluginStart(char *		outSig);
void	XTLuaXPluginStop(void);
void XTLuaXPluginDisable(void);
int XTLuaXPluginEnable(void);
void XTLuaXPluginReceiveMessage(
					XPLMPluginID	inFromWho,
					int				inMessage,
					void *			inParam);
int reloadScripts(XPLMCommandRef c, XPLMCommandPhase phase, void * ref);

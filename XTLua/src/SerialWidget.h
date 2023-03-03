#include "XPWidgets.h"
#include <string>
#include "json/json.hpp"

using nlohmann::json;
 class SerialWidget
{
private:
    
   
    
    
    
    bool doShow=false;
    int x=100;
    int y=500;
    int w=350;
    int h=100;
    json windowsettings;
    
    /*static int	SettingsWidgetsHandler(
						XPWidgetMessage			inMessage,
						XPWidgetID				inWidget,
						intptr_t				inParam1,
						intptr_t				inParam2);*/
public:
    
    SerialWidget();
    void init(std::string value);
    void show();
    std::string getdRef();
    std::string getTitle();
    std::string getKey();
};

static SerialWidget serialWindow;
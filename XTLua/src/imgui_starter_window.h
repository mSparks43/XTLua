/*
 *   Imgui Starter Window for X-Plane
 *   William Good
 *
 *   This is a templete to allow you to use Imgui with X-Plane
 *
 *
 *
 *
 */
#ifndef SRC_IMGUIWIDGET_H_
#define SRC_IMGUIWIDGET_H_

#include "../src/ImgWindow/ImgWindow.h"
#include <vector>

// Configure one-time setup like fonts
void configureImgWindow();
// Cleanup one-time setup
void cleanupAfterImgWindow();

// Our own class defining our own UI
class ImguiWidget: public ImgWindow {
protected:
    // texture number and size of the image we want to show
    static int      image_id;
    static ImVec2   image_size;
    // Counter for the number of windows opened
    static int      num_win;
    // I am window number...
    const int       myWinNum;
    // Note to myself that a change of window mode is requested
    XPLMWindowPositioningMode nextWinPosMode = -1;
    // Our flight loop callback in case we need one
    XPLMFlightLoopID flId = nullptr;
    // Values in node "Buttons" / "Checkboxes"
    bool makeRed = false;
    int         radioChoice = 1;
    // Values in node "Sliders"
    float       sliderVal1  = 0.123f;
    float       sliderVal2  = 0.0f;
    int         sliderVal3  = 50;
    float       sliderAngle = 0.0f;
    // Values in node "ComboBox"
    int         choice1     = 1;
    int         choice2     = 1;
    // Values in node "Input"
    std::string userText;
    int         userI1 = 123;
    int         userI2 = 1234;
    // Values in node "List"
    std::vector<const char*> listContent;
    int         listSelItem = 0;
    void (*guiFunc_ptr)(void);
    // Structure and data used for displaying a table
public:
    struct tableDataTy {
        std::string     reg;
        std::string     model;
        std::string     typecode;
        std::string     owner;
        float           heading = 0.0f;
        bool            turnsLeft = false;
        bool            filtered = true;    // included in search result?
        
        // is s (upper cased!) in any text?
        bool contains (const std::string& s) const;
        
    };
    typedef std::vector<tableDataTy> tableDataListTy;
protected:
    tableDataListTy     tableList;
public:
    ImguiWidget(int left, int top, int right, int bot,
                XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle,
                XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows,
                void (*guiFunc)(void)=NULL);
    ~ImguiWidget() override;
protected:
    // Main function: creates the window's UI
    void buildInterface() override;

    // flight loop callback for stuff we cannot do during drawing callback
    static float cbFlightLoop(
        float                inElapsedSinceLastCall,
        float                inElapsedTimeSinceLastFlightLoop,
        int                  inCounter,
        void*                inRefcon);
};

//

#endif /* SRC_IMGUIWIDGET_H_ */

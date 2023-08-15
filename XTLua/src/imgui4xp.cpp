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

#define VERSION_NUMBER "1.10 build " __DATE__ " " __TIME__

// All our headers combined
#include "imgui4xp.h"

/// Our "standard" window size
constexpr int WIN_WIDTH     = 800;      ///< window width
constexpr int WIN_HEIGHT    = 450;      ///< window height
constexpr int WIN_PAD       =  75;      ///< distance from left and top border
constexpr int WIN_COLL_OFS  =  30;      ///< offset of collated windows

// --- Global Variables ---

// Is VR enabled?
bool vr_is_enabled = false;

// the ImGui windows we are going to create
// managed in the form of smart pointers.
typedef std::shared_ptr<ImgWindow> ImgWindowSPtrTy;
typedef std::vector<ImgWindowSPtrTy> ImgWindowSPtrVecTy;
ImgWindowSPtrVecTy gWndList;

/// Calculate window's standard coordinates
void CalcWinCoords (int& left, int& top, int& right, int& bottom)
{
    // Screen coordinates
    int screenLeft, screenTop;
    XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, nullptr, nullptr);

    // Coordinates of our window
    left    = screenLeft    + WIN_PAD;
    right   = left          + WIN_WIDTH;
    top     = screenTop     - WIN_PAD;
    bottom  = top           - WIN_HEIGHT;
}

/// Creates another window right inside the list
ImgWindow* AddWindow (void (*guiFunc)(void))
{
    int left, top, right, bottom;
    CalcWinCoords(left, top, right, bottom);
    XPLMWindowDecoration decoration = xplm_WindowDecorationSelfDecoratedResizable;
    XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows; 
    // This creates a ImguiWidget object inside the list,
    // which in turn creates the actual window through its constructor
    std::shared_ptr<ImguiWidget> window=std::make_shared<ImguiWidget>(left, top, right, bottom,
                                                        decoration,
                                                        layer,guiFunc);
    gWndList.emplace_back(window);
    return window.get();
}






void IMGGUIXPluginDisable(void) {
    // Destroy all window object in order to properly clean up.
    // (Can't use ImgWindow::SafeDelete here as that would wait for a
    //  flight loop callback, which won't be delivered any longer.
    //  Delete should be safe here as no rendering is taking place and will no longer.)
    gWndList.clear();
    
    // Cleanup the general stuff
    cleanupAfterImgWindow();
}

int IMGGUIXPluginEnable(void) {
    // Some general ImGui setup
    configureImgWindow();
    
    // Create a first window
   // AddWindow();

    return 1;
}

void IMGGUIXPluginReceiveMessage(XPLMPluginID, int inMessage, void * /*inParam*/) {
    
    switch (inMessage) {
        // Move all windows in or out of VR when VR mode changes
        case XPLM_MSG_ENTERED_VR:
            vr_is_enabled = true;
            // We don't move popped out windows into VR because we think
            // the user didn't want it inside the sim
            for (ImgWindowSPtrTy& pWnd: gWndList) {
                if (!pWnd->IsPoppedOut())
                    pWnd->SetWindowPositioningMode(xplm_WindowVR);
            }
            break;
            
        case XPLM_MSG_EXITING_VR:
            vr_is_enabled = false;
            // Geometry gets lost when moving into VR, so we need to re-position the window now
            int left, top, right, bottom;
            CalcWinCoords(left, top, right, bottom);
            for (ImgWindowSPtrTy& pWnd: gWndList) {
                // If we don't move popped out windows then we need to make
                // sure that we only move VR windows back into the sim,
                // so that a popped out window stays popped out
                if (pWnd->IsInVR()) {
                    pWnd->SetWindowPositioningMode(xplm_WindowPositionFree);
                    pWnd->SetWindowGeometry(left, top, right, bottom);
                    // move the next window a bit to the side and down to see all of them arranged
                    left += WIN_COLL_OFS; right  += WIN_COLL_OFS;
                    top  -= WIN_COLL_OFS; bottom -= WIN_COLL_OFS;
                }
            }
            break;
    }
}

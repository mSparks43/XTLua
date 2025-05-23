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

// All our headers combined
#include "imgui4xp.h"

// Image processing (for reading "imgui_demo.jpg"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//
// MARK: ImGui extension: formatted IDs
//

namespace ImGui {

/// @brief Helper for creating unique IDs
/// @details Required when creating many widgets in a loop, e.g. in a table
IMGUI_API void PushID_formatted(const char* format, ...)    IM_FMTARGS(1);

IMGUI_API void PushID_formatted(const char* format, ...)
{
    // format the variable string
    va_list args;
    char sz[500];
    va_start (args, format);
    vsnprintf(sz, sizeof(sz), format, args);
    va_end (args);
    // Call the actual push function
    PushID(sz);
}

/// @brief Button with on-hover popup helper text
/// @param label Text on Button
/// @param tip Tooltip text when hovering over the button (or NULL of none)
/// @param colFg Foreground/text color (optional, otherwise no change)
/// @param colBg Background color (optional, otherwise no change)
/// @param size button size, 0 for either axis means: auto size
IMGUI_API bool ButtonTooltip(const char* label,
                             const char* tip = nullptr,
                             ImU32 colFg = IM_COL32(1,1,1,0),
                             ImU32 colBg = IM_COL32(1,1,1,0),
                             const ImVec2& size = ImVec2(0,0))
{
    // Setup colors
    if (colFg != IM_COL32(1,1,1,0))
        ImGui::PushStyleColor(ImGuiCol_Text, colFg);
    if (colBg != IM_COL32(1,1,1,0))
        ImGui::PushStyleColor(ImGuiCol_Button, colBg);

    // do the button
    bool b = ImGui::Button(label, size);
    
    // restore previous colors
    if (colBg != IM_COL32(1,1,1,0))
        ImGui::PopStyleColor();
    if (colFg != IM_COL32(1,1,1,0))
        ImGui::PopStyleColor();

    // do the tooltip
    if (tip && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", tip);
    
    // return if button pressed
    return b;
}

}

//
// MARK: Global data and functions
//

// The raw TTF data of OpenFontIcons has been generated into the following file
#include "fa-solid-900.inc"

// Initial data for the example table
/*ImguiWidget::tableDataListTy TABLE_CONTENT = {
    {"6533","MH-65C Dolphin","AS65","United States Coast Guard",0.0f,false},
    {"N493TR","SR22T","S22T","Aircraft Guaranty Corp Trustee",0.0f,true},
    {"N77FK","G-IV","GLF4","Wilmington Trust Co Trustee",0.0f,true},
    {"N911XB","EC135T1","EC35","Air Med Services Llc",0.0f,false},
    {"OY-JRJ","Avions de Transport Regional ATR 42 310","AT43","Danish Air Transport",0.0f,false},
    {"CB-8001","C-17A Globemaster III","C17","Indian Air Force",0.0f,false},
    {"G-DVIP","AGUSTA A109E","A109","Castle Air",0.0f,true},
    {"OE-KSD","91 D Safir","SB91","Patrick Lohr",0.0f,false},
    {"D-ITOR","Citation CJ2+","C25A","Hormann Kg",0.0f,false},
    {"N544XL","Citation Excel","C56X","High Tec Industries Services Inc",0.0f,true},
    {"N368MS","R44 II","R44","Silvestri Mark J",0.0f,true},
    {"N451QX","DHC-8-402","DH8D","Horizon Air Industries Inc",0.0f,true},
    {"N1125J","1125 WESTWIND ASTRA","ASTR","Djb Air Llc",0.0f,false},
    {"N250SH","AS 350 B2","AS50","Sundance Helicopters Inc",0.0f,true}
};*/

// To show how global values synch between window instances we declare here 2 global variables
// Values in node "Drag Controls"
float       g_dragVal1  = 0.0f;
int         g_dragVal2  = 0;


// Trying to find a way to get a image to be displayed
const std::string IMAGE_NAME = "./Resources/plugins/imgui4xp/imgui_demo.jpg";

// Font size, also roughly defines height of one line
constexpr float FONT_SIZE = 16.0f;

/// Uses "stb_image" library to load a picture into memory
/// @param fileName Path to image file
/// @param[out] imgWidth Image width in pixel
/// @param[out] imgHeight Image height in pixel
/// @return texture id
/// @exception std::runtime_error if image not found
int loadImage(const std::string& fileName, int& imgWidth, int& imgHeight) {
    int nComps;
    uint8_t *data = stbi_load(fileName.c_str(), &imgWidth, &imgHeight, &nComps, sizeof(uint32_t));

    if (!data) {
        throw std::runtime_error(std::string("Couldn't load image: ") + stbi_failure_reason());
    }

    int id;
    XPLMGenerateTextureNumbers(&id, 1);
    XPLMBindTexture2d(id, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGBA, imgWidth, imgHeight, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, data);

    stbi_image_free(data);

    return id;
}

/// Wrapper around loadImage() capturing the possible exception
/// @param fileName Path to image file
/// @param[out] imgSize Image size in pixel
/// @return texture id for loaded image, or 0 in case of failure
int try2load_image(const std::string& fileName, ImVec2& imgSize) {
    try {
        int imgWidth=0, imgHeight=0;
        int ret = loadImage(fileName, imgWidth, imgHeight);
        imgSize.x = float(imgWidth);
        imgSize.y = float(imgHeight);
        return ret;
    } catch (const std::exception &e) {
        std::string err = std::string("imgui4xp Error: ") + e.what() + " in " + fileName + "\n";
        XPLMDebugString(err.c_str());
        return 0;
    }
}

// Helper: Turns a string upper case
inline std::string& toupper (std::string& s)
{
    std::for_each(s.begin(), s.end(), [](char& c) { c = toupper(c); });
    return s;
}
static const ImWchar ranges[] = { 0x0020, 0x07FA, //  Latin + Latin Supplement
        0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
        0x2DE0, 0x2DFF, // Cyrillic Extended-A
        0xA640, 0xA69F, // Cyrillic Extended-B
        0,
    };
void configureImgWindow()
{
    XPLMDebugString("AUTOATC: IMGUIXPlugin make_shared\n");
  ImgWindow::sFontAtlas = std::make_shared<ImgFontAtlas>();
  ImGui::CreateContext(ImgWindow::sFontAtlas->mOurAtlas);
  ImGuiIO& io = ImGui::GetIO();
  // use actual parameters to configure the font, or use one of the other methods.

  // this is a post from kuroneko on x-plane.org explaining this use.

  // Basic setup looks something like:
  // To avoid bleeding VRAM like it's going out of fashion, there is only one font atlas shared over all ImgWindows
  // and we keep the manged pointer to it in the ImgWindow class statics.

  // I use the C++11 managed/smart pointers to enforce RAII behaviours rather than encouraging use of new/delete.
  //  This means the font atlas will only get destroyed when you break all references to it.
  // (ie: via ImgWindow::sFontAtlas.reset())  You should never really need to do that though,
  // unless you're being disabled (because you'll lose your texture handles anyway and it's probably a good idea
  // to forcibly tear down the font atlas then).

  // It's probably a bug that the instance of ImgWindow doesn't actually take a copy of the shared_ptr to ensure
  // the font atlas remains valid until it's destroyed.  I was working on a lot of things when I threw that update
  // together and I was heading down that path, but I think I forgot to finish it.


  // you can use any of these fonts that are provided with X-Plane or find you own.
  // Currently you can only load one font and not sure if this might change in the future.
   ImFontConfig config;
    
  ImFontAtlas glyph_ranges;

   
  XPLMDebugString("AUTOATC: IMGUIXPlugin AddFontFromFileTTF\n");
   ImgWindow::sFontAtlas->AddFontFromFileTTF("Resources/fonts/DejaVuSans.ttf", FONT_SIZE,&config,ranges); //glyph_ranges.GetGlyphRangesCyrillic()fullranges);
  //ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/DejaVuSansMono.ttf", FONT_SIZE);
   
    // Now we merge some icons from the OpenFontsIcons font into the above font
    // (see `imgui/docs/FONTS.txt`)
   config.MergeMode = true;
    
    // We only read very selectively the individual glyphs we are actually using
    // to safe on texture space
    static ImVector<ImWchar> icon_ranges;
    ImFontGlyphRangesBuilder builder;
    // Add all icons that are actually used (they concatenate into one string)
    XPLMDebugString("AUTOATC: IMGUIXPlugin AddText\n");
    builder.AddText(ICON_FA_TRASH_ALT ICON_FA_SEARCH
                    ICON_FA_EXTERNAL_LINK_SQUARE_ALT
                    ICON_FA_WINDOW_MAXIMIZE ICON_FA_WINDOW_MINIMIZE
                    ICON_FA_WINDOW_RESTORE ICON_FA_WINDOW_CLOSE);
    builder.BuildRanges(&icon_ranges);
    XPLMDebugString("AUTOATC: IMGUIXPlugin AddFontFromMemoryCompressedTTF\n");
    // Merge the icon font with the text font
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(fa_solid_900_compressed_data,
                                                          fa_solid_900_compressed_size,
                                                          FONT_SIZE,
                                                          &config,
                                                          icon_ranges.Data);
}

 void configureImgWindow_win()
 {
     XPLMDebugString("AUTOATC: configureImgWindow\n");
    ImgWindow::sFontAtlas = std::make_shared<ImgFontAtlas>();
     ImGui::CreateContext(ImgWindow::sFontAtlas->mOurAtlas);
     ImGuiIO& io = ImGui::GetIO();
     ImFontConfig config;
     //ImFont* font = io.Fonts->AddFontDefault(&config);
     XPLMDebugString("AUTOATC: AddFontDefault\n");
     config.MergeMode = true;
     /*static const ImWchar this_ranges[] = { 0x0020, 0x00FF, //Latin
       0x0100, 0x07FA, //  + Latin Supplement
      //0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
       0x2DE0, 0x2DFF, // Cyrillic Extended-A
       0xA640, 0xA69F, // Cyrillic Extended-B
       0,
     };*/
     //ImFont* font1 = io.Fonts->AddFontFromFileTTF("Resources/fonts/DejaVuSans.ttf", FONT_SIZE);
     //ImFont* font1 = io.Fonts->AddFontFromFileTTF("Resources/fonts/DejaVuSans.ttf", //FONT_SIZE, &config, this_ranges);
     static ImVector<ImWchar> icon_ranges;
     ImFontGlyphRangesBuilder builder;
     XPLMDebugString("AUTOATC: AddFontFromFileTTF\n");
     // Add all icons that are actually used (they concatenate into one string)
     builder.AddText(ICON_FA_TRASH_ALT ICON_FA_SEARCH
         ICON_FA_EXTERNAL_LINK_SQUARE_ALT
         ICON_FA_WINDOW_MAXIMIZE ICON_FA_WINDOW_MINIMIZE
         ICON_FA_WINDOW_RESTORE ICON_FA_WINDOW_CLOSE);
     builder.BuildRanges(&icon_ranges);
     XPLMDebugString("AUTOATC: AddOldFontFromMemoryCompressedTTF\n");
     // Merge the icon font with the text font
     io.Fonts->AddFontFromMemoryCompressedTTF(fa_solid_900_compressed_data,
         fa_solid_900_compressed_size,
         FONT_SIZE,
         &config,
         icon_ranges.Data);
     
     //io.Fonts->Build();
     XPLMDebugString("AUTOATC: configureImgWindow done\n");
 }
void configureImgWindow_old()
{
  ImgWindow::sFontAtlas = std::make_shared<ImgFontAtlas>();

  // use actual parameters to configure the font, or use one of the other methods.

  // this is a post from kuroneko on x-plane.org explaining this use.

  // Basic setup looks something like:
  // To avoid bleeding VRAM like it's going out of fashion, there is only one font atlas shared over all ImgWindows
  // and we keep the manged pointer to it in the ImgWindow class statics.

  // I use the C++11 managed/smart pointers to enforce RAII behaviours rather than encouraging use of new/delete.
  //  This means the font atlas will only get destroyed when you break all references to it.
  // (ie: via ImgWindow::sFontAtlas.reset())  You should never really need to do that though,
  // unless you're being disabled (because you'll lose your texture handles anyway and it's probably a good idea
  // to forcibly tear down the font atlas then).

  // It's probably a bug that the instance of ImgWindow doesn't actually take a copy of the shared_ptr to ensure
  // the font atlas remains valid until it's destroyed.  I was working on a lot of things when I threw that update
  // together and I was heading down that path, but I think I forgot to finish it.


  // you can use any of these fonts that are provided with X-Plane or find you own.
  // Currently you can only load one font and not sure if this might change in the future.
   ImFontConfig config;
    
  ImFontAtlas glyph_ranges;

   
  XPLMDebugString("AUTOATC: AddFontFromFileTTF\n");
  // ImgWindow::sFontAtlas->AddFontFromFileTTF("Resources/fonts/DejaVuSans.ttf", FONT_SIZE,&config,ranges); //glyph_ranges.GetGlyphRangesCyrillic()fullranges);
  // ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/DejaVuSansMono.ttf", FONT_SIZE);
  // ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/Inconsolata.ttf", FONT_SIZE);
  // ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/ProFontWindows", FONT_SIZE);
  // ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/Roboto-Bold.ttf", FONT_SIZE);
  // ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/RobotoCondensed-Regular.ttf", FONT_SIZE);
  // ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/Roboto-Light.ttf", FONT_SIZE);
  // ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/Roboto-Regular.ttf", FONT_SIZE);
  // ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/tahomabd.ttf", FONT_SIZE);

    //ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/DejaVuSansMono.ttf", FONT_SIZE);
    
    // Now we merge some icons from the OpenFontsIcons font into the above font
    // (see `imgui/docs/FONTS.txt`)
   config.MergeMode = true;
   XPLMDebugString("AUTOATC: BuildRanges\n");
    // We only read very selectively the individual glyphs we are actually using
    // to safe on texture space
    static ImVector<ImWchar> icon_ranges;
    ImFontGlyphRangesBuilder builder;
    // Add all icons that are actually used (they concatenate into one string)
    builder.AddText(ICON_FA_TRASH_ALT ICON_FA_SEARCH
                    ICON_FA_EXTERNAL_LINK_SQUARE_ALT
                    ICON_FA_WINDOW_MAXIMIZE ICON_FA_WINDOW_MINIMIZE
                    ICON_FA_WINDOW_RESTORE ICON_FA_WINDOW_CLOSE);
    builder.BuildRanges(&icon_ranges);
    XPLMDebugString("AUTOATC: AddFontFromMemoryCompressedTTF\n");
    // Merge the icon font with the text font
    ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(fa_solid_900_compressed_data,
                                                          fa_solid_900_compressed_size,
                                                          FONT_SIZE,
                                                          &config,
                                                          icon_ranges.Data);
    XPLMDebugString("AUTOATC: configureImgWindow done\n");
}

// Undo what we did in configureImgWindow()
void cleanupAfterImgWindow()
{
    // We just destroy the font atlas
    //ImgWindow::sFontAtlas.reset();
   // ImGui::DestroyContext();
}

//
// MARK: ImguiWidget (our example implementation of ImguiWindow)
//

// texture number and size of the image we want to show
// (static, because we want to load the image into a texture just once)
int      ImguiWidget::image_id = 0;
ImVec2   ImguiWidget::image_size;

// Counter for the number of windows opened
int      ImguiWidget::num_win = 0;

// Does any text contain the characters in s?
bool ImguiWidget::tableDataTy::contains (const std::string& s) const
{
    // try finding s in all our texts
    for (const std::string& t: {reg, model, typecode, owner} )
    {
        std::string l = t;
        if (toupper(l).find(s) != std::string::npos)
            return true;
    }
    
    // not found
    return false;
}


ImguiWidget::ImguiWidget(int left, int top, int right, int bot,
                         XPLMWindowDecoration decoration,
                         XPLMWindowLayer layer,void (*guiFunc)(void)) :
    ImgWindow(left, top, right, bot, decoration, layer),
    myWinNum(++num_win)             // assign a unique window number
{
    // Disable reading/writing of "imgui.ini"
    XPLMDebugString("AutoATC: ImguiWidget\n");
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    guiFunc_ptr = guiFunc;
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0, 180);
    SetWindowDragArea(0, 5, INT_MAX, 5 + 2*int(FONT_SIZE));
    // We take the parameter combination "SelfDecorateResizeable" + "LayerFlightOverlay"
    // to mean: simulate HUD
    if (decoration  == xplm_WindowDecorationSelfDecoratedResizable &&
        layer       == xplm_WindowLayerFloatingWindows)
    {
        // let's set a fairly transparent, barely visible background
        
        // There's no window decoration, so to move the window we need to
        // activate a "drag area", here a small strip (roughly double text height)
        // at the top of the window, ie. the window can be moved by
        // dragging a spot near the window's top
        
    }
    XPLMDebugString("AutoATC: XPLMCreateFlightLoop\n");
    // Create a flight loop id, but don't schedule it yet
    XPLMCreateFlightLoop_t flDef = {
        sizeof(flDef),                              // structSize
        xplm_FlightLoop_Phase_BeforeFlightModel,    // phase
        cbFlightLoop,                               // callbackFunc
        (void*)this,                                // refcon
    };
    flId = XPLMCreateFlightLoop(&flDef);
    
    // Define our own window title
    SetWindowTitle("Imgui v" IMGUI_VERSION " for XTLua");
    //SetWindowTitle("AutoATC Pad");
    SetWindowResizingLimits(100, 100, 1024, 1024);
    SetVisible(true);
    
    // Initialize the list content
    listContent = {
        "1st line", "2nd line", "3rd line", "4th line", "5th line",
        "6th line", "7th line", "8th line", "9th line", "10th line"
    };
    
    // if not yet loaded: try loading an image for display
    //if (!image_id)
     //   image_id = try2load_image(IMAGE_NAME, image_size);
    
    // copy initial table example data, init with random heading
   /* tableList = TABLE_CONTENT;
    for (tableDataTy& td: tableList)
        td.heading = float(std::rand() % 3600) / 10.0f;*/
}

ImguiWidget::~ImguiWidget()
{
    if (flId)
        XPLMDestroyFlightLoop(flId);
}
void ImguiWidget::buildInterface() {
    if(guiFunc_ptr!=NULL)
        guiFunc_ptr();

    /*float win_width = ImGui::GetWindowWidth();
    float win_height = ImGui::GetWindowHeight();

    ImGui::TextUnformatted("Hello, World!");
    ImGui::BeginGroup();
    ImGui::Text("Window size: width = %.0f  height = %.0f", win_width, win_height);
    
    ImGui::TextUnformatted("Two Widgets");
    ImGui::SameLine();
    ImGui::TextUnformatted("One Line.");
    ImGui::EndGroup();*/
}


// Outside all rendering we can change things like window mode
float ImguiWidget::cbFlightLoop(float, float, int, void* inRefcon)
{
    XPLMDebugString("AutoATC: cbFlightLoop\n");
    // refcon is pointer to ImguiWidget
    ImguiWidget& wnd = *reinterpret_cast<ImguiWidget*>(inRefcon);
    XPLMDebugString("AutoATC: cbFlightLoop 2\n");
    // Has user requested a change in window mode?
    if (wnd.nextWinPosMode >= 0) {
        wnd.SetWindowPositioningMode(wnd.nextWinPosMode);
        // If we pop in, then we need to explicitely set a position for the window to appear
        if (wnd.nextWinPosMode == xplm_WindowPositionFree) {
            int left, top, right, bottom;
            wnd.GetCurrentWindowGeometry(left, top, right, bottom);
            // Normalize to our starting position (WIN_PAD|WIN_PAD), but keep size unchanged
            const int width  = right-left;
            const int height = top-bottom;
            CalcWinCoords(left, top, right, bottom);
            right  = left + width;
            bottom = top - height;
            wnd.SetWindowGeometry(left, top, right, bottom);
        }
        wnd.nextWinPosMode = -1;
    }
    
    // don't call me again
    return 0.0f;
}

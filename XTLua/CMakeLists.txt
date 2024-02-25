cmake_minimum_required(VERSION 3.8)

project(XTLua_Project)
find_package(Freetype REQUIRED)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(APPEND SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/src/imgui/imgui.cpp"
    "${CMAKE_SOURCE_DIR}/src/imgui/imgui_demo.cpp"
    "${CMAKE_SOURCE_DIR}/src/imgui/imgui_draw.cpp"
    "${CMAKE_SOURCE_DIR}/src/imgui/imgui_tables.cpp"
    "${CMAKE_SOURCE_DIR}/src/imgui/imgui_widgets.cpp"
    "${CMAKE_SOURCE_DIR}/src/imgui/misc/cpp/imgui_stdlib.cpp"
	"${CMAKE_SOURCE_DIR}/src/imgui/misc/freetype/imgui_freetype.cpp"
    "${CMAKE_SOURCE_DIR}/src/ImgWindow/ImgFontAtlas.cpp"
    "${CMAKE_SOURCE_DIR}/src/ImgWindow/ImgWindow.cpp"
)
add_library(XTLua SHARED ${SOURCE_FILES})
target_include_directories(XTLua PUBLIC ${FREETYPE_INCLUDE_DIRS})
target_compile_definitions(XTLua PUBLIC "XPLM200=1" "XPLM210=1" "XPLM300=1" "XPLM301=1" "IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1")

target_include_directories(XTLua PUBLIC "${CMAKE_SOURCE_DIR}/include" 
	"${CMAKE_SOURCE_DIR}/../SDK/CHeaders/XPLM" 
	"${CMAKE_SOURCE_DIR}/../SDK/CHeaders/Widgets" 
	"${CMAKE_SOURCE_DIR}/../lua_sdk"
	"${CMAKE_SOURCE_DIR}/src/imgui"
	"${CMAKE_SOURCE_DIR}/src/imgui/misc/cpp"
	"${CMAKE_SOURCE_DIR}/src/ImgWindow"
	"${CMAKE_SOURCE_DIR}/src/imgui/misc/freetype/"
)

set_target_properties(XTLua PROPERTIES PREFIX "")
set_target_properties(XTLua PROPERTIES SUFFIX ".xpl")

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(XTLua PUBLIC "IBM=1")

	target_compile_options(XTLua PUBLIC "/MD")

	find_library(XPLM_SDK "XPLM_64.lib" PATHS "${CMAKE_SOURCE_DIR}/../SDK/Libraries/Win")
	find_library(LUA_SDK "lua51.lib" PATHS "${CMAKE_SOURCE_DIR}/../lua_sdk")

	target_link_libraries(XTLua PUBLIC ${XPLM_SDK} ${LUA_SDK})

	set_target_properties(XTLua PROPERTIES OUTPUT_NAME "win")

	install(TARGETS XTLua RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/../deploy/xtlua/64")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
	target_compile_definitions(XTLua PUBLIC "LIN=1")

	target_compile_options(XTLua PUBLIC "-fvisibility=hidden" "-fPIC")
	target_link_options(XTLua PUBLIC "-shared" "-rdynamic" "-nodefaultlibs" "-undefined_warning")

	find_library(LUA_SDK "libluajit.a" PATHS "${CMAKE_SOURCE_DIR}/../lua_sdk")
	target_link_libraries(XTLua PUBLIC ${LUA_SDK})

	set_target_properties(XTLua PROPERTIES OUTPUT_NAME "lin")

	install(TARGETS XTLua LIBRARY DESTINATION "${CMAKE_SOURCE_DIR}/../deploy/xtlua/64")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
	target_compile_definitions(XTLua PUBLIC "APL=1")

	find_library(XPLM_SDK "XPLM.framework" PATHS "${CMAKE_SOURCE_DIR}/../SDK/Libraries/Mac")
	find_library(LUA_SDK "libluajit_mac.a" PATHS "${CMAKE_SOURCE_DIR}/../lua_sdk")

	target_link_libraries(XTLua PUBLIC ${XPLM_SDK} ${LUA_SDK})

	set_target_properties(XTLua PROPERTIES OUTPUT_NAME "mac")

	install(TARGETS XTLua LIBRARY DESTINATION "${CMAKE_SOURCE_DIR}/../deploy/xtlua/64")
endif()
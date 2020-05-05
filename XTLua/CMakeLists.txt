cmake_minimum_required(VERSION 3.8)

project("XTLua")

file(GLOB "SOURCE_FILES" "src/*.cpp")
add_library("XTLua" SHARED ${SOURCE_FILES})

include_directories("include" "${CMAKE_SOURCE_DIR}/../X-Plane SDK/CHeaders/XPLM" "${CMAKE_SOURCE_DIR}/../Lua SDK/include")

set("CMAKE_CXX_STANDARD" 11)
set("CMAKE_CXX_STANDARD_REQUIRED" ON)

add_compile_definitions("XPLM200=1")
add_compile_definitions("XPLM210=1")
add_compile_definitions("XPLM300=1")

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
	add_compile_definitions("IBM=1")

	add_compile_options("/MD")

	find_library("XPLM_SDK" "XPLM_64.lib" PATHS "${CMAKE_SOURCE_DIR}/../X-Plane SDK/Libraries/Win")
	find_library("LUA_SDK" "lua51.lib" PATHS "${CMAKE_SOURCE_DIR}/../Lua SDK/lib")

	target_link_libraries("XTLua" ${XPLM_SDK})

	set_target_properties("XTLua" PROPERTIES OUTPUT_NAME "win")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
	add_compile_definitions("LIN=1")

	add_compile_options("-fvisibility=hidden" "-fPIC" "-shared" "-rdynamic" "-nodefaultlibs" "-undefined_warning")
	add_link_options("--version-script=${CMAKE_SOURCE_DIR}/exports.txt")

	find_library("LUA_SDK" "libluajit_mac.a" PATHS "${CMAKE_SOURCE_DIR}/../Lua SDK/lib")

	set_target_properties("XTLua" PROPERTIES OUTPUT_NAME "lin")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin")
	add_compile_definitions("APL=1")

	find_library("XPLM_SDK" "XPLM.framework" PATHS "${CMAKE_SOURCE_DIR}/../X-Plane SDK/Libraries/Mac")
	find_library("LUA_SDK" "libluajit_mac.a" PATHS "${CMAKE_SOURCE_DIR}/../Lua SDK/lib")

	target_link_libraries("XTLua" ${XPLM_SDK})

	set_target_properties("XTLua" PROPERTIES OUTPUT_NAME "mac")
endif()

target_link_libraries("XTLua" ${LUA_SDK})

set_target_properties("XTLua" PROPERTIES SUFFIX ".xpl")

install(TARGETS "XTLua" RUNTIME DESTINATION "${CMAKE_SOURCE_DIR}/../deploy/xtlua/64")
cmake_minimum_required(VERSION 3.25.0)
cmake_policy(SET CMP0087 NEW)
if (NOT DEFINED GUI_BASE_DIR)
	if (DEFINED ENV{GUI_BASE_DIR})
		set(GUI_BASE_DIR $ENV{GUI_BASE_DIR})
	else()
		set(GUI_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../plugin-GUI)
	endif()
endif()
get_filename_component(GUI_BASE_DIR ${GUI_BASE_DIR} ABSOLUTE)

set(PLUGIN_NAME XDAQ-Neuropixels)
project(${PLUGIN_NAME} VERSION 0.5.1 LANGUAGES CXX)

option(XDAQ_NP_BUILD_INSTALLER "Configure a CPack/NSIS Windows installer" OFF)

set(GUI_COMMONLIB_DIR ${GUI_BASE_DIR}/installed_libs)

set(CONFIGURATION_FOLDER $<$<CONFIG:Debug>:Debug>$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>$<$<CONFIG:Release>:Release> )

list(APPEND CMAKE_PREFIX_PATH ${GUI_COMMONLIB_DIR} ${GUI_COMMONLIB_DIR}/${CONFIGURATION_FOLDER})

# Create plugin target
set(SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Source)
file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false "${SOURCE_PATH}/*.cpp" "${SOURCE_PATH}/*.h")
if (APPLE)
	add_library(${PLUGIN_NAME} MODULE ${SRC_FILES})
else()
	add_library(${PLUGIN_NAME} SHARED ${SRC_FILES})
endif()

target_compile_definitions(${PLUGIN_NAME} PRIVATE
    OEPLUGIN
    $<$<PLATFORM_ID:Linux>:JUCE_DISABLE_NATIVE_FILECHOOSERS=1>
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:_DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG=1>
	$<$<PLATFORM_ID:Linux>:OS_LINUX>
	$<$<PLATFORM_ID:Windows>:OS_WINDOWS>
	$<$<PLATFORM_ID:Darwin>:OS_MACOS>
	PLUGIN_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
	PLUGIN_VERSION_MINOR=${PROJECT_VERSION_MINOR}
	PLUGIN_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

target_compile_features(${PLUGIN_NAME} PUBLIC cxx_auto_type cxx_generalized_initializers cxx_std_23)
target_include_directories(${PLUGIN_NAME} PUBLIC ${GUI_BASE_DIR}/JuceLibraryCode ${GUI_BASE_DIR}/JuceLibraryCode/modules ${GUI_BASE_DIR}/Plugins/Headers ${GUI_COMMONLIB_DIR}/include)

#Libraries and compiler options
find_package(fmt REQUIRED)
find_package(libxdaqnp REQUIRED)
target_link_libraries(${PLUGIN_NAME} PRIVATE
	fmt::fmt
	xdaq::xdaq_npapi
)

set_target_properties(${PLUGIN_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	target_compile_definitions(${PLUGIN_NAME} PRIVATE
		NOMINMAX
		WIN32_LEAN_AND_MEAN
		NOGDI
		"JUCE_API=__declspec(dllimport)"
	)
	target_link_libraries(${PLUGIN_NAME} PRIVATE
		${GUI_BASE_DIR}/Build/${CONFIGURATION_FOLDER}/open-ephys.lib
	)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	target_link_libraries(${PLUGIN_NAME} PRIVATE
		GL X11 Xext Xinerama asound dl freetype pthread rt
	)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set_target_properties(${PLUGIN_NAME} PROPERTIES BUNDLE TRUE)
	target_link_options(${PLUGIN_NAME} PRIVATE -undefined dynamic_lookup)
endif()


# Install plugin target
set(PLUGIN_INSTALL_DIR plugins CACHE STRING "Plugin installation directory")
set(SHARED_INSTALL_DIR shared CACHE STRING "Shared installation directory")

# When packaging the standalone installer, CMAKE_INSTALL_PREFIX is set explicitly
# (so the default-prefix block below is skipped). Force the api10 layout here so the
# packaged tree matches what the Open Ephys GUI expects.
if(XDAQ_NP_BUILD_INSTALLER)
	set(PLUGIN_INSTALL_DIR plugins-api10 CACHE STRING "Plugin installation directory" FORCE)
	set(SHARED_INSTALL_DIR shared-api10 CACHE STRING "Shared installation directory" FORCE)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		# Normalize backslashes from %LOCALAPPDATA% to forward slashes so the path is a
		# valid CMake/CPack string (avoids "Invalid escape sequence" warnings).
		if(DEFINED ENV{LOCALAPPDATA})
    		file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}/Open Ephys" XDAQ_NP_WIN_INSTALL_PREFIX)
		else()
    		file(TO_CMAKE_PATH "$ENV{USERPROFILE}/AppData/Local/Open Ephys" XDAQ_NP_WIN_INSTALL_PREFIX)
		endif()
		set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${XDAQ_NP_WIN_INSTALL_PREFIX}")
		set(PLUGIN_INSTALL_DIR plugins-api10 CACHE STRING "Plugin installation directory" FORCE)
		set(SHARED_INSTALL_DIR shared-api10 CACHE STRING "Shared installation directory" FORCE)
	elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
		set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE $ENV{HOME}/Library/Application\ Support/open-ephys)
		set(PLUGIN_INSTALL_DIR plugins-api10 CACHE STRING "Plugin installation directory" FORCE)
		set(SHARED_INSTALL_DIR shared-api10 CACHE STRING "Shared installation directory" FORCE)
	elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${GUI_BASE_DIR}/Build)
		set(PLUGIN_INSTALL_DIR ${CONFIGURATION_FOLDER}/plugins CACHE STRING "Plugin installation directory" FORCE)
		set(SHARED_INSTALL_DIR ${CONFIGURATION_FOLDER}/shared CACHE STRING "Shared installation directory" FORCE)
	endif()
endif()

install(TARGETS ${PLUGIN_NAME}
	LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
	RUNTIME DESTINATION ${PLUGIN_INSTALL_DIR}
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set_target_properties(${PLUGIN_NAME} PROPERTIES
		BUILD_WITH_INSTALL_RPATH TRUE
		INSTALL_RPATH "@loader_path/../Frameworks"
	)
	libxdaqnp_install_runtime(
		PLUGIN_DEST  ${PLUGIN_INSTALL_DIR}/${PLUGIN_NAME}.bundle/Contents/Frameworks
		SHARED_DEST  ${PLUGIN_INSTALL_DIR}/${PLUGIN_NAME}.bundle/Contents/Frameworks
		MANAGERS_DEST ${PLUGIN_INSTALL_DIR}/${PLUGIN_NAME}.bundle/Contents/Frameworks/device_managers
	)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set_target_properties(${PLUGIN_NAME} PROPERTIES
		BUILD_WITH_INSTALL_RPATH TRUE
		INSTALL_RPATH "$ORIGIN/../shared/${PLUGIN_NAME}"
	)
	libxdaqnp_install_runtime(
		PLUGIN_DEST  ${PLUGIN_INSTALL_DIR}
		SHARED_DEST  ${SHARED_INSTALL_DIR}/${PLUGIN_NAME}
		MANAGERS_DEST ${SHARED_INSTALL_DIR}/${PLUGIN_NAME}/device_managers
	)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	libxdaqnp_install_runtime(
		PLUGIN_DEST  ${SHARED_INSTALL_DIR}
		SHARED_DEST  ${SHARED_INSTALL_DIR}
		MANAGERS_DEST ${SHARED_INSTALL_DIR}/${PLUGIN_NAME}/device_managers
	)
endif()

# Optional redistributable installer (CPack)
# option(XDAQ_NP_BUILD_INSTALLER "Build a redistributable installer package" OFF)

if(XDAQ_NP_BUILD_INSTALLER)
	set(CPACK_PACKAGE_NAME "${PLUGIN_NAME}")
	set(CPACK_PACKAGE_VENDOR "KonteX Neuroscience")
	set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "XDAQ-Neuropixels plugin for the Open Ephys GUI")

	if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		set(CPACK_GENERATOR "NSIS")
		set(CPACK_PACKAGE_FILE_NAME "${PLUGIN_NAME}-${PROJECT_VERSION}-windows-x64-installer")
		# Install into the per-user Local AppData where the Open Ephys GUI loads plugins.
		# The NSIS installer elevates to admin and switches to the "all users" shell
		# context, which makes $LOCALAPPDATA resolve to C:\ProgramData. $PROFILE is NOT
		# affected by the shell context, so $PROFILE\AppData\Local always resolves to the
		# real C:\Users\<user>\AppData\Local for both the directory page and the install.
		# Backslashes are doubled so they survive the CPackConfig.cmake round-trip and
		# emit single backslashes into the generated .nsi script.
		set(CPACK_NSIS_INSTALL_ROOT [[$PROFILE\\AppData\\Local]])
		set(CPACK_PACKAGE_INSTALL_DIRECTORY "Open Ephys")
		set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
	endif()

	include(CPack)
endif()


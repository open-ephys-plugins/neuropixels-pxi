cmake_minimum_required(VERSION 3.5.0)
cmake_policy(SET CMP0087 NEW)
if (NOT DEFINED GUI_BASE_DIR)
	if (DEFINED ENV{GUI_BASE_DIR})
		set(GUI_BASE_DIR $ENV{GUI_BASE_DIR})
	else()
		set(GUI_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../plugin-GUI)
	endif()
endif()
get_filename_component(GUI_BASE_DIR ${GUI_BASE_DIR} ABSOLUTE)

set(PLUGIN_NAME XDAQ-OE-NP)
project(${PLUGIN_NAME} VERSION 0.1.0 LANGUAGES CXX)

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
	OEPLUGIN
	"$<$<PLATFORM_ID:Windows>:JUCE_API=__declspec(dllimport)>"
	$<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
	$<$<PLATFORM_ID:Linux>:JUCE_DISABLE_NATIVE_FILECHOOSERS=1>
	$<$<CONFIG:Debug>:DEBUG=1>
	$<$<CONFIG:Debug>:_DEBUG=1>
	$<$<CONFIG:Release>:NDEBUG=1>
)

set(CMAKE_CXX_STANDARD 23)

set(SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Source)
file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false "${SOURCE_PATH}/*.cpp" "${SOURCE_PATH}/*.h")
set(GUI_COMMONLIB_DIR ${GUI_BASE_DIR}/installed_libs)

#set(CONFIGURATION_FOLDER $<$<CONFIG:Debug>:Debug>$<$<NOT:$<CONFIG:Debug>>:Release>)
set(CONFIGURATION_FOLDER $<$<CONFIG:Debug>:Debug>$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>$<$<CONFIG:Release>:Release> )

list(APPEND CMAKE_PREFIX_PATH ${GUI_COMMONLIB_DIR} ${GUI_COMMONLIB_DIR}/${CONFIGURATION_FOLDER})

if (APPLE)
	add_library(${PLUGIN_NAME} MODULE ${SRC_FILES})
else()
	add_library(${PLUGIN_NAME} SHARED ${SRC_FILES})
endif()

target_compile_definitions(${PLUGIN_NAME} PRIVATE
	PLUGIN_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
	PLUGIN_VERSION_MINOR=${PROJECT_VERSION_MINOR}
	PLUGIN_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

target_compile_features(${PLUGIN_NAME} PUBLIC cxx_auto_type cxx_generalized_initializers cxx_std_23)
target_include_directories(${PLUGIN_NAME} PUBLIC ${GUI_BASE_DIR}/JuceLibraryCode ${GUI_BASE_DIR}/JuceLibraryCode/modules ${GUI_BASE_DIR}/Plugins/Headers ${GUI_COMMONLIB_DIR}/include)

set(GUI_BIN_DIR ${GUI_BASE_DIR}/Build/${CONFIGURATION_FOLDER})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# find_package(XDAQ_NPAPI REQUIRED)
endif()


#Libraries and compiler options
if(MSVC)
	target_link_libraries(${PLUGIN_NAME} ${GUI_BIN_DIR}/open-ephys.lib)
	target_compile_options(${PLUGIN_NAME} PRIVATE /sdl- /W0)
	
	install(TARGETS ${PLUGIN_NAME} RUNTIME DESTINATION ${GUI_BIN_DIR}/plugins  CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES})
	install(FILES $<TARGET_PDB_FILE:${PLUGIN_NAME}> DESTINATION  ${GUI_BIN_DIR}/plugins OPTIONAL)

	set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../libs)
elseif(LINUX)
	target_link_libraries(${PLUGIN_NAME} PRIVATE GL X11 Xext Xinerama asound dl freetype pthread rt)
	set_property(TARGET ${PLUGIN_NAME} APPEND_STRING PROPERTY LINK_FLAGS
		"-fvisibility=hidden -fPIC -rdynamic -Wl,-rpath,'$$ORIGIN/../shared'")
	target_compile_options(${PLUGIN_NAME} PRIVATE -fPIC -rdynamic)
	
	install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION ${GUI_BIN_DIR}/plugins)
elseif(APPLE)
	set_target_properties(${PLUGIN_NAME} PROPERTIES BUNDLE TRUE)
	set_property(TARGET ${PLUGIN_NAME} APPEND_STRING PROPERTY LINK_FLAGS
	"-undefined dynamic_lookup -rpath @loader_path/../../../../shared")

	install(TARGETS ${PLUGIN_NAME} DESTINATION $ENV{HOME}/Library/Application\ Support/open-ephys/plugins-api8)
	set(CMAKE_PREFIX_PATH /opt/local)
endif()

#create filters for vs and xcode

foreach( src_file IN ITEMS ${SRC_FILES})
	get_filename_component(src_path "${src_file}" PATH)
	file(RELATIVE_PATH src_path_rel "${SOURCE_PATH}" "${src_path}")
	string(REPLACE "/" "\\" group_name "${src_path_rel}")
	source_group("${group_name}" FILES "${src_file}")
endforeach()


set(NEUROPIX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source/API)
target_include_directories(${PLUGIN_NAME} PRIVATE ${NEUROPIX_INCLUDE_DIR})

find_package(XDAQ_NPAPI REQUIRED)
target_link_libraries(${PLUGIN_NAME} PRIVATE xdaq::np::XDAQ_NPAPI)


set(PLUGIN_INSTALL_DIR plugins CACHE STRING "Plugin installation directory")
set(SHARED_INSTALL_DIR shared CACHE STRING "Shared installation directory")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
		set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE C:/ProgramData/Open\ Ephys)
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


if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	install(TARGETS ${PLUGIN_NAME}
		LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
		RUNTIME DESTINATION ${PLUGIN_INSTALL_DIR}
	)
	install(DIRECTORY ${XDAQ_DEVICE_MANAGER_DIR} DESTINATION ${SHARED_INSTALL_DIR}/${PLUGIN_NAME})
	install(DIRECTORY ${XDAQ_RESOURCES_DIR} DESTINATION ${SHARED_INSTALL_DIR}/${PLUGIN_NAME})
	set(CPACK_PACKAGE_NAME ${PLUGIN_NAME})
	set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/package)

	set(CPACK_NSIS_DISPLAY_NAME ${PLUGIN_NAME})
	set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)
	set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
	set(CPACK_NSIS_UNINSTALL_NAME "Uninstall ${PLUGIN_NAME}")
	set(CPACK_PACKAGE_VENDOR "KonteX Neuroscience")
	string(REPLACE "/" "\\\\" CPACK_NSIS_INSTALL_ROOT ${CMAKE_INSTALL_PREFIX})
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "")
	include(CPACK)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	install(TARGETS ${PLUGIN_NAME}
		LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
		RUNTIME DESTINATION ${PLUGIN_INSTALL_DIR}
	)
	install(DIRECTORY ${XDAQ_DEVICE_MANAGER_DIR}
		DESTINATION ${PLUGIN_INSTALL_DIR}/${PLUGIN_NAME}.bundle/Contents/PlugIns
	)
	install(DIRECTORY ${XDAQ_RESOURCES_DIR}
		DESTINATION ${PLUGIN_INSTALL_DIR}/${PLUGIN_NAME}.bundle/Contents/PlugIns
	)
	set(CPACK_GENERATOR "DragNDrop")
	set(CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK ON)
    set(CPACK_PRE_BUILD_SCRIPTS "${CMAKE_SOURCE_DIR}/cmake/codesign.cmake")
	include(CPACK)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set_target_properties(${PLUGIN_NAME} PROPERTIES
		BUILD_WITH_INSTALL_RPATH TRUE
		INSTALL_RPATH "$ORIGIN/../shared/${PLUGIN_NAME}"
	)

	install(DIRECTORY ${XDAQ_DEVICE_MANAGER_DIR} DESTINATION ${SHARED_INSTALL_DIR}/${PLUGIN_NAME})
	install(DIRECTORY ${XDAQ_RESOURCES_DIR} DESTINATION ${SHARED_INSTALL_DIR}/${PLUGIN_NAME})
	install(TARGETS ${PLUGIN_NAME}
		LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
		RUNTIME DESTINATION ${PLUGIN_INSTALL_DIR}
	)
	install(IMPORTED_RUNTIME_ARTIFACTS xdaq::np::XDAQ_NPAPI
		LIBRARY DESTINATION ${SHARED_INSTALL_DIR}/${PLUGIN_NAME}
		RUNTIME DESTINATION ${SHARED_INSTALL_DIR}/${PLUGIN_NAME}
	)
endif()


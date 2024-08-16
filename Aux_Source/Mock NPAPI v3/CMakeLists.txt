
cmake_minimum_required(VERSION 3.12)

project(MOCK_NPAPI_V3 LANGUAGES C CXX)

# Create shared library target
add_library(MOCK_NPAPI_V3_LIB SHARED
	src/NeuropixAPI.cpp

)


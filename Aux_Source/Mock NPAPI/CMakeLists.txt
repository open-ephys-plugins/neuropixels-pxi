
cmake_minimum_required(VERSION 3.12)

project(MOCK_NPAPI LANGUAGES C CXX)

# Create shared library target
add_library(MOCK_NPAPI_LIB SHARED
	src/neuropixAPI.c
)


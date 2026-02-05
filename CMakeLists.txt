cmake_minimum_required(VERSION 3.15)
project(nexus_streamlink VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    message(STATUS "Building Nexus addon")

    # Build as shared library (DLL) for Nexus
    add_library(nexus_streamlink SHARED
        nexus_streamlink.cpp
        Nexus.h
        ArcDPS.h
        UnofficialExtras.h
    )

    # Set output name for Nexus addon
    set_target_properties(nexus_streamlink PROPERTIES
        OUTPUT_NAME "nexus_streamlink"
        PREFIX ""
        SUFFIX ".dll"
    )

    # Link Windows libraries
    target_link_libraries(nexus_streamlink PRIVATE kernel32)

    # Disable C++ exceptions and RTTI for smaller binary
    if(MSVC)
        target_compile_options(nexus_streamlink PRIVATE
            /W4         # Warning level 4
            /WX-        # Don't treat warnings as errors
            /GR-        # Disable RTTI
            /EHs-c-     # Disable exceptions
            /MT         # Static runtime (no MSVCRT dependency)
        )
        target_compile_definitions(nexus_streamlink PRIVATE
            _CRT_SECURE_NO_WARNINGS
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
    endif()

    # MinGW support
    if(MINGW)
        target_compile_options(nexus_streamlink PRIVATE
            -Wall
            -fno-exceptions
            -fno-rtti
        )
        target_link_options(nexus_streamlink PRIVATE
            -static-libgcc
            -static-libstdc++
            -static
        )
    endif()
endif()

# Install target
install(TARGETS nexus_streamlink
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

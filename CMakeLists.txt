cmake_minimum_required(VERSION 3.15)
project(arcdps_killstreak VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    # Build as shared library (DLL)
    add_library(arcdps_killstreak SHARED
        killstreak.cpp
        arcdps_structs.h
    )

    # Set output name to match ArcDPS plugin naming convention
    # Plugins should be named d3d11.dll or arcdps_*.dll
    set_target_properties(arcdps_killstreak PROPERTIES
        OUTPUT_NAME "arcdps_killstreak"
        PREFIX ""
        SUFFIX ".dll"
    )

    # Link Windows libraries
    target_link_libraries(arcdps_killstreak PRIVATE
        kernel32
    )

    # Disable C++ exceptions and RTTI for smaller binary (optional)
    if(MSVC)
        target_compile_options(arcdps_killstreak PRIVATE
            /W4         # Warning level 4
            /WX-        # Don't treat warnings as errors
            /GR-        # Disable RTTI
            /EHs-c-     # Disable exceptions
            /MT         # Static runtime (no MSVCRT dependency)
        )
        target_compile_definitions(arcdps_killstreak PRIVATE
            _CRT_SECURE_NO_WARNINGS
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
    endif()

    # MinGW support
    if(MINGW)
        target_compile_options(arcdps_killstreak PRIVATE
            -Wall
            -fno-exceptions
            -fno-rtti
        )
        target_link_options(arcdps_killstreak PRIVATE
            -static-libgcc
            -static-libstdc++
            -static
        )
    endif()
endif()

# Install target
install(TARGETS arcdps_killstreak
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

# Platform-specific source selection
if(WIN32)
    set(PLATFORM_SRC "win32/win32.cpp")
    set(PLATFORM_DEPS "") 
elseif(APPLE)
    set(PLATFORM_SRC "mac/mac.mm")
    set(PLATFORM_DEPS "-framework Cocoa -framework Metal -framework QuartzCore")
else()
    set(PLATFORM_SRC "linux/linux.cpp")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11_LIBS REQUIRED IMPORTED_TARGET x11 xext xcursor xrandr xrender xi xfixes)
    pkg_check_modules(WAYLAND_LIBS REQUIRED IMPORTED_TARGET wayland-client wayland-egl wayland-cursor egl)
    set(PLATFORM_DEPS PkgConfig::X11_LIBS PkgConfig::WAYLAND_LIBS)
endif()

add_library(platform SHARED)

target_sources(platform
    PUBLIC FILE_SET CXX_MODULES 
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES platform.cppm
    
    PRIVATE
    ${PLATFORM_SRC}
)

target_include_directories(platform INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

target_link_libraries(platform PRIVATE SDL3::SDL3-static ${PLATFORM_DEPS})
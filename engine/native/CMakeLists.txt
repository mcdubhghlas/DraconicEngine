include(Compiler)
include(Modules)

add_subdirectory(thirdparty)

add_modules_library(core SHARED)
target_link_libraries(core PUBLIC definitions math io memory)

add_subdirectory(rendering)
add_subdirectory(platform)
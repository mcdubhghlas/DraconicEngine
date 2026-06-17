add_subdirectory(thirdparty SYSTEM)

add_modules_library(runtime)
target_link_libraries(runtime PUBLIC platform core Input scene rendering)

# TODO: binding library
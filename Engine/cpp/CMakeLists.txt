add_subdirectory(thirdparty SYSTEM)

add_modules_library(runtime SHARED)
target_link_libraries(runtime PUBLIC Platform Core Input Scene Rendering)

# TODO: binding library
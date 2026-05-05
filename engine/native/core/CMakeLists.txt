add_modules_library(definitions)

add_modules_library(math)
target_link_libraries(math PUBLIC definitions)

add_modules_library(io)
target_link_libraries(io PUBLIC definitions stb_image)

add_modules_library(memory)
target_link_libraries(memory PUBLIC definitions)

# image_loader is a FEATURE module built on io
add_library(image_loader)

target_sources(image_loader
    PUBLIC
        FILE_SET CXX_MODULES FILES io/image_loader.cppm
    PRIVATE
        io/image_loader.cpp
)

target_link_libraries(image_loader
    PRIVATE
        stb_image
        io
        definitions
)
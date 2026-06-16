add_modules_library(Definitions PIC)
add_modules_library(math)
add_modules_library(io)
add_modules_library(memory)

target_link_libraries(math PUBLIC definitions bx)
target_link_libraries(io PUBLIC definitions stb_image)
target_link_libraries(memory PUBLIC definitions math)

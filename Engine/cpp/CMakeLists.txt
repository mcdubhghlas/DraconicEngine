add_subdirectory(ThirdParty SYSTEM)

add_modules_library(Runtime SHARED)
# Runtime exposes the whole engine through the `draconic` module. Shell (the single
# library holding both backends) is linked PUBLIC, so a consumer only links Runtime and
# does `import draconic;`; createShell() resolves from Shell.
target_link_libraries(Runtime PUBLIC Core Shell Scene Rendering)

# TODO: binding library
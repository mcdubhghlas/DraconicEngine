export module core.filesystem;

import std;

export namespace draco::filesystem
{
    // Returns a buffer of the file data
    std::vector<std::uint8_t> load_binary(const std::string& path);
}

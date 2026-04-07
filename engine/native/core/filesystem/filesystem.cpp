module;

import std;

module core.filesystem;

namespace draco::filesystem
{
    std::vector<std::uint8_t> load_binary(const std::string& path)
    {
        // Open at the end (ate) to get size and in binary mode
        std::ifstream file(path, std::ios::binary | std::ios::ate);
    
        if (!file.is_open()) {
            std::println("Error: Could not open file at: {}", path);
            // Return an empty vector
            return {}; 
        }

        std::streamsize size = file.tellg();
        if (size <= 0) {
            std::println("Error: File is empty or unreadable: {}", path);
            return {};
        }

        file.seekg(0, std::ios::beg);

        std::vector<std::uint8_t> buffer(static_cast<std::size_t>(size));
        if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return buffer;
        }

        std::println("Error: Failed to read file contents: {}", path);
        return {};
    }
}
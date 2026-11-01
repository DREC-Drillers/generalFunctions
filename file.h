#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace generalFunctions {

std::string readFile(const std::filesystem::path& path);





//xz che tam dalshe proishodit

template <typename T = std::uint8_t>
[[nodiscard]] std::vector<T> readBinaryFile(const std::filesystem::path& path)
{
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>
                  && !std::is_same_v<T, bool>
                  && std::is_same_v<T, std::remove_cv_t<T>>,
                  "T must be an unqualified unsigned integer type other than bool");

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file for reading: " + path.string());
    }

    std::vector<T> result;
    std::array<T, 64 * 1024 / sizeof(T)> buffer{};
    while (true) {
        file.read(reinterpret_cast<char*>(buffer.data()),
                  static_cast<std::streamsize>(buffer.size() * sizeof(T)));
        const auto byteCount = static_cast<std::size_t>(file.gcount());
        // A short final read sets failbit and eofbit; this is normal.
        if (file.bad() || (file.fail() && !file.eof())) {
            throw std::runtime_error("Cannot read file: " + path.string());
        }
        if (byteCount % sizeof(T) != 0) {
            throw std::runtime_error("File size is not divisible by element size: "
                                     + path.string());
        }
        result.insert(result.end(), buffer.data(), buffer.data() + byteCount / sizeof(T));
        if (file.eof()) {
            return result;
        }
    }
}

// Reads lines without LF/CRLF endings; preserves empty lines.
// A final newline does not create an extra empty line.
[[nodiscard]] std::vector<std::string> readLines(const std::filesystem::path& path);

// Creates or truncates a file. Parent directories must already exist.
// Writes bytes unchanged. Replacement is not atomic.
// Throws std::runtime_error on open/write/close errors.
void writeFile(const std::filesystem::path& path, std::string_view content);

// Creates or appends. Does not insert a newline automatically.
void appendFile(const std::filesystem::path& path, std::string_view content);

} // namespace generalFunctions
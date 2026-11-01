#include <algorithm>
#include <iostream>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <utility>

#include "file.h"

namespace generalFunctions 
{

namespace
{

void fileError(const std::string msg, const std::filesystem::path& path)
{
    throw std::runtime_error(msg + ":" + path.string());
}

} //namespace

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream file(path);

    if (!file) 
    {
        fileError("Cannot open file for reading", path);
    }

    std::string buffer;
    std::string line;

    while(std::getline(file, line))
    {
        buffer += line + "\n";
    }

    if (file.bad() || !file.eof()) 
    {
        fileError("Cannot read file", path);
    }

    return buffer;
}



//xz che tam dalshe proishodit
namespace {

constexpr std::size_t fileBufferSize = 64 * 1024;

void writeContent(const std::filesystem::path& path,
                  std::string_view content, std::ios::openmode mode)
{
    std::ofstream file(path, std::ios::binary | std::ios::out | mode);
    if (!file) {
        fileError("Cannot open file for writing", path);
    }

    // Bounded chunks keep conversion to streamsize safe for large inputs.
    while (!content.empty()) {
        const auto count = std::min(content.size(), fileBufferSize);
        file.write(content.data(), static_cast<std::streamsize>(count));
        if (!file) {
            fileError("Cannot write file", path);
        }
        content.remove_prefix(count);
    }

    // Buffered errors may appear only when flushing/closing the file.
    file.close();
    if (!file) {
        fileError("Cannot close file after writing", path);
    }
}

} // namespace

void writeFile(const std::filesystem::path& path, std::string_view content)
{
    writeContent(path, content, std::ios::trunc);
}

void appendFile(const std::filesystem::path& path, std::string_view content)
{
    writeContent(path, content, std::ios::app);
}

std::vector<std::string> readLines(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        fileError("Cannot open file for reading", path);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        // Remove CR only if it was followed by the LF delimiter.
        if (!file.eof() && !line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(std::move(line));
    }
    if (file.bad() || !file.eof()) {
        fileError("Cannot read file", path);
    }
    return lines;
}

} // namespace generalFunctions
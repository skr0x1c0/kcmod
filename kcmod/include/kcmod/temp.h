//
// Created by Sreejith Krishnan R on 25/02/23.
//

#pragma once

#include <filesystem>

namespace kcmod {

class TemporaryFile {
public:
    TemporaryFile();
    TemporaryFile(const std::filesystem::path &path);
    ~TemporaryFile();

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

}// namespace kcmod
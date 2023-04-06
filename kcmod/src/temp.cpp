//
// Created by Sreejith Krishnan R on 25/02/23.
//

#include "debug.h"
#include <unistd.h>

#include "temp.h"


using namespace kcmod;


TemporaryFile::TemporaryFile()
    : path_{std::filesystem::temp_directory_path() / "kcmod.XXXXXXXXXX"} {
    int fd = mkstemp(path_.string().data());
    kcmod_verify(fd >= 0);
    close(fd);
}

TemporaryFile::TemporaryFile(const std::filesystem::path &path): TemporaryFile() {
    std::filesystem::copy_file(path, path_, std::filesystem::copy_options::overwrite_existing);
}

TemporaryFile::~TemporaryFile() {
    std::filesystem::remove(path_);
}

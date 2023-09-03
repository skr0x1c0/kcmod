// Copyright (c) skr0x1c0 2023.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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

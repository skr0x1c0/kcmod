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

#include <map>
#include <span>

#include <docopt.h>
#include <mio/mmap.hpp>

#include "debug.h"

#include "kernelcache.h"
#include "temp.h"

using namespace kcmod;

static const char k_usage[] =
    R"(kcmod.

    Usage:
      kcmod replace <fileset_id> --kernelcache=<kc> --kext=<kext> --output=<output> [--symbols=<symbols>]

    Options:
      -k --kernelcache <kc>       Input kernelcache (Mach-O fileset)
      -x --kext <kext>            Kext to replace fileset
      -s --symbols <symbols>      Additional symbol information in json format
      -o --output <output>        Output kernelcache
      --version                   Show version.
)";

int main(int argc, const char *argv[]) {
    std::map<std::string, docopt::value> args =
        docopt::docopt(k_usage,
                       {argv + 1, argv + argc},
                       true,
                       "kcmod v1.0.0");

    if (args["replace"]) {
        TemporaryFile output{args["--kernelcache"].asString()};
        mio::mmap_sink kc_mmap{output.path().string()};
        std::span<char> kc_data{kc_mmap.data(), kc_mmap.size()};
        KernelCache kc {kc_data};
        KernelExtension kext{args["--kext"].asString()};
        std::optional<std::filesystem::path> symbols =
            args["--symbols"] ? std::optional{std::filesystem::path{args["--symbols"].asString()}}
                              : std::nullopt;
        kc.replace_fileset(args["<fileset_id>"].asString(), kext, symbols);
        std::filesystem::copy_file(output.path(), args["--output"].asString(),
                                   std::filesystem::copy_options::overwrite_existing);
    } else {
        kcmod_not_reachable();
    }
    return 0;
}
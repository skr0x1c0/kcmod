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
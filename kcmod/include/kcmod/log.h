//
// Created by Sreejith Krishnan R on 26/02/23.
//

#pragma once

namespace kcmod {

#define kcmod_log_debug(format, ...) fmt::print("[DEBUG] " format "\n", ##__VA_ARGS__)
#define kcmod_log_warn(format, ...) fmt::print("[WARN] " format "\n", ##__VA_ARGS__)

}// namespace kcmod

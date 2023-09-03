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

#include <sys/kernel_types.h>
#include <pexpert/pexpert.h>
#include <security/mac_policy.h>
#include <IOKit/IOLib.h>

#include <kcmod/kcmod.h>


KCMOD_OVERRIDE(int, mac_policy_register, struct mac_policy_conf* conf, mac_policy_handle_t* handlep, void* xd) {
    IOLog("***********************************\n");
    IOLog("***********************************\n");
    IOLog("***********************************\n");
    IOLog("***********************************\n");
    IOLog("mac_policy_register override: %s\n", conf->mpc_name);
    IOLog("***********************************\n");
    IOLog("***********************************\n");
    IOLog("***********************************\n");
    IOLog("***********************************\n");
    return KCMOD_SUPER(mac_policy_register, conf, handlep, xd);
}

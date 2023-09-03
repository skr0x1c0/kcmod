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

#include <mach/mach_types.h>
#include <sys/sysctl.h>

uint32_t kcmod_test_config = 100;
SYSCTL_INT(_kern, OID_AUTO, kcmod_test, CTLFLAG_RW | CTLFLAG_LOCKED, &kcmod_test_config, 0, "kcmod test sysctl");

kern_return_t test_start(kmod_info_t * ki, void *d);
kern_return_t test_stop(kmod_info_t *ki, void *d);

kern_return_t test_start(kmod_info_t * ki, void *d)
{
    sysctl_register_oid(&sysctl__kern_kcmod_test);
    return KERN_SUCCESS;
}

kern_return_t test_stop(kmod_info_t *ki, void *d)
{
    sysctl_unregister_oid(&sysctl__kern_kcmod_test);
    return KERN_SUCCESS;
}

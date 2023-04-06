//
//  test.c
//  test
//
//  Created by Sreejith Krishnan R on 22/02/23.
//

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

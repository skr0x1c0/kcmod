//
// Created by Sreejith Krishnan R on 01/03/23.
//

#include <string.h>

#include <mach/arm/boolean.h>
#include <pexpert/pexpert.h>

#include <kcmod/kcmod.h>


KCMOD_OVERRIDE(boolean_t, PE_parse_boot_argn, const char* arg_string, void *buf, int buf_size) {
    if (strcmp(arg_string, "serial") == 0) {
        kprintf("PE_parse_boot_argn serial override");
        *(int *)buf = 3;
        return 1;
    }
    return __kcmod_hook_super_PE_parse_boot_argn(arg_string, buf, buf_size);
}

KCMOD_OVERRIDE(boolean_t, PE_parse_boot_argn_internal, char* args, const char* arg_string, void *buf, int buf_size, bool force) {
    if (strcmp(arg_string, "serial") == 0) {
        kprintf("PE_parse_boot_argn_internal serial override");
        *(int *)buf = 3;
        return 1;
    }
    return __kcmod_hook_override_PE_parse_boot_argn_internal(args, arg_string, buf, buf_size, force);
}
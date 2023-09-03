//
// Created by Sreejith Krishnan R on 01/03/23.
//

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

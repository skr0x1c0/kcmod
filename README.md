# KCMod

KCMod is an utility for overriding functions and replacing filesets with new kernel extensions in a Mach-O kernelcache. It is mainly intented to modify kernelcache in `ipsw` files for security research using [Apple Security Research Device](https://security.apple.com/research-device/)


# Installation
``` sh
git clone --recurse-submodules git@github.com:skr0x1c0/kcmod.git
cd kcmod/kcmod
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j kcmod
sudo cp kcmod/kcmod /usr/local/bin/
```


# Usage

## Linking a kernel extension with kernelcache

To link a new kernel extension with a kernelcache, `kcmod` must **replace** an exisiting fileset inside the kernelcache with the new kext. This is because the Mach-O kernelcache does not contain information (like `LC_SEGMENT_SPLIT_INFO`) required for relocating its fileset segments. 

The following example command replaces the fileset `com.apple.nke.l2tp` inside a kernelcache with a new kext. 

``` sh
kcmod replace com.apple.nke.l2tp --kernelcache <path-to-kc> --kext <path-to-ikext> --output <path-to-output-kc>
```

**NOTE:** Select a victim fileset such that the size of each segment in victim fileset is greater than or equal to size of corresponding segment in new kext.


## Overriding functions in kernelcache

When you link a kext using `kcmod replace` command, the utility will look for functions inside kext with names starting with `__kcmod_hook_`.

When it encounters a function with a name like `__kcmod_hook_override_sample_fn`, it will look for function with name `sample_fn` inside the kernelcache and replace the first instruction in `sample_fn` with an unconditional branch instruction `b #imm`. The value of `imm` will be set such that program counter will jump to `__kcmod_hook_override_sample_fn` method effectively overriding the `sample_fn`. 

original `sample_fn`:
``` assembly
0x4000     orig_sample_fn_instr_0
0x4004     orig_sample_fn_instr_1
0x4008     orig_sample_fn_instr_2
...
``` 

new `sample_fn`:
``` assembly
0x4000     b 0x4200  # __kcmod_hook_override_sample_fn
0x4004     orig_sample_fn_instr_1
0x4008     orig_sample_fn_instr_2
...
```

override function `__kcmod_hook_override_sample_fn`:
``` assembly
0x4200     hook_fn_instr_0
0x4204     hook_fn_instr_1
0x4208     ret
```


When you need to execute code in the original `sample_fn` you must also define a method named `__kcmod_hook_super_sample_fn`. The `kcmod replace` command will fill this function with relocated original first instruction in `sample_fn` and an unconditional branch instruction for setting the program counter to second instruction in `sample_fn`

override function `__kcmod_hook_override_sample_fn` which calls original `sample_fn`:
``` assembly
0x4200     hook_fn_instr_0
0x4204     hook_fn_instr_1
0x4208     bl 0x4100  # __kcmod_hook_super_sample_fn
0x420b     ret
```

super function `__kcmod_hook_super_sample_fn`:
``` assembly
0x4100     relocated_orig_sample_fn_instr_0
0x4104     b 0x4004  # 2nd instruction in sample_fn
0x4108     brk #1
0x410b     brk #1
```


The macros `KCMOD_OVERRIDE` and `KCMOD_SUPER` can be used for easily creating these override and super functions. These macros are defined in header file `kext/libs/kcmod_hooks/include/kcmod/kcmod.h`.

For example, to override the function `mac_policy_register` in the kernelcache, build a kext with following code and use `kcmod replace` command to link the kext with the kernelcache

``` c
#include <kcmod/kcmod.h>

KCMOD_OVERRIDE(int, mac_policy_register, struct mac_policy_conf* conf, mac_policy_handle_t* handlep, void* xd) {
    IOLog("mac_policy_register override: %s\n", conf->mpc_name);
    return KCMOD_SUPER(mac_policy_register, conf, handlep, xd);
}
```

For a complete example see [`kext/example_kext`](kext/example_kext)



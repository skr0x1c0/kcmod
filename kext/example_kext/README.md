# Example kext

The example kext demonstrates two main capabilities of `kcmod`, overriding functions (see [hooks.c](./hooks.c)) and linking kext (see [main.c](./main.c)) with an existing kernelcache. To build and install the example kext on your Apple SRD, follow the documentation below.

# Prerequisites

## Generating iphoneos.internal.sdk

The default iPhoneOS SDK provided with Xcode does not have the header files and libraries required for building a kernel extension for the iPhone. To build a SDK capable of this, follow below steps

1. Make sure `Terminal.app` have Full Disk Access (Settings / Privacy & Security / Full Disk Access).
2. Using terminal, execute the following commands
    ``` sh
    cd <project-root-dir>/xcode_sdk
    mkdir build
    cd build
    cmake ..
    make kcmod_apple_internal_sdk_iphoneos
    ```


## Installing img4lib

The kernelcache extracted from an `ipsw` file will be in `img4` format. `kcmod` requires kernelcache in Mach-O format. To extract Mach-O kernelcache from `img4` file and to encode it back to `img4` format you can use the `img4lib`. Execute the following commands to build and install `img4lib`

``` sh
git clone --recurse-submodules https://github.com/pinauten/img4lib
cd img4lib
COMMONCRYPTO=1 make
sudo cp img4 /usr/local/bin/
```

# Building and installing ikext to SRD device

## Building the ikext

Execute the following commands to build the kernel extension `example_kext` for iPhone

``` sh
cd <project-root-dir>/kext
mkdir build
cd build
cmake ..
make example_kext_ios_package
```

The `ikext` will be saved in `example_kext/example_kext_ios.ikext`


## Build ipsw with the ikext

1. Download the `ipsw` file for your SRD device from this [URL](https://github.com/apple/security-research-device/wiki/2.-Installing-System-Versions). In this example we will use `iPhoneOS 16.4.1` ipsw for `iPhone 13 SRD`.

    ``` sh
    wget "https://updates.cdn-apple.com/2023SpringFCS/fullrestores/032-71284/CF85AC1F-2DC7-4D1C-8221-0837335100A9/iPhone14,4_16.4.1_20E252_Restore.ipsw"
    ```

2. Extract the research kernelcache in `img4` format and convert it to Mach-O format
    ``` sh
    unzip -j iPhone14\,4_16.4.1_20E252_Restore.ipsw kernelcache.research.iphone14
    img4 -i kernelcache.research.iphone14 -o kernelcache.research.iphone14.macho
    ```

3. Link the ikext with extracted kernelcache
    ``` sh
    kcmod replace com.apple.nke.l2tp --kernelcache ./kernelcache.research.iphone14.macho --kext <project-root-dir>/kext/build/example_kext/example_kext_ios.ikext --output ./kernelcache.research.iphone14.macho.patched
    ```

4. Convert the patched kernelcache to `img4` format
    ``` sh
    img4 -i kernelcache.research.iphone14 -Rc kernelcache.research.iphone14.macho.patched -o kernelcache.research.iphone14
    ```

5. Build the `ipsw` with new kernelcache
    ``` sh
    cp iPhone14\,4_16.4.1_20E252_Restore.ipsw iPhone14\,4_16.4.1_20E252_Restore_new.ipsw
    zip iPhone14\,4_16.4.1_20E252_Restore_new.ipsw kernelcache.research.iphone14
    ``` 

Now you can flash the `iPhone14,4_16.4.1_20E252_Restore_new.ipsw` firmware using `srdutil` to your `iPhone 13 SRD`

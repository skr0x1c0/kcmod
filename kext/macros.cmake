macro(kcmod_cmake_find_xcode_sdk var name)
    set(result_code)
    execute_process(
            COMMAND xcrun -sdk ${name} --show-sdk-path
            RESULT_VARIABLE result_code
            OUTPUT_VARIABLE ${var}
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (result_code)
        message(FATAL_ERROR "Failed to find xcode sdk ${name}, result code: '${result_code})'")
    else ()
        if (${ARGV1})
            file(TO_CMAKE_PATH "${var}" ${var})
        endif ()
    endif ()
endmacro()

macro(kcmod_cmake_add_kext target sdk_name platform_name platform_version sources info_plist)
    kcmod_cmake_find_xcode_sdk(sdk_root ${sdk_name})

    add_executable(${target} ${sources})

    target_compile_options(${target} PRIVATE -nostdinc
            -std=gnu11
            -fno-builtin
            -Wno-trigraphs
            -fno-common
            -fstrict-aliasing
            -mkernel
            -Wno-implicit-function-declaration
            -arch arm64e
            -isysroot ${sdk_root}
            -target arm64e-apple-${platform_name}${platform_version})

    target_compile_definitions(${target} PRIVATE -DKERNEL
            -DKERNEL_PRIVATE
            -DDRIVER_PRIVATE
            -DAPPLE
            -DNeXT
            -DDEBUG=1)

    target_include_directories(${target} PRIVATE
            ${sdk_root}/System/Library/Frameworks/Kernel.framework/PrivateHeaders
            ${sdk_root}/System/Library/Frameworks/Kernel.framework/Headers)

    target_link_options(${target} PRIVATE
            -isysroot ${sdk_root}
            -target arm64e-apple-${platform_name}${platform_version}
            "SHELL:-arch arm64e"
            -nostdlib
            "SHELL:-Xlinker -kext"
            "SHELL:-Xlinker -export_dynamic"
            "SHELL:-Xlinker -no_deduplicate"
            "SHELL:-Xlinker -no_adhoc_codesign")

    target_link_libraries(${target} kmodc++ kmod cc_kext)

    # HACK: cmake automatically adds -arch arm64 to the compiler commands, which leads to
    # binary containing both arm64 and arm64e slices. We only need the arm64e slice
    add_custom_target(${target}_lipo
            COMMAND lipo $<TARGET_FILE:${target}> -thin arm64e -output ${CMAKE_CURRENT_BINARY_DIR}/${target}.arm64e
            BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${target}.arm64e)

    set(info_plist_out ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
    add_custom_target(${target}_plist
            COMMAND cmake -E copy ${info_plist} ${info_plist_out}
            COMMAND plutil -insert DTPlatformName -string ${platform_name} ${info_plist_out}
            COMMAND plutil -insert CFBundleSupportedPlatforms -array ${info_plist_out}
            COMMAND plutil -insert CFBundleSupportedPlatforms.0 -string ${platform_name} ${info_plist_out}
            COMMAND plutil -insert DTPlatformVersion -string ${platform_version} ${info_plist_out}
            COMMAND plutil -insert DTSDKName -string ${sdk_name} ${info_plist_out}
            COMMAND plutil -insert LSMinimumSystemVersion -string ${platform_version} ${info_plist_out}
            BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${target}.plist)

    if (${platform_name} MATCHES macosx)
        set(kext_binary_dir ${CMAKE_CURRENT_BINARY_DIR}/${target}.kext/Contents/MacOS)
        set(kext_plist_dir ${CMAKE_CURRENT_BINARY_DIR}/${target}.kext/Contents)
    elseif(${platform_name} MATCHES ios)
        set(kext_binary_dir ${CMAKE_CURRENT_BINARY_DIR}/${target}.ikext)
        set(kext_plist_dir ${CMAKE_CURRENT_BINARY_DIR}/${target}.ikext)
    else()
        message(FATAL_ERROR "Unknown platform: ${platform_name}")
    endif()

    add_custom_target(${target}_package
            COMMAND cmake -E remove_directory ${kext_plist_dir}
            COMMAND cmake -E make_directory ${kext_binary_dir}
            COMMAND cmake -E copy ${CMAKE_CURRENT_BINARY_DIR}/${target}.arm64e ${kext_binary_dir}/${target}
            COMMAND cmake -E copy ${info_plist_out} ${kext_plist_dir}/Info.plist
            DEPENDS ${target}_lipo ${target}_plist
            BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/${target}.kext)
endmacro()

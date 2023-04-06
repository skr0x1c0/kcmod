macro(find_xcode_sdk var name)
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

set(MACOS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/external/apple")
set(AVAILABILITY_VERSIONS_SOURCE_DIR "${MACOS_SOURCE_DIR}/AvailabilityVersions")
set(LIBDISPATCH_SOURCE_DIR "${MACOS_SOURCE_DIR}/libdispatch")
set(XNU_SOURCE_DIR "${MACOS_SOURCE_DIR}/xnu")
set(LIBKMOD_SOURCE_DIR ${XNU_SOURCE_DIR}/libkern/kmod)

macro(build_internal_sdk name)
    find_xcode_sdk(BASE_SDK ${name})
    if (NOT BASE_SDK)
        message(FATAL_ERROR "Could not find ${name} SDK")
    endif ()
    cmake_path(GET BASE_SDK PARENT_PATH SDK_DIR)
    if (NOT EXISTS ${SDK_DIR})
        message(FATAL_ERROR "Could not find XCode SDK directory at ${SDK_DIR}")
    endif ()

    set(INTERNAL_SDK_NAME "${name}.internal")
    set(INTERNAL_SDK "${SDK_DIR}/${INTERNAL_SDK_NAME}.sdk")

    set(TARGET_NAME kcmod_apple_internal_sdk_${name})
    set(BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}")

    set(INTERNAL_SDK_USER_INCLUDE_DIR "${INTERNAL_SDK}/usr/include")
    set(INTERNAL_SDK_KERNEL_HEADERS_DIR "${INTERNAL_SDK}/System/Library/Frameworks/Kernel.framework/Headers")

    add_custom_target(
            ${TARGET_NAME}_clone
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${INTERNAL_SDK}"
            COMMAND ditto "${BASE_SDK}" "${INTERNAL_SDK}"
            COMMAND plutil -replace "DisplayName" -string "${INTERNAL_SDK_NAME}" "${INTERNAL_SDK}/SDKSettings.plist"
            COMMAND plutil -replace "CanonicalName" -string "${INTERNAL_SDK_NAME}" "${INTERNAL_SDK}/SDKSettings.plist"
            COMMAND plutil -convert json -o "${INTERNAL_SDK}/SDKSettings.json" "${INTERNAL_SDK}/SDKSettings.plist"
    )

    add_custom_target(
            ${TARGET_NAME}_availability_versions
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${BINARY_DIR}/AvailabilityVersions"
            COMMAND make -C "${AVAILABILITY_VERSIONS_SOURCE_DIR}" DSTROOT="${BINARY_DIR}/AvailabilityVersions" install
            COMMAND ditto "${BINARY_DIR}/AvailabilityVersions/usr/local/libexec" "${INTERNAL_SDK}/usr/local/libexec"
            DEPENDS ${TARGET_NAME}_clone
    )

    add_custom_target(
            ${TARGET_NAME}_xnu_headers
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${BINARY_DIR}/xnu_headers"
            COMMAND make -C "${XNU_SOURCE_DIR}" SDKROOT="${INTERNAL_SDK_NAME}" ARCH_CONFIGS="ARM64" DSTROOT="${BINARY_DIR}/xnu_headers" installhdrs
            COMMAND ditto "${BINARY_DIR}/xnu_headers" "${INTERNAL_SDK}"
            DEPENDS ${TARGET_NAME}_availability_versions
            COMMAND ln -sf "${INTERNAL_SDK_USER_INCLUDE_DIR}/AssertMacros.h" "${INTERNAL_SDK_KERNEL_HEADERS_DIR}/AssertMacros.h"
            COMMAND ln -sf "${INTERNAL_SDK_USER_INCLUDE_DIR}/Availability.h" "${INTERNAL_SDK_KERNEL_HEADERS_DIR}/Availability.h"
            COMMAND ln -sf "${INTERNAL_SDK_USER_INCLUDE_DIR}/AvailabilityInternal.h" "${INTERNAL_SDK_KERNEL_HEADERS_DIR}/AvailabilityInternal.h"
            COMMAND ln -sf "${INTERNAL_SDK_USER_INCLUDE_DIR}/AvailabilityMacros.h" "${INTERNAL_SDK_KERNEL_HEADERS_DIR}/AvailabilityMacros.h"
            COMMAND ln -sf "${INTERNAL_SDK_USER_INCLUDE_DIR}/AvailabilityVersions.h" "${INTERNAL_SDK_KERNEL_HEADERS_DIR}/AvailabilityVersions.h"
            COMMAND ln -sf "${INTERNAL_SDK_USER_INCLUDE_DIR}/TargetConditionals.h" "${INTERNAL_SDK_KERNEL_HEADERS_DIR}/TargetConditionals.h"
    )

    add_custom_target(
            ${TARGET_NAME}_libdispatch
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${BINARY_DIR}/libdispatch"
            COMMAND xcodebuild install -sdk ${INTERNAL_SDK_NAME} ARCHS="arm64e" -target libfirehose_kernel PRODUCT_NAME=firehose_kernel DSTROOT="${BINARY_DIR}/libdispatch"
            WORKING_DIRECTORY "${LIBDISPATCH_SOURCE_DIR}"
            COMMAND ditto "${BINARY_DIR}/libdispatch/usr/local" "${INTERNAL_SDK}/usr/local"
            DEPENDS ${TARGET_NAME}_xnu_headers
    )

    add_custom_target(
            ${TARGET_NAME}_libkmod
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${BINARY_DIR}/libkmod"
            COMMAND xcodebuild install -sdk ${INTERNAL_SDK_NAME} ARCHS="arm64e" -target libkmod PRODUCT_NAME=kmod DSTROOT="${BINARY_DIR}/libkmod" OTHER_CFLAGS="-Wno-incompatible-sysroot"
            COMMAND xcodebuild install -sdk ${INTERNAL_SDK_NAME} ARCHS="arm64e" -target libkmodc++ PRODUCT_NAME=kmodc++ DSTROOT="${BINARY_DIR}/libkmod" OTHER_CFLAGS="-Wno-incompatible-sysroot"
            WORKING_DIRECTORY "${LIBKMOD_SOURCE_DIR}"
            COMMAND ditto "${BINARY_DIR}/libkmod/usr" "${INTERNAL_SDK}/usr"
            DEPENDS ${TARGET_NAME}_xnu_headers
    )

    add_custom_target(
            ${TARGET_NAME}
            DEPENDS
            ${TARGET_NAME}_clone
            ${TARGET_NAME}_availability_versions
            ${TARGET_NAME}_xnu_headers
            ${TARGET_NAME}_libdispatch
            ${TARGET_NAME}_libkmod
    )
endmacro()

build_internal_sdk(macosx)
build_internal_sdk(iphoneos)

add_custom_target(kcmod_install_apple_internal_sdks
        DEPENDS kcmod_apple_internal_sdk_macosx kcmod_apple_internal_sdk_iphoneos)

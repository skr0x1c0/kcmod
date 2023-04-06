set(C_SOURCES
        hooks.c
        info.c
        main.c)

set(INFO_PLIST
        ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)

set(MOD_LIBS
        kcmod_hooks)

kcmod_cmake_add_kext(example_kext_macosx macosx.internal macosx 13.2 "${C_SOURCES}" ${INFO_PLIST})
target_link_libraries(example_kext_macosx ${MOD_LIBS})

kcmod_cmake_add_kext(example_kext_ios iphoneos.internal ios 16.2 "${C_SOURCES}" ${INFO_PLIST})
target_link_libraries(example_kext_ios ${MOD_LIBS})

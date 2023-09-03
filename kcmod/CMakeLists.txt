cmake_minimum_required(VERSION 3.25)
project(kcmod)

set(CMAKE_CXX_STANDARD 20)

set(CXX_HEADERS
        include/kcmod/aarch64.h
        include/kcmod/common.h
        include/kcmod/debug.h
        include/kcmod/fixup_chain.h
        include/kcmod/hooks.h
        include/kcmod/kernelcache.h
        include/kcmod/kext.h
        include/kcmod/log.h
        include/kcmod/macho.h
        include/kcmod/memio.h
        include/kcmod/plist.h
        include/kcmod/split_seg.h
        include/kcmod/symidx.h
        include/kcmod/temp.h)

set(CXX_SRC
        src/aarch64.cpp
        src/fixup_chain.cpp
        src/main.cpp
        src/hooks.cpp
        src/kernelcache.cpp
        src/kext.cpp
        src/plist.cpp
        src/split_seg.cpp
        src/symidx.cpp
        src/temp.cpp)

add_executable(kcmod ${CXX_SRC} ${CXX_HEADERS})
target_include_directories(kcmod PRIVATE include/kcmod)
target_link_libraries(kcmod mio fmt docopt nlohmann_json)
target_link_libraries(kcmod "-framework Foundation")

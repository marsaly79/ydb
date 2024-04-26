# Generated by devtools/yamaker.

INCLUDE(${ARCADIA_ROOT}/build/platform/clang/arch.cmake)

LIBRARY(clang_rt.hwasan_cxx${CLANG_RT_SUFFIX})

LICENSE(
    Apache-2.0 AND
    Apache-2.0 WITH LLVM-exception AND
    MIT AND
    NCSA
)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

SUBSCRIBER(g:cpp-contrib)

ADDINCL(
    contrib/libs/clang14-rt/lib
)

NO_COMPILER_WARNINGS()

NO_UTIL()

NO_SANITIZE()

CFLAGS(
    -DHWASAN_WITH_INTERCEPTORS=1
    -DUBSAN_CAN_USE_CXXABI
    -fcommon
    -ffreestanding
    -fno-builtin
    -fno-exceptions
    -fno-lto
    -fno-rtti
    -fno-stack-protector
    -fomit-frame-pointer
    -frtti
    -funwind-tables
    -fvisibility=hidden
)

SRCDIR(contrib/libs/clang14-rt/lib)

SRCS(
    hwasan/hwasan_new_delete.cpp
    ubsan/ubsan_handlers_cxx.cpp
    ubsan/ubsan_type_hash.cpp
    ubsan/ubsan_type_hash_itanium.cpp
    ubsan/ubsan_type_hash_win.cpp
)

END()

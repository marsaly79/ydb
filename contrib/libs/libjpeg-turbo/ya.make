# Generated by devtools/yamaker from nixpkgs 22.05.

LIBRARY()

LICENSE(
    BSD-3-Clause AND
    Beerware AND
    IJG AND
    Libpbm AND
    Public-Domain AND
    Zlib
)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

VERSION(2.1.4)

ORIGINAL_SOURCE(https://github.com/libjpeg-turbo/libjpeg-turbo/archive/2.1.4.tar.gz)

ADDINCL(
    contrib/libs/libjpeg-turbo
    FOR
    asm
    contrib/libs/libjpeg-turbo/simd/nasm
)

IF (OS_DARWIN OR OS_IOS)
    SET(ASM_PREFIX '_')
ENDIF()

NO_COMPILER_WARNINGS()

NO_RUNTIME()

CFLAGS(
    -DBMP_SUPPORTED
    -DPPM_SUPPORTED
)

IF (SANITIZER_TYPE)
    CFLAGS(
        -DWITH_SANITIZER
    )
ENDIF()

IF (OS_ANDROID)
    SRCS(
        jsimd_none.c
    )
ELSEIF (ARCH_I386)
    SRCS(
        simd/i386/jccolor-avx2.asm
        simd/i386/jccolor-mmx.asm
        simd/i386/jccolor-sse2.asm
        simd/i386/jcgray-avx2.asm
        simd/i386/jcgray-mmx.asm
        simd/i386/jcgray-sse2.asm
        simd/i386/jchuff-sse2.asm
        simd/i386/jcphuff-sse2.asm
        simd/i386/jcsample-avx2.asm
        simd/i386/jcsample-mmx.asm
        simd/i386/jcsample-sse2.asm
        simd/i386/jdcolor-avx2.asm
        simd/i386/jdcolor-mmx.asm
        simd/i386/jdcolor-sse2.asm
        simd/i386/jdmerge-avx2.asm
        simd/i386/jdmerge-mmx.asm
        simd/i386/jdmerge-sse2.asm
        simd/i386/jdsample-avx2.asm
        simd/i386/jdsample-mmx.asm
        simd/i386/jdsample-sse2.asm
        simd/i386/jfdctflt-3dn.asm
        simd/i386/jfdctflt-sse.asm
        simd/i386/jfdctfst-mmx.asm
        simd/i386/jfdctfst-sse2.asm
        simd/i386/jfdctint-avx2.asm
        simd/i386/jfdctint-mmx.asm
        simd/i386/jfdctint-sse2.asm
        simd/i386/jidctflt-3dn.asm
        simd/i386/jidctflt-sse.asm
        simd/i386/jidctflt-sse2.asm
        simd/i386/jidctfst-mmx.asm
        simd/i386/jidctfst-sse2.asm
        simd/i386/jidctint-avx2.asm
        simd/i386/jidctint-mmx.asm
        simd/i386/jidctint-sse2.asm
        simd/i386/jidctred-mmx.asm
        simd/i386/jidctred-sse2.asm
        simd/i386/jquant-3dn.asm
        simd/i386/jquant-mmx.asm
        simd/i386/jquant-sse.asm
        simd/i386/jquantf-sse2.asm
        simd/i386/jquanti-avx2.asm
        simd/i386/jquanti-sse2.asm
        simd/i386/jsimd.c
        simd/i386/jsimdcpu.asm
    )
ELSEIF (ARCH_X86_64)
    SRCS(
        simd/x86_64/jccolor-avx2.asm
        simd/x86_64/jccolor-sse2.asm
        simd/x86_64/jcgray-avx2.asm
        simd/x86_64/jcgray-sse2.asm
        simd/x86_64/jchuff-sse2.asm
        simd/x86_64/jcphuff-sse2.asm
        simd/x86_64/jcsample-avx2.asm
        simd/x86_64/jcsample-sse2.asm
        simd/x86_64/jdcolor-avx2.asm
        simd/x86_64/jdcolor-sse2.asm
        simd/x86_64/jdmerge-avx2.asm
        simd/x86_64/jdmerge-sse2.asm
        simd/x86_64/jdsample-avx2.asm
        simd/x86_64/jdsample-sse2.asm
        simd/x86_64/jfdctflt-sse.asm
        simd/x86_64/jfdctfst-sse2.asm
        simd/x86_64/jfdctint-avx2.asm
        simd/x86_64/jfdctint-sse2.asm
        simd/x86_64/jidctflt-sse2.asm
        simd/x86_64/jidctfst-sse2.asm
        simd/x86_64/jidctint-avx2.asm
        simd/x86_64/jidctint-sse2.asm
        simd/x86_64/jidctred-sse2.asm
        simd/x86_64/jquantf-sse2.asm
        simd/x86_64/jquanti-avx2.asm
        simd/x86_64/jquanti-sse2.asm
        simd/x86_64/jsimd.c
        simd/x86_64/jsimdcpu.asm
    )
ELSEIF (ARCH_ARM7 AND NOT MSVC)
    SRCS(
        simd/arm/aarch32/jchuff-neon.c
        simd/arm/aarch32/jsimd.c
        simd/arm/aarch32/jsimd_neon.S
    )
ELSEIF (ARCH_ARM64 AND NOT MSVC)
    ADDINCL(
        contrib/libs/libjpeg-turbo/simd/arm
    )
    SRCS(
        simd/arm/aarch64/jchuff-neon.c
        simd/arm/aarch64/jsimd.c
        simd/arm/jccolor-neon.c
        simd/arm/jcgray-neon.c
        simd/arm/jcphuff-neon.c
        simd/arm/jcsample-neon.c
        simd/arm/jdcolor-neon.c
        simd/arm/jdmerge-neon.c
        simd/arm/jdsample-neon.c
        simd/arm/jfdctfst-neon.c
        simd/arm/jfdctint-neon.c
        simd/arm/jidctfst-neon.c
        simd/arm/jidctint-neon.c
        simd/arm/jidctred-neon.c
        simd/arm/jquanti-neon.c
    )
ELSE()
    SRCS(
        jsimd_none.c
    )
ENDIF()

SRCS(
    jaricom.c
    jcapimin.c
    jcapistd.c
    jcarith.c
    jccoefct.c
    jccolor.c
    jcdctmgr.c
    jchuff.c
    jcicc.c
    jcinit.c
    jcmainct.c
    jcmarker.c
    jcmaster.c
    jcomapi.c
    jcparam.c
    jcphuff.c
    jcprepct.c
    jcsample.c
    jctrans.c
    jdapimin.c
    jdapistd.c
    jdarith.c
    jdatadst-tj.c
    jdatadst.c
    jdatasrc-tj.c
    jdatasrc.c
    jdcoefct.c
    jdcolor.c
    jddctmgr.c
    jdhuff.c
    jdicc.c
    jdinput.c
    jdmainct.c
    jdmarker.c
    jdmaster.c
    jdmerge.c
    jdphuff.c
    jdpostct.c
    jdsample.c
    jdtrans.c
    jerror.c
    jfdctflt.c
    jfdctfst.c
    jfdctint.c
    jidctflt.c
    jidctfst.c
    jidctint.c
    jidctred.c
    jmemmgr.c
    jmemnobs.c
    jquant1.c
    jquant2.c
    jutils.c
    rdbmp.c
    rdppm.c
    transupp.c
    turbojpeg.c
    wrbmp.c
    wrppm.c
)

END()

RECURSE(
    cjpeg
    djpeg
    jpegtran
    tjunittest
)

IF (NOT OS_ANDROID AND NOT OS_IOS)
    RECURSE_FOR_TESTS(
        ut
    )
ENDIF()
/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// Modified by Yao Wei Tjong for Urho3D

#ifdef TEST_MAIN
#include "SDL_config.h"
#else
#include "../SDL_internal.h"
#endif

#if defined(__WIN32__) || defined(__WINRT__)
#include "../core/windows/SDL_windows.h"
#endif
#if defined(__OS2__)
#define INCL_DOS
#include <os2.h>
#ifndef QSV_NUMPROCESSORS
#define QSV_NUMPROCESSORS 26
#endif
#endif

/* CPU feature detection for SDL */

#include "SDL_cpuinfo.h"
#include "SDL_assert.h"

#ifdef HAVE_SYSCONF
#include <unistd.h>
#endif
#ifdef HAVE_SYSCTLBYNAME
#include <sys/types.h>
#include <sys/sysctl.h>
#endif
#if defined(__MACOSX__) && (defined(__ppc__) || defined(__ppc64__))
#include <sys/sysctl.h>         /* For AltiVec check */
#elif defined(__OpenBSD__) && defined(__powerpc__)
#include <sys/param.h>
#include <sys/sysctl.h> /* For AltiVec check */
#include <machine/cpu.h>
#elif SDL_ALTIVEC_BLITTERS && HAVE_SETJMP
#include <signal.h>
#include <setjmp.h>
#endif

#if defined(__QNXNTO__)
#include <sys/syspage.h>
#endif

#if (defined(__LINUX__) || defined(__ANDROID__)) && defined(__ARM_ARCH)
/*#include <asm/hwcap.h>*/
#ifndef AT_HWCAP
#define AT_HWCAP 16
#endif
#ifndef HWCAP_NEON
#define HWCAP_NEON (1 << 12)
#endif
#if defined HAVE_GETAUXVAL
#include <sys/auxv.h>
#else
#include <fcntl.h>
#endif
#endif

#if defined(__ANDROID__) && defined(__ARM_ARCH) && !defined(HAVE_GETAUXVAL)
#if __ARM_ARCH < 8
#include <cpu-features.h>
#endif
#endif

#define CPU_HAS_RDTSC   (1 << 0)
#define CPU_HAS_ALTIVEC (1 << 1)
#define CPU_HAS_MMX     (1 << 2)
#define CPU_HAS_3DNOW   (1 << 3)
#define CPU_HAS_SSE     (1 << 4)
#define CPU_HAS_SSE2    (1 << 5)
#define CPU_HAS_SSE3    (1 << 6)
#define CPU_HAS_SSE41   (1 << 7)
#define CPU_HAS_SSE42   (1 << 8)
#define CPU_HAS_AVX     (1 << 9)
#define CPU_HAS_AVX2    (1 << 10)
#define CPU_HAS_NEON    (1 << 11)
#define CPU_HAS_AVX512F (1 << 12)

#if SDL_ALTIVEC_BLITTERS && HAVE_SETJMP && !__MACOSX__ && !__OpenBSD__
/* This is the brute force way of detecting instruction sets...
   the idea is borrowed from the libmpeg2 library - thanks!
 */
static jmp_buf jmpbuf;
static void
illegal_instruction(int sig)
{
    longjmp(jmpbuf, 1);
}
#endif /* HAVE_SETJMP */

static int
CPU_haveCPUID(void)
{
    int has_CPUID = 0;

/* *INDENT-OFF* */
#ifndef SDL_CPUINFO_DISABLED
#if defined(__GNUC__) && defined(i386)
    __asm__ (
"        pushfl                      # Get original EFLAGS             \n"
"        popl    %%eax                                                 \n"
"        movl    %%eax,%%ecx                                           \n"
"        xorl    $0x200000,%%eax     # Flip ID bit in EFLAGS           \n"
"        pushl   %%eax               # Save new EFLAGS value on stack  \n"
"        popfl                       # Replace current EFLAGS value    \n"
"        pushfl                      # Get new EFLAGS                  \n"
"        popl    %%eax               # Store new EFLAGS in EAX         \n"
"        xorl    %%ecx,%%eax         # Can not toggle ID bit,          \n"
"        jz      1f                  # Processor=80486                 \n"
"        movl    $1,%0               # We have CPUID support           \n"
"1:                                                                    \n"
    : "=m" (has_CPUID)
    :
    : "%eax", "%ecx"
    );
#elif defined(__GNUC__) && defined(__x86_64__)
/* Technically, if this is being compiled under __x86_64__ then it has 
   CPUid by definition.  But it's nice to be able to prove it.  :)      */
    __asm__ (
"        pushfq                      # Get original EFLAGS             \n"
"        popq    %%rax                                                 \n"
"        movq    %%rax,%%rcx                                           \n"
"        xorl    $0x200000,%%eax     # Flip ID bit in EFLAGS           \n"
"        pushq   %%rax               # Save new EFLAGS value on stack  \n"
"        popfq                       # Replace current EFLAGS value    \n"
"        pushfq                      # Get new EFLAGS                  \n"
"        popq    %%rax               # Store new EFLAGS in EAX         \n"
"        xorl    %%ecx,%%eax         # Can not toggle ID bit,          \n"
"        jz      1f                  # Processor=80486                 \n"
"        movl    $1,%0               # We have CPUID support           \n"
"1:                                                                    \n"
    : "=m" (has_CPUID)
    :
    : "%rax", "%rcx"
    );
#elif (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)
    __asm {
        pushfd                      ; Get original EFLAGS
        pop     eax
        mov     ecx, eax
        xor     eax, 200000h        ; Flip ID bit in EFLAGS
        push    eax                 ; Save new EFLAGS value on stack
        popfd                       ; Replace current EFLAGS value
        pushfd                      ; Get new EFLAGS
        pop     eax                 ; Store new EFLAGS in EAX
        xor     eax, ecx            ; Can not toggle ID bit,
        jz      done                ; Processor=80486
        mov     has_CPUID,1         ; We have CPUID support
done:
    }
#elif defined(_MSC_VER) && defined(_M_X64)
    has_CPUID = 1;
#elif defined(__sun) && defined(__i386)
    __asm (
"       pushfl                 \n"
"       popl    %eax           \n"
"       movl    %eax,%ecx      \n"
"       xorl    $0x200000,%eax \n"
"       pushl   %eax           \n"
"       popfl                  \n"
"       pushfl                 \n"
"       popl    %eax           \n"
"       xorl    %ecx,%eax      \n"
"       jz      1f             \n"
"       movl    $1,-8(%ebp)    \n"
"1:                            \n"
    );
#elif defined(__sun) && defined(__amd64)
    __asm (
"       pushfq                 \n"
"       popq    %rax           \n"
"       movq    %rax,%rcx      \n"
"       xorl    $0x200000,%eax \n"
"       pushq   %rax           \n"
"       popfq                  \n"
"       pushfq                 \n"
"       popq    %rax           \n"
"       xorl    %ecx,%eax      \n"
"       jz      1f             \n"
"       movl    $1,-8(%rbp)    \n"
"1:                            \n"
    );
#endif
#endif
/* *INDENT-ON* */
    return has_CPUID;
}

#if defined(__GNUC__) && defined(i386)
#define cpuid(func, a, b, c, d) \
    __asm__ __volatile__ ( \
"        pushl %%ebx        \n" \
"        xorl %%ecx,%%ecx   \n" \
"        cpuid              \n" \
"        movl %%ebx, %%esi  \n" \
"        popl %%ebx         \n" : \
            "=a" (a), "=S" (b), "=c" (c), "=d" (d) : "a" (func))
#elif defined(__GNUC__) && defined(__x86_64__)
#define cpuid(func, a, b, c, d) \
    __asm__ __volatile__ ( \
"        pushq %%rbx        \n" \
"        xorq %%rcx,%%rcx   \n" \
"        cpuid              \n" \
"        movq %%rbx, %%rsi  \n" \
"        popq %%rbx         \n" : \
            "=a" (a), "=S" (b), "=c" (c), "=d" (d) : "a" (func))
#elif (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)
#define cpuid(func, a, b, c, d) \
    __asm { \
        __asm mov eax, func \
        __asm xor ecx, ecx \
        __asm cpuid \
        __asm mov a, eax \
        __asm mov b, ebx \
        __asm mov c, ecx \
        __asm mov d, edx \
}
#elif defined(_MSC_VER) && defined(_M_X64)
#define cpuid(func, a, b, c, d) \
{ \
    int CPUInfo[4]; \
    __cpuid(CPUInfo, func); \
    a = CPUInfo[0]; \
    b = CPUInfo[1]; \
    c = CPUInfo[2]; \
    d = CPUInfo[3]; \
}
#else
#define cpuid(func, a, b, c, d) \
    do { a = b = c = d = 0; (void) a; (void) b; (void) c; (void) d; } while (0)
#endif

static int CPU_CPUIDFeatures[4];
static int CPU_CPUIDMaxFunction = 0;
static SDL_bool CPU_OSSavesYMM = SDL_FALSE;
static SDL_bool CPU_OSSavesZMM = SDL_FALSE;

static void
CPU_calcCPUIDFeatures(void)
{
    static SDL_bool checked = SDL_FALSE;
    if (!checked) {
        checked = SDL_TRUE;
        if (CPU_haveCPUID()) {
            int a, b, c, d;
            cpuid(0, a, b, c, d);
            CPU_CPUIDMaxFunction = a;
            if (CPU_CPUIDMaxFunction >= 1) {
                cpuid(1, a, b, c, d);
                CPU_CPUIDFeatures[0] = a;
                CPU_CPUIDFeatures[1] = b;
                CPU_CPUIDFeatures[2] = c;
                CPU_CPUIDFeatures[3] = d;

                /* Check to make sure we can call xgetbv */
                if (c & 0x08000000) {
                    /* Call xgetbv to see if YMM (etc) register state is saved */
#if defined(__GNUC__) && (defined(i386) || defined(__x86_64__))
                    __asm__(".byte 0x0f, 0x01, 0xd0" : "=a" (a) : "c" (0) : "%edx");
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64)) && (_MSC_FULL_VER >= 160040219) /* VS2010 SP1 */
                    a = (int)_xgetbv(0);
#elif (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)
                    __asm
                    {
                        xor ecx, ecx
                        _asm _emit 0x0f _asm _emit 0x01 _asm _emit 0xd0
                        mov a, eax
                    }
#endif
                    CPU_OSSavesYMM = ((a & 6) == 6) ? SDL_TRUE : SDL_FALSE;
                    CPU_OSSavesZMM = (CPU_OSSavesYMM && ((a & 0xe0) == 0xe0)) ? SDL_TRUE : SDL_FALSE;
                }
            }
        }
    }
}

static int
CPU_haveAltiVec(void)
{
    volatile int altivec = 0;
#ifndef SDL_CPUINFO_DISABLED
#if (defined(__MACOSX__) && (defined(__ppc__) || defined(__ppc64__))) || (defined(__OpenBSD__) && defined(__powerpc__))
#ifdef __OpenBSD__
    int selectors[2] = { CTL_MACHDEP, CPU_ALTIVEC };
#else
    int selectors[2] = { CTL_HW, HW_VECTORUNIT };
#endif
    int hasVectorUnit = 0;
    size_t length = sizeof(hasVectorUnit);
    int error = sysctl(selectors, 2, &hasVectorUnit, &length, NULL, 0);
    if (0 == error)
        altivec = (hasVectorUnit != 0);
#elif SDL_ALTIVEC_BLITTERS && HAVE_SETJMP
    void (*handler) (int sig);
    handler = signal(SIGILL, illegal_instruction);
    if (setjmp(jmpbuf) == 0) {
        asm volatile ("mtspr 256, %0\n\t" "vand %%v0, %%v0, %%v0"::"r" (-1));
        altivec = 1;
    }
    signal(SIGILL, handler);
#endif
#endif
    return altivec;
}

#if defined(__LINUX__) && defined(__ARM_ARCH) && !defined(HAVE_GETAUXVAL)
static int
readProcAuxvForNeon(void)
{
    int neon = 0;
    int kv[2];
    const int fd = open("/proc/self/auxv", O_RDONLY);
    if (fd != -1) {
        while (read(fd, kv, sizeof (kv)) == sizeof (kv)) {
            if (kv[0] == AT_HWCAP) {
                neon = ((kv[1] & HWCAP_NEON) == HWCAP_NEON);
                break;
            }
        }
        close(fd);
    }
    return neon;
}
#endif


static int
CPU_haveNEON(void)
{
/* The way you detect NEON is a privileged instruction on ARM, so you have
   query the OS kernel in a platform-specific way. :/ */
#if defined(SDL_CPUINFO_DISABLED)
   return 0; /* disabled */
#elif (defined(__WINDOWS__) || defined(__WINRT__)) && (defined(_M_ARM) || defined(_M_ARM64))
/* Visual Studio, for ARM, doesn't define __ARM_ARCH. Handle this first. */
/* Seems to have been removed */
#  if !defined(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE)
#    define PF_ARM_NEON_INSTRUCTIONS_AVAILABLE 19
#  endif
/* All WinRT ARM devices are required to support NEON, but just in case. */
    return IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE) != 0;
#elif !defined(__ARM_ARCH)
    return 0;  /* not an ARM CPU at all. */
#elif __ARM_ARCH >= 8
    return 1;  /* ARMv8 always has non-optional NEON support. */
#elif defined(__APPLE__) && (__ARM_ARCH >= 7)
    /* (note that sysctlbyname("hw.optional.neon") doesn't work!) */
    return 1;  /* all Apple ARMv7 chips and later have NEON. */
#elif defined(__APPLE__)
    return 0;  /* assume anything else from Apple doesn't have NEON. */
#elif defined(__QNXNTO__)
    return SYSPAGE_ENTRY(cpuinfo)->flags & ARM_CPU_FLAG_NEON;
#elif (defined(__LINUX__) || defined(__ANDROID__)) && defined(HAVE_GETAUXVAL)
    return ((getauxval(AT_HWCAP) & HWCAP_NEON) == HWCAP_NEON);
#elif defined(__LINUX__)
    return readProcAuxvForNeon();
#elif defined(__ANDROID__)
    /* Use NDK cpufeatures to read either /proc/self/auxv or /proc/cpuinfo */
    {
        AndroidCpuFamily cpu_family = android_getCpuFamily();
        if (cpu_family == ANDROID_CPU_FAMILY_ARM) {
            uint64_t cpu_features = android_getCpuFeatures();
            if ((cpu_features & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
                return 1;
            }
        }
        return 0;
    }
#else
#warning SDL_HasNEON is not implemented for this ARM platform. Write me.
    return 0;
#endif
}

static int
CPU_have3DNow(void)
{
    if (CPU_CPUIDMaxFunction > 0) {  /* that is, do we have CPUID at all? */
        int a, b, c, d;
        cpuid(0x80000000, a, b, c, d);
        if (a >= 0x80000001) {
            cpuid(0x80000001, a, b, c, d);
            return (d & 0x80000000);
        }
    }
    return 0;
}

#define CPU_haveRDTSC() (CPU_CPUIDFeatures[3] & 0x00000010)
#define CPU_haveMMX() (CPU_CPUIDFeatures[3] & 0x00800000)
#define CPU_haveSSE() (CPU_CPUIDFeatures[3] & 0x02000000)
#define CPU_haveSSE2() (CPU_CPUIDFeatures[3] & 0x04000000)
#define CPU_haveSSE3() (CPU_CPUIDFeatures[2] & 0x00000001)
#define CPU_haveSSE41() (CPU_CPUIDFeatures[2] & 0x00080000)
#define CPU_haveSSE42() (CPU_CPUIDFeatures[2] & 0x00100000)
#define CPU_haveAVX() (CPU_OSSavesYMM && (CPU_CPUIDFeatures[2] & 0x10000000))

static int
CPU_haveAVX2(void)
{
    if (CPU_OSSavesYMM && (CPU_CPUIDMaxFunction >= 7)) {
        int a, b, c, d;
        (void) a; (void) b; (void) c; (void) d;  /* compiler warnings... */
        cpuid(7, a, b, c, d);
        return (b & 0x00000020);
    }
    return 0;
}

static int
CPU_haveAVX512F(void)
{
    if (CPU_OSSavesZMM && (CPU_CPUIDMaxFunction >= 7)) {
        int a, b, c, d;
        (void) a; (void) b; (void) c; (void) d;  /* compiler warnings... */
        cpuid(7, a, b, c, d);
        return (b & 0x00010000);
    }
    return 0;
}

static int SDL_CPUCount = 0;

int
SDL_GetCPUCount(void)
{
    if (!SDL_CPUCount) {
#ifndef SDL_CPUINFO_DISABLED
#if defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
        if (SDL_CPUCount <= 0) {
            SDL_CPUCount = (int)sysconf(_SC_NPROCESSORS_ONLN);
        }
#endif
#ifdef HAVE_SYSCTLBYNAME
        if (SDL_CPUCount <= 0) {
            size_t size = sizeof(SDL_CPUCount);
            sysctlbyname("hw.ncpu", &SDL_CPUCount, &size, NULL, 0);
        }
#endif
#ifdef __WIN32__
        if (SDL_CPUCount <= 0) {
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            SDL_CPUCount = info.dwNumberOfProcessors;
        }
#endif
#ifdef __OS2__
        if (SDL_CPUCount <= 0) {
            DosQuerySysInfo(QSV_NUMPROCESSORS, QSV_NUMPROCESSORS,
                            &SDL_CPUCount, sizeof(SDL_CPUCount) );
        }
#endif
#endif
        /* There has to be at least 1, right? :) */
        if (SDL_CPUCount <= 0) {
            SDL_CPUCount = 1;
        }
    }
    return SDL_CPUCount;
}

/* Oh, such a sweet sweet trick, just not very useful. :) */
static const char *
SDL_GetCPUType(void)
{
    static char SDL_CPUType[13];

    if (!SDL_CPUType[0]) {
        int i = 0;

        CPU_calcCPUIDFeatures();
        if (CPU_CPUIDMaxFunction > 0) {  /* do we have CPUID at all? */
            int a, b, c, d;
            cpuid(0x00000000, a, b, c, d);
            (void) a;
            SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
            SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
            SDL_CPUType[i++] = (char)(b & 0xff); b >>= 8;
            SDL_CPUType[i++] = (char)(b & 0xff);

            SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
            SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
            SDL_CPUType[i++] = (char)(d & 0xff); d >>= 8;
            SDL_CPUType[i++] = (char)(d & 0xff);

            SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
            SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
            SDL_CPUType[i++] = (char)(c & 0xff); c >>= 8;
            SDL_CPUType[i++] = (char)(c & 0xff);
        }
        if (!SDL_CPUType[0]) {
            SDL_strlcpy(SDL_CPUType, "Unknown", sizeof(SDL_CPUType));
        }
    }
    return SDL_CPUType;
}


#ifdef TEST_MAIN  /* !!! FIXME: only used for test at the moment. */
static const char *
SDL_GetCPUName(void)
{
    static char SDL_CPUName[48];

    if (!SDL_CPUName[0]) {
        int i = 0;
        int a, b, c, d;

        CPU_calcCPUIDFeatures();
        if (CPU_CPUIDMaxFunction > 0) {  /* do we have CPUID at all? */
            cpuid(0x80000000, a, b, c, d);
            if (a >= 0x80000004) {
                cpuid(0x80000002, a, b, c, d);
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                cpuid(0x80000003, a, b, c, d);
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                cpuid(0x80000004, a, b, c, d);
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(a & 0xff); a >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(b & 0xff); b >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(c & 0xff); c >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
                SDL_CPUName[i++] = (char)(d & 0xff); d >>= 8;
            }
        }
        if (!SDL_CPUName[0]) {
            SDL_strlcpy(SDL_CPUName, "Unknown", sizeof(SDL_CPUName));
        }
    }
    return SDL_CPUName;
}
#endif

int
SDL_GetCPUCacheLineSize(void)
{
    const char *cpuType = SDL_GetCPUType();
    int a, b, c, d;
    (void) a; (void) b; (void) c; (void) d;
    if (SDL_strcmp(cpuType, "GenuineIntel") == 0) {
        cpuid(0x00000001, a, b, c, d);
        return (((b >> 8) & 0xff) * 8);
    } else if (SDL_strcmp(cpuType, "AuthenticAMD") == 0) {
        cpuid(0x80000005, a, b, c, d);
        return (c & 0xff);
    } else {
        /* Just make a guess here... */
        return SDL_CACHELINE_SIZE;
    }
}

static Uint32 SDL_CPUFeatures = 0xFFFFFFFF;
static Uint32 SDL_SIMDAlignment = 0xFFFFFFFF;

static Uint32
SDL_GetCPUFeatures(void)
{
    if (SDL_CPUFeatures == 0xFFFFFFFF) {
        CPU_calcCPUIDFeatures();
        SDL_CPUFeatures = 0;
        SDL_SIMDAlignment = 4;  /* a good safe base value */
        if (CPU_haveRDTSC()) {
            SDL_CPUFeatures |= CPU_HAS_RDTSC;
        }
        if (CPU_haveAltiVec()) {
            SDL_CPUFeatures |= CPU_HAS_ALTIVEC;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 16);
        }
        if (CPU_haveMMX()) {
            SDL_CPUFeatures |= CPU_HAS_MMX;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 8);
        }
        if (CPU_have3DNow()) {
            SDL_CPUFeatures |= CPU_HAS_3DNOW;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 8);
        }
        if (CPU_haveSSE()) {
            SDL_CPUFeatures |= CPU_HAS_SSE;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 16);
        }
        if (CPU_haveSSE2()) {
            SDL_CPUFeatures |= CPU_HAS_SSE2;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 16);
        }
        if (CPU_haveSSE3()) {
            SDL_CPUFeatures |= CPU_HAS_SSE3;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 16);
        }
        if (CPU_haveSSE41()) {
            SDL_CPUFeatures |= CPU_HAS_SSE41;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 16);
        }
        if (CPU_haveSSE42()) {
            SDL_CPUFeatures |= CPU_HAS_SSE42;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 16);
        }
        if (CPU_haveAVX()) {
            SDL_CPUFeatures |= CPU_HAS_AVX;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 32);
        }
        if (CPU_haveAVX2()) {
            SDL_CPUFeatures |= CPU_HAS_AVX2;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 32);
        }
        if (CPU_haveAVX512F()) {
            SDL_CPUFeatures |= CPU_HAS_AVX512F;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 64);
        }
        if (CPU_haveNEON()) {
            SDL_CPUFeatures |= CPU_HAS_NEON;
            SDL_SIMDAlignment = SDL_max(SDL_SIMDAlignment, 16);
        }
    }
    return SDL_CPUFeatures;
}

#define CPU_FEATURE_AVAILABLE(f) ((SDL_GetCPUFeatures() & f) ? SDL_TRUE : SDL_FALSE)

SDL_bool SDL_HasRDTSC(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_RDTSC);
}

SDL_bool
SDL_HasAltiVec(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_ALTIVEC);
}

SDL_bool
SDL_HasMMX(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_MMX);
}

SDL_bool
SDL_Has3DNow(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_3DNOW);
}

SDL_bool
SDL_HasSSE(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_SSE);
}

SDL_bool
SDL_HasSSE2(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_SSE2);
}

SDL_bool
SDL_HasSSE3(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_SSE3);
}

SDL_bool
SDL_HasSSE41(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_SSE41);
}

SDL_bool
SDL_HasSSE42(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_SSE42);
}

SDL_bool
SDL_HasAVX(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_AVX);
}

SDL_bool
SDL_HasAVX2(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_AVX2);
}

SDL_bool
SDL_HasAVX512F(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_AVX512F);
}

SDL_bool
SDL_HasNEON(void)
{
    return CPU_FEATURE_AVAILABLE(CPU_HAS_NEON);
}

static int SDL_SystemRAM = 0;

int
SDL_GetSystemRAM(void)
{
    if (!SDL_SystemRAM) {
#ifndef SDL_CPUINFO_DISABLED
#if defined(HAVE_SYSCONF) && defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
        if (SDL_SystemRAM <= 0) {
            SDL_SystemRAM = (int)((Sint64)sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE) / (1024*1024));
        }
#endif
#ifdef HAVE_SYSCTLBYNAME
        if (SDL_SystemRAM <= 0) {
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
#ifdef HW_REALMEM
            int mib[2] = {CTL_HW, HW_REALMEM};
#else
            /* might only report up to 2 GiB */
            int mib[2] = {CTL_HW, HW_PHYSMEM};
#endif /* HW_REALMEM */
#else
            int mib[2] = {CTL_HW, HW_MEMSIZE};
#endif /* __FreeBSD__ || __FreeBSD_kernel__ */
            Uint64 memsize = 0;
            size_t len = sizeof(memsize);
            
            if (sysctl(mib, 2, &memsize, &len, NULL, 0) == 0) {
                SDL_SystemRAM = (int)(memsize / (1024*1024));
            }
        }
#endif
#ifdef __WIN32__
        if (SDL_SystemRAM <= 0) {
            MEMORYSTATUSEX stat;
            stat.dwLength = sizeof(stat);
            if (GlobalMemoryStatusEx(&stat)) {
                SDL_SystemRAM = (int)(stat.ullTotalPhys / (1024 * 1024));
            }
        }
#endif
#ifdef __OS2__
        if (SDL_SystemRAM <= 0) {
            Uint32 sysram = 0;
            DosQuerySysInfo(QSV_TOTPHYSMEM, QSV_TOTPHYSMEM, &sysram, 4);
            SDL_SystemRAM = (int) (sysram / 0x100000U);
        }
#endif
#endif
    }
    return SDL_SystemRAM;
}


// Dry - bug fix - check if SIMD is supported
#ifdef __EMSCRIPTEN__
size_t
SDL_SIMDGetAlignment(void)
{
    SDL_assert(0 && "We should never get here if the platform does not support SIMD");
    return 4;
}
#else
size_t
SDL_SIMDGetAlignment(void)
{
    if (SDL_SIMDAlignment == 0xFFFFFFFF) {
        SDL_GetCPUFeatures();  /* make sure this has been calculated */
    }
    SDL_assert(SDL_SIMDAlignment != 0);
    return SDL_SIMDAlignment;
}

void *
SDL_SIMDAlloc(const size_t len)
{
    const size_t alignment = SDL_SIMDGetAlignment();
    const size_t padding = alignment - (len % alignment);
    const size_t padded = (padding != alignment) ? (len + padding) : len;
    Uint8 *retval = NULL;
    Uint8 *ptr = (Uint8 *) SDL_malloc(padded + alignment + sizeof (void *));
    if (ptr) {
        /* store the actual malloc pointer right before our aligned pointer. */
        retval = ptr + sizeof (void *);
        retval += alignment - (((size_t) retval) % alignment);
        *(((void **) retval) - 1) = ptr;
    }
    return retval;
}

void
SDL_SIMDFree(void *ptr)
{
    if (ptr) {
        void **realptr = (void **) ptr;
        realptr--;
        SDL_free(*(((void **) ptr) - 1));
    }
}
#endif


#ifdef TEST_MAIN

#include <stdio.h>

int
main()
{
    printf("CPU count: %d\n", SDL_GetCPUCount());
    printf("CPU type: %s\n", SDL_GetCPUType());
    printf("CPU name: %s\n", SDL_GetCPUName());
    printf("CacheLine size: %d\n", SDL_GetCPUCacheLineSize());
    printf("RDTSC: %d\n", SDL_HasRDTSC());
    printf("Altivec: %d\n", SDL_HasAltiVec());
    printf("MMX: %d\n", SDL_HasMMX());
    printf("3DNow: %d\n", SDL_Has3DNow());
    printf("SSE: %d\n", SDL_HasSSE());
    printf("SSE2: %d\n", SDL_HasSSE2());
    printf("SSE3: %d\n", SDL_HasSSE3());
    printf("SSE4.1: %d\n", SDL_HasSSE41());
    printf("SSE4.2: %d\n", SDL_HasSSE42());
    printf("AVX: %d\n", SDL_HasAVX());
    printf("AVX2: %d\n", SDL_HasAVX2());
    printf("AVX-512F: %d\n", SDL_HasAVX512F());
    printf("NEON: %d\n", SDL_HasNEON());
    printf("RAM: %d MB\n", SDL_GetSystemRAM());
    return 0;
}

#endif /* TEST_MAIN */

/* vi: set ts=4 sw=4 expandtab: */

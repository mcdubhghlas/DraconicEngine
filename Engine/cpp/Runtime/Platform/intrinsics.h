// platform/intrinsics.h

#pragma once

#include "Runtime/Platform/simd.h"

#if ARCH_X64
    #include <immintrin.h>
#elif ARCH_ARM64
    #include <arm_neon.h>
#endif

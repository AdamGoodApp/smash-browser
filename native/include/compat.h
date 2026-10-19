#ifndef MELEE_NATIVE_COMPAT_H
#define MELEE_NATIVE_COMPAT_H
#ifdef __linux__
#define __assert MeleeNativeAssert
#endif
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define __fabs fabs
#define __fabsf fabsf
#define sqrtf_accurate sqrtf
#define sqrtf__Ff sqrtf
static inline double melee_native_frsqrte(double x) { return 1.0 / sqrt(x); }
#define __frsqrte melee_native_frsqrte

/* Paired-single SDK entry points become portable scalar math routines. */
#define PSMTXIdentity C_MTXIdentity
#define PSMTXCopy C_MTXCopy
#define PSMTXConcat C_MTXConcat
#define PSMTXInverse C_MTXInverse
#define PSMTXTranspose C_MTXTranspose
#define PSMTXMultVec C_MTXMultVec
#define PSMTXMultVecSR C_MTXMultVecSR
#define PSMTXQuat C_MTXQuat
#define PSMTXRotAxisRad C_MTXRotAxisRad
#define PSMTXScale C_MTXScale
#define PSMTXTrans C_MTXTrans
#define PSVECAdd C_VECAdd
#define PSVECSubtract C_VECSubtract
#define PSVECScale C_VECScale
#define PSVECNormalize C_VECNormalize
#define PSVECMag C_VECMag
#define PSVECCrossProduct C_VECCrossProduct
#define PSVECDotProduct C_VECDotProduct
#define MTXFrustum C_MTXFrustum
#define MTXOrtho C_MTXOrtho
#define MTXPerspective C_MTXPerspective
#define MTXRotRad C_MTXRotRad
#define MTXLightFrustum C_MTXLightFrustum
#define MTXLightOrtho C_MTXLightOrtho
#define MTXLightPerspective C_MTXLightPerspective

/* The game tells ARAM offsets from main-memory addresses by value: GameCube MEM1
   is mapped at 0x80000000 and ARAM offsets stay below its 16 MiB size. 64-bit
   hosts place every host address far above 0x80000000; wasm32 addresses are all
   below it, so the browser build links with GLOBAL_BASE at 16 MiB and uses the
   ARAM size as the boundary instead. */
#ifdef __EMSCRIPTEN__
#define MELEE_MAIN_MEMORY_BASE 0x01000000U
#else
#define MELEE_MAIN_MEMORY_BASE 0x80000000U
#endif

#endif

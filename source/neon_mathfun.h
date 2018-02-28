#ifndef _NEON_MATHFUN_H_
#define _NEON_MATHFUN_H_

#include <arm_neon.h>

typedef float32x4_t v4sf;  // vector of 4 float
typedef uint32x4_t v4su;  // vector of 4 uint32
typedef int32x4_t v4si;  // vector of 4 uint32

v4sf log_ps(v4sf x);
v4sf exp_ps(v4sf x);
void sincos_ps(v4sf x, v4sf *ysin, v4sf *ycos);
v4sf sin_ps(v4sf x);
v4sf cos_ps(v4sf x);

#endif

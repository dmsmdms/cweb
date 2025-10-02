#include <calc/calc-math.h>
#include <xmmintrin.h>

double calc_sqrt(double x)
{
    __m128d v = _mm_set_sd(x);
    v = _mm_sqrt_sd(v, v);
    return _mm_cvtsd_f64(v);
}

float calc_sqrtf(float x)
{
    __m128 v = _mm_set_ss(x);
    v = _mm_sqrt_ss(v);
    return _mm_cvtss_f32(v);
}

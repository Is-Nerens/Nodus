
#pragma once

#if defined(__AVX512F__)
    #include <immintrin.h>
    #define NU_SIMD_AVX512
    #define NU_SIMD_WIDTH 16
    typedef __m512 float32xN;

#elif defined(__AVX__)
    #include <immintrin.h>
    #define NU_SIMD_AVX
    #define NU_SIMD_WIDTH 8
    typedef __m256 float32xN;

#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    #include <immintrin.h>
    #define NU_SIMD_SSE2
    #define NU_SIMD_WIDTH 4
    typedef __m128 float32xN;

#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
    #include <arm_neon.h>
    #define NU_SIMD_NEON
    #define NU_SIMD_WIDTH 4
    typedef float32x4_t float32xN;

#else
    #define NU_SIMD_SCALAR
    #define NU_SIMD_WIDTH 4
    typedef struct {
        float32 v[4];
    } float32xN;
#endif


// Create
static inline float32xN NU_SIMD_Set(float32 x, float32 y, float32 z, float32 w)
{
#ifdef NU_SIMD_AVX512
    return _mm512_setr_ps(x,y,z,w,x,y,z,w,x,y,z,w,x,y,z,w);
#elif defined(NU_SIMD_AVX)
    return _mm256_setr_ps(x,y,z,w,x,y,z,w);
#elif defined(NU_SIMD_SSE2)
    return _mm_setr_ps(x,y,z,w);
#elif defined(NU_SIMD_NEON)
    float32 values[4] = {x,y,z,w};
    return vld1q_f32(values);
#else
    return (float32xN){{x,y,z,w}};
#endif
}


// Arithmetic
static inline float32xN NU_SIMD_Add(float32xN a, float32xN b)
{
#ifdef NU_SIMD_AVX512
    return _mm512_add_ps(a,b);
#elif defined(NU_SIMD_AVX)
    return _mm256_add_ps(a,b);
#elif defined(NU_SIMD_SSE2)
    return _mm_add_ps(a,b);
#elif defined(NU_SIMD_NEON)
    return vaddq_f32(a,b);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = a.v[i]+b.v[i];
    return r;
#endif
}

static inline float32xN NU_SIMD_Sub(float32xN a, float32xN b)
{
#ifdef NU_SIMD_AVX512
    return _mm512_sub_ps(a,b);
#elif defined(NU_SIMD_AVX)
    return _mm256_sub_ps(a,b);
#elif defined(NU_SIMD_SSE2)
    return _mm_sub_ps(a,b);
#elif defined(NU_SIMD_NEON)
    return vsubq_f32(a,b);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = a.v[i]-b.v[i];
    return r;
#endif
}

static inline float32xN NU_SIMD_Mul(float32xN a, float32xN b)
{
#ifdef NU_SIMD_AVX512
    return _mm512_mul_ps(a,b);
#elif defined(NU_SIMD_AVX)
    return _mm256_mul_ps(a,b);
#elif defined(NU_SIMD_SSE2)
    return _mm_mul_ps(a,b);
#elif defined(NU_SIMD_NEON)
    return vmulq_f32(a,b);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = a.v[i]*b.v[i];
    return r;
#endif
}

static inline float32xN NU_SIMD_Div(float32xN a, float32xN b)
{
#ifdef NU_SIMD_AVX512
    return _mm512_div_ps(a,b);
#elif defined(NU_SIMD_AVX)
    return _mm256_div_ps(a,b);
#elif defined(NU_SIMD_SSE2)
    return _mm_div_ps(a,b);
#elif defined(NU_SIMD_NEON)
    return vdivq_f32(a,b);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = a.v[i]/b.v[i];
    return r;
#endif
}


// Scalar arithmetic
static inline float32xN NU_SIMD_AddScalar(float32xN a, float32 b)
{
#ifdef NU_SIMD_AVX512
    return _mm512_add_ps(a,_mm512_set1_ps(b));
#elif defined(NU_SIMD_AVX)
    return _mm256_add_ps(a,_mm256_set1_ps(b));
#elif defined(NU_SIMD_SSE2)
    return _mm_add_ps(a,_mm_set1_ps(b));
#elif defined(NU_SIMD_NEON)
    return vaddq_f32(a,vdupq_n_f32(b));
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = a.v[i]+b;
    return r;
#endif
}

static inline float32xN NU_SIMD_MulScalar(float32xN a, float32 b)
{
#ifdef NU_SIMD_AVX512
    return _mm512_mul_ps(a,_mm512_set1_ps(b));
#elif defined(NU_SIMD_AVX)
    return _mm256_mul_ps(a,_mm256_set1_ps(b));
#elif defined(NU_SIMD_SSE2)
    return _mm_mul_ps(a,_mm_set1_ps(b));
#elif defined(NU_SIMD_NEON)
    return vmulq_n_f32(a,b);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = a.v[i]*b;
    return r;
#endif
}


// Min / Max
static inline float32xN NU_SIMD_Min(float32xN a, float32xN b)
{
#ifdef NU_SIMD_AVX512
    return _mm512_min_ps(a,b);
#elif defined(NU_SIMD_AVX)
    return _mm256_min_ps(a,b);
#elif defined(NU_SIMD_SSE2)
    return _mm_min_ps(a,b);
#elif defined(NU_SIMD_NEON)
    return vminq_f32(a,b);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = a.v[i]<b.v[i]?a.v[i]:b.v[i];
    return r;
#endif
}

static inline float32xN NU_SIMD_Max(float32xN a, float32xN b)
{
#ifdef NU_SIMD_AVX512
    return _mm512_max_ps(a,b);
#elif defined(NU_SIMD_AVX)
    return _mm256_max_ps(a,b);
#elif defined(NU_SIMD_SSE2)
    return _mm_max_ps(a,b);
#elif defined(NU_SIMD_NEON)
    return vmaxq_f32(a,b);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = a.v[i]>b.v[i]?a.v[i]:b.v[i];
    return r;
#endif
}


// Square root
static inline float32xN NU_SIMD_Sqrt(float32xN a)
{
#ifdef NU_SIMD_AVX512
    return _mm512_sqrt_ps(a);
#elif defined(NU_SIMD_AVX)
    return _mm256_sqrt_ps(a);
#elif defined(NU_SIMD_SSE2)
    return _mm_sqrt_ps(a);
#elif defined(NU_SIMD_NEON)
    return vsqrtq_f32(a);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = __builtin_sqrtf(a.v[i]);
    return r;
#endif
}


// Absolute value
static inline float32xN NU_SIMD_Abs(float32xN a)
{
#ifdef NU_SIMD_AVX512
    return _mm512_castsi512_ps(_mm512_andnot_si512(
        _mm512_set1_epi32(0x80000000),
        _mm512_castps_si512(a)));
#elif defined(NU_SIMD_AVX)
    return _mm256_andnot_ps(_mm256_set1_ps(-0.0f),a);
#elif defined(NU_SIMD_SSE2)
    return _mm_andnot_ps(_mm_set1_ps(-0.0f),a);
#elif defined(NU_SIMD_NEON)
    return vabsq_f32(a);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = a.v[i]<0?-a.v[i]:a.v[i];
    return r;
#endif
}


// Multiply-add
static inline float32xN NU_SIMD_MulAdd(float32xN a, float32xN b, float32xN c)
{
#ifdef NU_SIMD_AVX512
    return _mm512_fmadd_ps(a,b,c);
#elif defined(NU_SIMD_AVX)
    return _mm256_add_ps(_mm256_mul_ps(a,b),c);
#elif defined(NU_SIMD_SSE2)
    return _mm_add_ps(_mm_mul_ps(a,b),c);
#elif defined(NU_SIMD_NEON)
    return vfmaq_f32(c,a,b);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = a.v[i]*b.v[i]+c.v[i];
    return r;
#endif
}


// Memory
static inline float32xN NU_SIMD_Load(const float32* p)
{
#ifdef NU_SIMD_AVX512
    return _mm512_loadu_ps(p);
#elif defined(NU_SIMD_AVX)
    return _mm256_loadu_ps(p);
#elif defined(NU_SIMD_SSE2)
    return _mm_loadu_ps(p);
#elif defined(NU_SIMD_NEON)
    return vld1q_f32(p);
#else
    float32xN r;
    for (int i=0; i<4; i++) r.v[i] = p[i];
    return r;
#endif
}

static inline void NU_SIMD_Store(float32* p, float32xN a)
{
#ifdef NU_SIMD_AVX512
    _mm512_storeu_ps(p,a);
#elif defined(NU_SIMD_AVX)
    _mm256_storeu_ps(p,a);
#elif defined(NU_SIMD_SSE2)
    _mm_storeu_ps(p,a);
#elif defined(NU_SIMD_NEON)
    vst1q_f32(p,a);
#else
    for (int i=0; i<4; i++) p[i] = a.v[i];
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                           BASICS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <math.h>      // Basic Math
#include <float.h>     // Some FP
#include <xmmintrin.h> // SSE
#include <emmintrin.h> // SSE 2
#include <pmmintrin.h> // SSE 3
#include <smmintrin.h> // SSE 4.1

#define EPSILON     0.0001f
#define EPSILONBIG  0.1f
#define PI_MULT_2   6.283185307f
#define ISZERO(a)   (((a) > -EPSILON) & ((a) < EPSILON))
#define MAX(a,b)    ((a) >= (b) ? (a) : (b))
#define MIN(a,b)    ((a) <= (b) ? (a) : (b))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                           VECTOR TYPE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(SSE2) || defined(SSE4)
// The basic 4D vector used for vector calculations.
// Must be aligned to 16 bytes and allocated on the heap by using _aligned_alloc instead of malloc
// Union provides easy access for different use-cases, including SIMD instructions by 'SSE'
typedef union __declspec(intrin_type) __declspec(align(16)) V4
{
   float DATA[4];
   __m128 SSE;
   struct { float X; float Y; float Z; float W; };
   struct { float R; float G; float B; float A; };
} V4;

typedef V4 V3;   // V3 is just a V4
typedef V4 V2;   // V2 is just a V4
#else
typedef struct V4 { float X, Y, Z, W; } V4;
typedef struct V3 { float X, Y, Z; } V3;
typedef struct V2 { float X, Y; } V2;
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                         SSE VECTOR OPERATIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(SSE2) || defined(SSE4)
#define V3ADD(a,b,c)    (a)->SSE = _mm_add_ps((b)->SSE, (c)->SSE);         // a = b + c
#define V3SUB(a,b,c)    (a)->SSE = _mm_sub_ps((b)->SSE, (c)->SSE);         // a = b - c
#define V3SCALE(a,b)    (a)->SSE = _mm_mul_ps((a)->SSE, _mm_set1_ps(b));   // a = a * b  (b is float)
#define V3CROSS(a,b,c)  (a)->SSE = _mm_sub_ps(                                                                                            \
   _mm_mul_ps(_mm_shuffle_ps((b)->SSE, (b)->SSE, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps((c)->SSE, (c)->SSE, _MM_SHUFFLE(3, 1, 0, 2))),   \
   _mm_mul_ps(_mm_shuffle_ps((b)->SSE, (b)->SSE, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps((c)->SSE, (c)->SSE, _MM_SHUFFLE(3, 0, 2, 1))));

#define V3SCALE3(a,b)   (a)->SSE = _mm_mul_ps((a)->SSE, (b)->SSE);
#define V3SQRT3(a,b)    (a)->SSE = _mm_sqrt_ps((b)->SSE);

__forceinline int V3ISZERO(const V3* a)
{
   const __m128  eps_posi = _mm_set1_ps(EPSILON);             //  e
   const __m128  eps_nega = _mm_set1_ps(-EPSILON);            // -e
   const __m128  lth_mask = _mm_cmplt_ps(a->SSE, eps_posi);   // r1 = a <  e
   const __m128  gth_mask = _mm_cmpgt_ps(a->SSE, eps_nega);   // r2 = a > -e
   const __m128  and_mask = _mm_and_ps(gth_mask, lth_mask);   // r3 = r1 & r2
   const __m128i cast     = _mm_castps_si128(and_mask);       // cast from float to int (no-op)
   const int     mask     = _mm_movemask_epi8(cast);          // combine some bits from all registers
   return mask & 0x0FFF;                                      // filter for 3D
}

#if defined(SSE4)
#define V3DOT3(a,b,c)   (a)->SSE = _mm_dp_ps((b)->SSE, (c)->SSE, 127);
#define V3DOT(a,b)      _mm_dp_ps((a)->SSE, (b)->SSE, 127).m128_f32[0]
#define V3LEN(a)        _mm_sqrt_ps(_mm_dp_ps((a)->SSE, (a)->SSE, 127)).m128_f32[0]
#define V3ROUND(a)      (a)->SSE = _mm_round_ps((a)->SSE, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
#else
__forceinline void V3DOT3(V3* a, const V3* b, const V3* c)
{
   const __m128  mult = _mm_mul_ps(b->SSE, c->SSE);
   const __m128i mski = _mm_set_epi32(0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
   const __m128  mskf = _mm_castsi128_ps(mski);
   const __m128  and  = _mm_and_ps(mult, mskf);
   const __m128  add1 = _mm_add_ps(and, _mm_shuffle_ps(and, and, _MM_SHUFFLE(2, 3, 0, 1)));
   const __m128  add2 = _mm_add_ps(add1, _mm_shuffle_ps(add1, add1, _MM_SHUFFLE(0, 1, 2, 3)));
   a->SSE = add2;
}

__forceinline float V3DOT(const V3* a, const V3* b)
{
   V3 v0;
   V3DOT3(&v0, a, b);
   return v0.X;
}

__forceinline float V3LEN(const V3* a)
{
   V3 v0;
   V3DOT3(&v0, a, a);
   V3SQRT3(&v0, &v0);
   return v0.X;
}

#define V3ROUND(a)          \
   (a)->X = roundf((a)->X); \
   (a)->Y = roundf((a)->Y); \
   (a)->Z = roundf((a)->Z);
#endif

#define V3LEN2(a)    V3DOT(a,a)

///////////////////////////////////////////////////////////////////////////

#define V2ADD(a,b,c)  V3ADD(a,b,c)
#define V2SUB(a,b,c)  V3SUB(a,b,c)
#define V2SCALE(a,b)  V3SCALE(a,b)

#define V2DOT(a,b)    ((a)->X * (b)->X + (a)->Y * (b)->Y)
#define V2LEN2(a)     V2DOT(a,a)
#define V2LEN(a)      _mm_sqrt_ss(_mm_set1_ps(V2LEN2((a)))).m128_f32[0]
#define V2ISZERO(a)   (ISZERO((a)->X) && ISZERO((a)->Y))

#if defined(SSE4)
#define V2ROUND(a)    V3ROUND(a)
#else
#define V2ROUND(a)          \
   (a)->X = roundf((a)->X); \
   (a)->Y = roundf((a)->Y);
#endif

// true if point (c) lies inside boundingbox defined by min/max of (a) and (b)
__forceinline bool ISINBOX(const V2* a, const V2* b, const V2* c)
{
   const __m128  epsi = _mm_set1_ps(EPSILONBIG);      // e
   const __m128  mini = _mm_min_ps(a->SSE, b->SSE);   // min(a,b)
   const __m128  maxi = _mm_max_ps(a->SSE, b->SSE);   // max(a,b)
   const __m128  left = _mm_sub_ps(mini, epsi);       // min - e
   const __m128  rigt = _mm_add_ps(maxi, epsi);       // max + e
   const __m128  geq  = _mm_cmpge_ps(c->SSE, left);   // c >= left
   const __m128  leq  = _mm_cmple_ps(c->SSE, rigt);   // c <= right
   const __m128  comb = _mm_and_ps(geq, leq);         // >= left & <= right
   const __m128i cast = _mm_castps_si128(comb);       // cast from float to int (no-op) [SSE2]
   const int     mask = _mm_movemask_epi8(cast);      // combine some bits from all registers [SSE2]
   return (mask & 0x00FF) == 0x00FF;                  // filter for 2D
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                        NO-SSE VECTOR OPERATIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if !(defined(SSE2) || defined(SSE4))
#define V3ADD(a,b,c) \
   (a)->X = (b)->X + (c)->X; \
   (a)->Y = (b)->Y + (c)->Y; \
   (a)->Z = (b)->Z + (c)->Z;

#define V3SUB(a,b,c) \
   (a)->X = (b)->X - (c)->X; \
   (a)->Y = (b)->Y - (c)->Y; \
   (a)->Z = (b)->Z - (c)->Z;

#define V3SCALE(a,b) \
   (a)->X *= (b); \
   (a)->Y *= (b); \
   (a)->Z *= (b);

#define V3ISZERO(a) (ISZERO((a)->X) & ISZERO((a)->Y) & ISZERO((a)->Z)) 

#define V3CROSS(a,b,c) \
   (a)->X = (b)->Y * (c)->Z - (b)->Z * (c)->Y; \
   (a)->Y = (b)->Z * (c)->X - (b)->X * (c)->Z; \
   (a)->Z = (b)->X * (c)->Y - (b)->Y * (c)->X;

#define V3DOT(a,b) ((a)->X * (b)->X + (a)->Y * (b)->Y + (a)->Z * (b)->Z)
#define V3LEN2(a)  V3DOT(a,a)
#define V3LEN(a)   sqrtf(V3LEN2((a)))

#define V3ROUND(a)          \
   (a)->X = roundf((a)->X); \
   (a)->Y = roundf((a)->Y); \
   (a)->Z = roundf((a)->Z);

///////////////////////////////////////////////////////////////////////////

#define V2ADD(a,b,c) \
   (a)->X = (b)->X + (c)->X; \
   (a)->Y = (b)->Y + (c)->Y;

#define V2SUB(a,b,c) \
   (a)->X = (b)->X - (c)->X; \
   (a)->Y = (b)->Y - (c)->Y;

#define V2SCALE(a,b) \
   (a)->X *= (b); \
   (a)->Y *= (b);

#define V2DOT(a,b)    ((a)->X * (b)->X + (a)->Y * (b)->Y)
#define V2LEN2(a)     V2DOT(a,a)
#define V2LEN(a)      sqrtf(V2LEN2((a)))
#define V2ISZERO(a)   (ISZERO((a)->X) && ISZERO((a)->Y))

#define V2ROUND(a)          \
   (a)->X = roundf((a)->X); \
   (a)->Y = roundf((a)->Y);

// true if point (c) lies inside boundingbox defined by min/max of (a) and (b)
#define ISINBOX(a, b, c) \
   (MIN((a)->X, (b)->X) - EPSILONBIG <= (c)->X && (c)->X <= MAX((a)->X, (b)->X) + EPSILONBIG && \
    MIN((a)->Y, (b)->Y) - EPSILONBIG <= (c)->Y && (c)->Y <= MAX((a)->Y, (b)->Y) + EPSILONBIG)

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                           OTHER ALGORITHMS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

__forceinline void V3NORMALIZE(V3* a)
{
   const float len = V3LEN(a);

   if (ISZERO(len)) 
      return;

   V3SCALE(a, 1.0f / len);
}

__forceinline void V2NORMALIZE(V2* a)
{
   const float len = V2LEN(a);

   if (ISZERO(len))
      return;

   V2SCALE(a, 1.0f / len);
}

// Rotates V2 instance by radian
__forceinline void V2ROTATE(V2* V, const float Radian)
{
   const float cs = cosf(Radian);
   const float sn = sinf(Radian);
   const float px = V->X;
   const float py = V->Y;
   V->X = px * cs - py * sn;
   V->Y = px * sn + py * cs;
}

// Möller–Trumbore intersection algorithm
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
// Returns true if there is an intersection
__forceinline bool IntersectLineTriangle(const V3* P1, const V3* P2, const V3* P3, const V3* S, const V3* E)
{
   V3 e1, e2, p, q, t, d;
   float u, v, f, inv_det, det;
   
   // vectors
   V3SUB(&d, E, S);      // line vector    S->E
   V3SUB(&e1, P2, P1);   // triangle edge P1->P2
   V3SUB(&e2, P3, P1);   // triangle edge P1->P3
   V3CROSS(&p, &d, &e2); // used to calculate determinant and u,v parms

   // if determinant is near zero, ray lies in plane of triangle
   det = V3DOT(&e1, &p);
   if (ISZERO(det))
      return false;

   inv_det = 1.0f / det;

   // calculate distance from P1 to line start
   V3SUB(&t, S, P1);

   // calculate u parameter and test bound
   u = V3DOT(&t, &p) * inv_det;
   if ((u < 0.0f) | (u > 1.0f))
      return false;

   // prepare to test v parameter
   V3CROSS(&q, &t, &e1);

   // calculate v parameter and test bound
   v = V3DOT(&d, &q) * inv_det;
   if ((v < 0.0f) | (u + v > 1.0f))
      return false;

   f = V3DOT(&e2, &q) * inv_det;

   // note: we additionally check for < 1.0f
   // = LINE intersection, not ray
   if ((f >= 0.0f) & (f <= 1.0f))
      return true;

   return false;
}

// Returns the minimum squared distance between
// point P and finite line segment given by Q1 and Q2
__forceinline float MinSquaredDistanceToLineSegment(const V2* P, const V2* Q1, const V2* Q2)
{
   V2 v1, v2, v3;

   // vectors
   V2SUB(&v1, P, Q1);   // from q1 to p
   V2SUB(&v3, Q2, Q1);  // line vector

   // squared distance between Q1 and Q2
   const float len2 = V2LEN2(&v3);

   // Q1 is on Q2 (no line at all)
   // use squared distance to Q1
   if (ISZERO(len2))
      return V2LEN2(&v1);
    
   const float t = V2DOT(&v1, &v3) / len2;

   // Q1 is closest
   if (t < 0.0f) 
      return V2LEN2(&v1);

   // Q2 is closest
   else if (t > 1.0f)
   {
      V2SUB(&v2, P, Q2);   // from q2 to p
      return V2LEN2(&v2);
   }

   // point on line is closest
   else
   {
      V2SCALE(&v3, t);
      V2ADD(&v3, Q1, &v3);
      V2SUB(&v3, &v3, P);
      return V2LEN2(&v3);
   }
}

// Generates random coordinates on point P which are guaranteed
// to be inside the triangle defined by A, B, C
__forceinline void RandomPointInTriangle(V2* P, const V2* A, const V2* B, const V2* C)
{
   // create two randoms in [0.0f , 1.0f]
   const float rnd1 = (float)rand() * (1.0f / (float)RAND_MAX);
   const float rnd2 = (float)rand() * (1.0f / (float)RAND_MAX);

   // get rootsqrt
   const float sqrt_rnd1 = sqrtf(rnd1);

   // coefficients
   const float coeff1 = 1.0f - sqrt_rnd1;
   const float coeff2 = sqrt_rnd1 * (1.0f - rnd2);
   const float coeff3 = rnd2 * sqrt_rnd1;

   // generate random coordinates on P
   P->X = coeff1 * A->X + coeff2 * B->X + coeff3 * C->X;
   P->Y = coeff1 * A->Y + coeff2 * B->Y + coeff3 * C->Y;
}

// Checks for intersection of a finite line segment (S, E) and a circle (M=center)
// http://stackoverflow.com/questions/1073336/circle-line-collision-detection
// Returns true if there is an intersection
__forceinline bool IntersectLineCircle(const V2* M, const float Radius, const V2* S, const V2* E)
{
   V2 d, f;
   V2SUB(&d, E, S);
   V2SUB(&f, S, M);

   const float a = V2DOT(&d, &d);
   const float b = 2.0f * V2DOT(&f, &d);
   const float c = V2DOT(&f, &f) - (Radius * Radius);
   const float div = 2.0f * a;
   const float discriminant = b * b - 4.0f * a * c;

   if (discriminant < 0.0f || ISZERO(div))
      return false;

#if !(defined(SSE2) || defined(SSE4))
   const float sqrt = sqrtf(discriminant);
   const float t1 = (-b - sqrt) / div;
   const float t2 = (-b + sqrt) / div;

   return
      (t1 >= 0.0f && t1 <= 1.0f) ||
      (t2 >= 0.0f && t2 <= 1.0f);
#else
   const __m128  sqrt = _mm_sqrt_ps(_mm_set1_ps(discriminant));  // sq. root
   const __m128  nmsk = _mm_set_ps(-0.0f, 0.0f, 0.0f, 0.0f);     // load negate mask
   const __m128  xor  = _mm_xor_ps(sqrt, nmsk);                  // xor to flip X of sqrt to -
   const __m128  add  = _mm_add_ps(_mm_set1_ps(-b), xor);        // -b + prepared sqrt
   const __m128  divi = _mm_div_ps(add, _mm_set1_ps(div));       // / div
   const __m128  cge  = _mm_cmpge_ps(divi, _mm_set1_ps(0.0f));   // >= 0
   const __m128  cle  = _mm_cmple_ps(divi, _mm_set1_ps(1.0f));   // <= 0
   const __m128  comb = _mm_and_ps(cge, cle);                    // >= 0 && <= 0
   const __m128i cast = _mm_castps_si128(comb);                  // cast from float to int (no-op) [SSE2]
   const int     mask = _mm_movemask_epi8(cast);                 // combine some bits from all registers [SSE2]
   return (mask & 0x00FF) == 0x00FF;                             // filter for 2D
#endif
}

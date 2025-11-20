#define HAS_AVX512 0	/* AVX-512 is a bit more efficient in some places (such as xor3) */

#include <stdint.h>
#include <stdlib.h>
#include <memory.h>

// ------------------------------------
// Win64
#ifdef _MSC_VER
#include <intrin.h>
#define _GCC_U64_TYPE uint64_t

// returns the width of a 64-bit number
inline int get_bitwidth64(uint64_t x)
{
	if (!x) return 0;
	unsigned long u;
	_BitScanReverse64(&u, x);
	return u + 1;
}

#else
// ------------------------------------
// GCC. was tested with:
// g++ fpelib.cpp -o fpelib.a -Ofast -march=native -frename-registers -w
#include <x86intrin.h>
#define _GCC_U64_TYPE long long unsigned int
#define _mm_setr_epi64x(a,b) _mm_set_epi64x((__int64_t)(b), (__int64_t)(a))
#define __popcnt64(x) __builtin_popcountll(x)

inline int get_bitwidth64(uint64_t x)
{
	return 64 - __builtin_clzll(x);
}

//inline void _BitScanReverse64(unsigned long* index, unsigned long long x)
//{	
	//*index = 63 - __builtin_clzll(x);
//}

inline uint64_t _udiv128(uint64_t x_hi, uint64_t x_lo, uint64_t y, uint64_t* r)
{
	// Source: https://github.com/michaeljclark/c128/blob/trunk/include/integer.h
	uint64_t q;
	__asm__("divq %[v]" : "=a"(q), "=d"(*r) : [v] "r"(y), "a"(x_lo), "d"(x_hi));
	return q;
}
// ------------------------------------
#endif





#if HAS_AVX512==1
#define xor3(a, b, c) _mm_ternarylogic_epi64(a, b, c, 0x96) /* (a+b+c) */
#else
#define xor3(a, b, c) xor2(xor2(a,b),c)
#endif

#define c_00 _mm_setzero_si128()
#define c_ff _mm_set1_epi8((uint8_t)0xff)
#define load128(ptr) _mm_loadu_si128((__m128i*)(ptr))
#define store128(ptr, x) _mm_storeu_si128((__m128i*)(ptr), x)
#define xor2(a, b) _mm_xor_si128(a, b)
#define aesenc(x, rk) _mm_aesenc_si128(x, rk);
#define aesdec(x, rk) _mm_aesdec_si128(x, rk);
#define shuffle(x, sh) _mm_shuffle_epi8(x, sh)
#define combine(s1, s2) _mm_or_si128(_mm_shuffle_epi8(s1, s2), _mm_cmpeq_epi8(s2, c_ff))



// -----------------------------------------------------------
// Additional functions for AVX2
// -----------------------------------------------------------

// Less or equal for unsigned integers
inline __m256i _mm256_cmple_epu64(__m256i A, __m256i B)
{
	return _mm256_cmpeq_epi64(_mm256_max_epu64(A, B), B);
}

inline __m256i _mm256_cmple_epu32(__m256i A, __m256i B)
{
	return _mm256_cmpeq_epi32(_mm256_max_epu32(A, B), B);
}

inline __m256i _mm256_cmple_epu16(__m256i A, __m256i B)
{
	return _mm256_cmpeq_epi16(_mm256_max_epu16(A, B), B);
}

inline __m256i _mm256_cmple_epu8(__m256i A, __m256i B)
{
	return _mm256_cmpeq_epi8(_mm256_max_epu8(A, B), B);
}

// Res = (A - B) % Q, assuming A < Q and B < Q; Q=0 means Q=2^16
inline __m256i _mm256_submod_epu16(__m256i A, __m256i B, __m256i Q)
{
	// let S=A-B (epu16) then if S > A then S += Q
	__m256i S = _mm256_sub_epi16(A, B);
	__m256i C = _mm256_cmple_epu16(S, A); // S<=A
	return _mm256_add_epi16(S, _mm256_andnot_si256(C, Q));
}

// Res = (A + B) % Q, assuming A < Q and B < Q; Q=0 means Q=2^16
inline __m256i _mm256_addmod_epu16(__m256i A, __m256i B, __m256i Q)
{
	// Can be replaced by (A - (Q - B)) % Q
	__m256i S = _mm256_add_epi16(B, _mm256_sub_epi16(A, Q));
	__m256i C = _mm256_cmple_epu16(S, A); // S<=A
	return _mm256_add_epi16(S, _mm256_andnot_si256(C, Q));
}

// -----------------------------------------------------------
// 192-bit unsigned bigInt
// -----------------------------------------------------------
struct __m192i
{
	uint64_t u[3];

	inline void addc(uint64_t v)
	{
		unsigned char carry;
		carry = _addcarryx_u64(0, u[0], v, (_GCC_U64_TYPE*)(u + 0));
		carry = _addcarryx_u64(carry, u[1], 0, (_GCC_U64_TYPE*)(u + 1));
		u[2] += carry;
	}

	inline void subc(uint64_t v)
	{
		unsigned char carry;
		carry = _addcarryx_u64(1, u[0], ~v, (_GCC_U64_TYPE*)(u + 0));
		carry = _addcarryx_u64(carry, u[1], -1LL, (_GCC_U64_TYPE*)(u + 1));
		u[2] += (uint64_t)-1LL;
		u[2] += carry;
	}

	inline void mulc(uint64_t v)
	{
		uint64_t hi1, hi2;
		u[0] = _mulx_u64(u[0], v, (_GCC_U64_TYPE*)(&hi1));
		u[1] = _mulx_u64(u[1], v, (_GCC_U64_TYPE*)(&hi2));
		u[2] = u[2] * v + hi2;
		u[2] += _addcarryx_u64(0, u[1], hi1, (_GCC_U64_TYPE*)(u + 1));
	}

	// divide this number by v and return the remainder
	inline uint64_t divremc(uint64_t v)
	{
		uint64_t rem = u[2] % v;
		u[2] /= v;
		u[1] = _udiv128(rem, u[1], v, &rem);
		u[0] = _udiv128(rem, u[0], v, &rem);
		return rem;
	}

	inline void set1(uint64_t v)
	{
		u[0] = v, u[1] = u[2] = 0;
	}

	inline void set_pwr2(int pwr)
	{
		u[0] = u[1] = u[2] = 0;
		u[pwr >> 6] = 1ULL << (pwr & 0x3f);
	}

	inline int popcnt(void)
	{
		return (int)__popcnt64(u[0]) + (int)__popcnt64(u[1]) + (int)__popcnt64(u[2]);
	}

	inline int bitwidth(void)
	{
		int res;
		if ((res = get_bitwidth64(u[2]))) return res + 128;
		if ((res = get_bitwidth64(u[1]))) return res + 64;
		return get_bitwidth64(u[0]);
	}

	/*
	inline int msb_bit(void)
	{
		unsigned long index = -1;
		if (u[2]) _BitScanReverse64(&index, u[2]), index += 128;
		else if (u[1]) _BitScanReverse64(&index, u[1]), index += 64;
		else if (u[0]) _BitScanReverse64(&index, u[0]);
		return (int)(long)index;
	}
	*/

	int is_zero(void)
	{
		return !(u[2] | u[1] | u[0]);
	}

	inline int cmpgt(__m192i &x)
	{
		if (u[2] > x.u[2]) return 1;
		if (u[2] < x.u[2]) return 0;
		if (u[1] > x.u[1]) return 1;
		if (u[1] < x.u[1]) return 0;
		if (u[0] > x.u[0]) return 1;
		return 0;
	}

	void random(int bits = 192, char set_hibit = 0)
	{
		if (bits < 0 || bits > 192) return;
		u[0] = u[1] = u[2] = 0;
		int n = bits >> 4;	// div 16
		int t = bits & 15;	// mod 16
		uint16_t* p = (uint16_t*)u;
		for (int i = 0; i < n; i++)
			p[i] = rand();
		if (t) p[n] = rand() & ((1 << t) - 1);
		if (set_hibit)
			if (t)
				p[n] |= 1 << (t - 1);
			else
				p[n - 1] |= 1 << 15;
	}
};

// -----------------------------------------------------------
// SMAC-3/4 Routines
// -----------------------------------------------------------
const __m128i smac_sigma = _mm_setr_epi8(7, 14, 15, 10, 12, 13, 3, 0, 4, 6, 1, 5, 8, 11, 2, 9);
const __m128i smac_const = _mm_setr_epi8(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

// SMAC(A1, A2, A3, M) -> (A1, A2, A3), requires temp register __m128i T
#define _smac_r0(A1, A2, A3, M)\
	T = _mm_xor_si128(_mm_xor_si128(A2, A3), M);\
	A3 = _mm_aesenc_si128(A2, M);\
	A2 = _mm_aesenc_si128(A1, M);\
	A1 = _mm_shuffle_epi8(T, smac_sigma)

// SMAC(A1, A2, A3, M) -> (A3, A1, A2), i.e., the new state is the same regs but rotated
#define _smac_r1(A1, A2, A3, M)\
		A3 = _mm_shuffle_epi8(_mm_xor_si128(_mm_xor_si128(A2, A3), M), smac_sigma);\
		A2 = _mm_aesenc_si128(A2, M); \
		A1 = _mm_aesenc_si128(A1, M)

// SMAC(A1, A2, A3, M) -> (B1, B2, B3), the new state is in a new set of regs B
#define _smac_r2(A, B, M)\
	B##1 = _mm_shuffle_epi8(_mm_xor_si128(_mm_xor_si128(A##2, A##3), M), smac_sigma);\
	B##3 = _mm_aesenc_si128(A##2, M);\
	B##2 = _mm_aesenc_si128(A##1, M)


#define _smac_initfinal1(A1, A2, A3)\
do{\
	__m128i T1 = A1, T2 = A2, T3 = A3;\
	for (int i = 0; i < 3; i++)\
	{\
		_smac_r1(A1, A2, A3, smac_const);\
		_smac_r1(A3, A1, A2, smac_const);\
		_smac_r1(A2, A3, A1, smac_const);\
	}\
	A1 = _mm_xor_si128(A1, T1);\
	A2 = _mm_xor_si128(A2, T2);\
	A3 = _mm_xor_si128(A3, T3);\
} while (0)

#define _smac_initfinal2(B1, B2, B3, A1, A2, A3, M)\
do{\
	B1 = A1, B2 = A2, B3 = A3;\
	for (int i = 0; i < 3; i++)\
	{\
		_smac_r1(B1, B2, B3, M);\
		_smac_r1(B3, B1, B2, M);\
		_smac_r1(B2, B3, B1, M);\
	}\
	B1 = _mm_xor_si128(B1, A1);\
	B2 = _mm_xor_si128(B2, A2);\
	B3 = _mm_xor_si128(B3, A3);\
} while (0)


inline void SMAC_Compress_u16(__m128i& A1, __m128i& A2, __m128i& A3, long long N, const uint16_t* X)
{
	__m128i T;
	long long i;
	for (i = 0; i <= (N - 24LL); i += 24LL)
	{
		__m128i M0 = load128(X + i);
		__m128i M1 = load128(X + i + 8);
		__m128i M2 = load128(X + i + 16);
		_smac_r1(A1, A2, A3, M0);
		_smac_r1(A3, A1, A2, M1);
		_smac_r1(A2, A3, A1, M2);
		_smac_r0(A1, A2, A3, smac_const);
	}

	if (i == N) return;

	for (; i <= (N - 8LL); i += 8LL) // unaligned blocks
	{
		__m128i M = load128(X + i);
		_smac_r0(A1, A2, A3, M);
	}

	if (i < N)	// unaligned bytes
	{
		uint16_t tmp[8] = { 0 };
		memcpy(tmp, X + i, (N - i) << 1);
		__m128i M = load128(tmp);
		_smac_r0(A1, A2, A3, M);
	}
	_smac_r0(A1, A2, A3, smac_const);	// force '1' even if 1 or 2 unaligned blocks
}


// -----------------------------------------------------------
// Rocca-S
// -----------------------------------------------------------
#define SC_Round(S, X0, X1) \
	S##7 = _mm_aesenc_si128(S##5, S##4); \
	S##5 = _mm_aesenc_si128(S##4, S##3); \
	S##4 = _mm_aesenc_si128(S##3, X1); \
	S##3 = _mm_aesenc_si128(S##2, S##6); \
	S##6 = _mm_xor_si128(S##6, S##1); \
	S##2 = _mm_aesenc_si128(S##1, S##0); \
	S##1 = _mm_aesenc_si128(S##0, X0); \
	S##0 = S##6, S##6 = S##7

#define SC_Keystream(S, C0, C1)	\
	C0 = _mm_aesenc_si128(_mm_xor_si128(S##3, S##5), S##0);	\
	C1 = _mm_aesenc_si128(_mm_xor_si128(S##4, S##6), S##2)


// -----------------------------------------------------------
// Other / not used
// -----------------------------------------------------------
/*
The below commented code is not really needed for this implementation of ALF
but might be needed for another implementation.
These functions can be used to propagate the high bit up to the MSB in parallel
for 8/16/32/64-bit words within a 256-bit register, which could be helpful for
a CW assistance. Also, a popcnt would give the ceil(log2) of the items.
*/
#if 0
// Mask all bits up to the most significant '1'
const __m256i ps_precision_mask = _mm256_set1_epi32((1ULL << 23) - 1ULL);

inline __m256i _mm256_log2mask_epu8(__m256i v)
{
	__m256i one = _mm256_set1_epi32(255);
	__m256i u2 = _mm256_and_si256(_mm256_srli_epi32(v, 16), one);
	__m256i u3 = _mm256_and_si256(_mm256_srli_epi32(v, 24), one);
	__m256i u0 = _mm256_and_si256(v, one);
	__m256i u1 = _mm256_and_si256(_mm256_srli_epi32(v, 8), one);
	u2 = _mm256_castps_si256(_mm256_cvtepi32_ps(u2));
	u3 = _mm256_castps_si256(_mm256_cvtepi32_ps(u3));
	u0 = _mm256_castps_si256(_mm256_cvtepi32_ps(u0));
	u1 = _mm256_castps_si256(_mm256_cvtepi32_ps(u1));
	u2 = _mm256_or_si256(u2, ps_precision_mask);
	u3 = _mm256_or_si256(u3, ps_precision_mask);
	u0 = _mm256_or_si256(u0, ps_precision_mask);
	u1 = _mm256_or_si256(u1, ps_precision_mask);
	u2 = _mm256_cvttps_epi32(_mm256_castsi256_ps(u2));
	u3 = _mm256_cvttps_epi32(_mm256_castsi256_ps(u3));
	u2 = _mm256_or_si256(_mm256_slli_epi32(u2, 16), _mm256_slli_epi32(u3, 24));
	u0 = _mm256_cvttps_epi32(_mm256_castsi256_ps(u0));
	u1 = _mm256_cvttps_epi32(_mm256_castsi256_ps(u1));
	u0 = _mm256_or_si256(u0, u2);
	u0 = _mm256_or_si256(u0, _mm256_slli_epi32(u1, 8));
	return u0;
}

inline __m256i _mm256_log2mask_epu16(__m256i v)
{
	__m256i f0 = _mm256_and_si256(v, _mm256_set1_epi32(0xffff));
	__m256i f1 = _mm256_srli_epi32(v, 16);
	f0 = _mm256_castps_si256(_mm256_cvtepi32_ps(f0));
	f1 = _mm256_castps_si256(_mm256_cvtepi32_ps(f1));
	f0 = _mm256_or_si256(f0, ps_precision_mask);
	f1 = _mm256_or_si256(f1, ps_precision_mask);
	f0 = _mm256_cvttps_epi32(_mm256_castsi256_ps(f0));
	f1 = _mm256_cvttps_epi32(_mm256_castsi256_ps(f1));
	return _mm256_or_si256(f0, _mm256_slli_epi32(f1, 16));
}

inline __m256i _mm256_log2mask_epu32(__m256i v)
{
	__m256i v0 = _mm256_and_si256(v, _mm256_set1_epi32(0xffff));
	__m256i v1 = _mm256_srli_epi32(v, 16);
	__m256i c = _mm256_cmpeq_epi16(v, _mm256_setzero_si256());
	c = _mm256_xor_si256(c, _mm256_set1_epi32(-1));
	c = _mm256_srli_epi32(c, 16);
	v0 = _mm256_castps_si256(_mm256_cvtepi32_ps(v0));
	v1 = _mm256_castps_si256(_mm256_cvtepi32_ps(v1));
	v0 = _mm256_or_si256(v0, ps_precision_mask);
	v1 = _mm256_or_si256(v1, ps_precision_mask);
	v0 = _mm256_cvttps_epi32(_mm256_castsi256_ps(v0));
	v1 = _mm256_cvttps_epi32(_mm256_castsi256_ps(v1));
	v0 = _mm256_or_si256(v0, c);
	v1 = _mm256_slli_epi32(v1, 16);
	return _mm256_or_si256(v0, v1);
}

inline __m256i _mm256_log2mask_epu64(__m256i v)
{
	__m256i v0 = _mm256_and_si256(v, _mm256_set1_epi32(0xffff));
	__m256i v1 = _mm256_srli_epi32(v, 16);
	__m256i c = _mm256_cmpeq_epi16(v, _mm256_setzero_si256());
	c = _mm256_xor_si256(c, _mm256_set1_epi32(-1));
	v0 = _mm256_castps_si256(_mm256_cvtepi32_ps(v0));
	v1 = _mm256_castps_si256(_mm256_cvtepi32_ps(v1));
	c = _mm256_or_si256(_mm256_srli_epi64(c, 16), _mm256_srli_epi64(c, 32));
	c = _mm256_or_si256(c, _mm256_srli_epi64(c, 16));
	v0 = _mm256_or_si256(v0, ps_precision_mask);
	v1 = _mm256_or_si256(v1, ps_precision_mask);
	v0 = _mm256_cvttps_epi32(_mm256_castsi256_ps(v0));
	v1 = _mm256_cvttps_epi32(_mm256_castsi256_ps(v1));
	v0 = _mm256_or_si256(v0, c);
	v1 = _mm256_slli_epi32(v1, 16);
	return _mm256_or_si256(v0, v1);
}
#endif


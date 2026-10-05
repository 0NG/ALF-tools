#define _CRT_SECURE_NO_WARNINGS

#include "alf_common.h"
#include "alf_ktm.h"
#include "alf_01.h"
#include "alf_16.h"
#include "alf_nt.h"
#include "alf_pack144.h"
#include <stdio.h>

// ----------------------------------------------------------------------
// Generic and Complete ALF solution with all interfaces supported
// In addition, provides cases for Modulus in range [2^144+1..infty]
// ----------------------------------------------------------------------

struct ALF
{
	union
	{	ALF0 alf0;
		ALF1 alf1;
		ALFnt alfnt;
		ALF16 alf16; // only this one can be combined with a stream
	} A;
	int A_select;
	KTM ktm;
	Pack144 pack;	// packed bigInt of the first lambda elements
	uint64_t N;	// length of data
	uint64_t lambda; // length of initial X-values that can be packed
	const uint16_t* Qmax_distinct_ref;	// distinct modulus
	uint16_t Qmax_same_modulus; // same modulus
	uint8_t tweak_saved[16];

	// -------------------------------------------------------------
	// Internal helper functions
	// -------------------------------------------------------------
	inline void _int_EngineInit(void)
	{	// assuming (pack.Qmax, N, lambda) is ready
		if (pack.bit_width < 16)
			A_select = pack.bit_width <= 8 ? 0 : 1;
		else
			A_select = pack.bit_width < 128 ? 2 : 3;
		if (A_select == 0) A.alf0.EngineInit((uint8_t)pack.Qmax.u[0]);
		else if (A_select == 1) A.alf1.EngineInit((uint16_t)pack.Qmax.u[0]);
		else if (A_select == 2) A.alfnt.EngineInit(pack.Qmax, pack.bit_width);
		else A.alf16.EngineInit(pack.Qmax, pack.bit_width);
	}

	inline void _int_Encrypt(__m192i& x)
	{
		if (A_select == 0)
			x.u[0] = A.alf0.sbox[x.u[0]];
		else if (A_select == 1)
			x.u[0] = A.alf1.Encrypt((uint16_t)x.u[0]);
		else if (A_select == 2)
			A.alfnt.Encrypt((uint8_t*)x.u, (uint8_t*)x.u);
		else
			A.alf16.Encrypt(x);
	}

	inline void _int_Decrypt(__m192i& x)
	{
		if (A_select == 0)
			x.u[0] = A.alf0.sbox[x.u[0]];
		else if (A_select == 1)
			x.u[0] = A.alf1.Decrypt((uint16_t)x.u[0]);
		else if (A_select == 2)
			A.alfnt.Decrypt((uint8_t*)x.u, (uint8_t*)x.u);
		else
			A.alf16.Decrypt(x);
	}

	// -------------------------------------------------------------
	// Shared API: KeyInit, TweakInit, PrepareDecrypt
	// -------------------------------------------------------------
	inline void KeyInit(uint8_t key[16], uint64_t AppID)
	{
		if (N == 1)
			ktm.KeyInit(key, AppID, pack.Qmax);
		else
			if (Qmax_distinct_ref) // stream of distinct modulus
				ktm.KeyInit(key, AppID, N, Qmax_distinct_ref);
			else // stream of same modulus
				/* [Fixed] according to the paper, we should use Q=q in this place, not q^N */
				ktm.KeyInit(key, AppID, N, (uint16_t)Qmax_same_modulus);
				/* [Was wrong] ktm.KeyInit(key, AppID, N, (uint16_t)pack.Qmax.u[0]); */

	}

	inline void TweakInit(uint8_t tweak[16])
	{
		if (lambda == N)
		{
			if (A_select == 0) A.alf0.TweakInit(ktm, tweak);
			if (A_select == 1) A.alf1.TweakInit(ktm, tweak);
			if (A_select == 2) A.alfnt.TweakInit(ktm, tweak);
			if (A_select == 3) A.alf16.TweakInit(ktm, tweak);
		}
		else
			store128(tweak_saved, load128(tweak));
	}

	inline void PrepareDecrypt(void)
	{
		if (A_select == 0)
			A.alf0.PrepareDecrypt();
		else if (A_select == 1)
			;	// do nothing, it already knows dec.RKs and has a reversed loop
		else if (A_select == 2)
			A.alfnt.PrepareDecrypt();
		else // if (A_select == 3) -- the only one remained case, thus no if() needed
			if (lambda == N)
				A.alf16.PrepareDecrypt();
			else
				;	// if Y-part exists then we postpone PrepareDecrypt() to the time
					// when we know the actual part of Y during the Decrypt procedure
	}

	// -------------------------------------------------------------
	// The First Interface -- single item modulo up to 2^144
	// -------------------------------------------------------------
	// single object (can be large up to 144 bits)
	inline void EngineInit(__m192i Qmax)
	{
		lambda = N = 1;
		pack.make1(Qmax);
		_int_EngineInit();
	}

	inline void Encrypt(__m192i& x)
	{
		_int_Encrypt(x);
	}

	inline void Decrypt(__m192i& x)
	{
		_int_Decrypt(x);
	}

	// -------------------------------------------------------------
	// The Second Interface -- single item modulo up to 2^64
	// -------------------------------------------------------------
	inline void EngineInit(uint64_t Qmax)
	{
		lambda = N = 1;
		pack.make1(Qmax);
		_int_EngineInit();
	}

	inline uint64_t Encrypt(uint64_t p)
	{
		__m192i x;
		x.u[0] = p;
		_int_Encrypt(x);
		return x.u[0];
	}

	inline uint64_t Decrypt(uint64_t c)
	{
		__m192i x;
		x.u[0] = c;
		_int_Decrypt(x);
		return x.u[0];
	}

	// -------------------------------------------------------------
	// The Third Interface -- a vector of N same or distinct modulus
	// Each item of the stream must fit in 16-bit data type uint16_t
	// -------------------------------------------------------------
	// distinct modulus
	inline void EngineInit(uint64_t _N, const uint16_t * Qmax)
	{	Qmax_distinct_ref = Qmax;
		N = _N;
		if (N == 1)
			pack.make1(*Qmax), lambda = 1;
		else
			lambda = pack.make<1>(N, Qmax);
		_int_EngineInit();
	}

	// same modulus
	inline void EngineInit(uint64_t _N, uint16_t Qmax)
	{
		Qmax_distinct_ref = NULL;
		Qmax_same_modulus = Qmax;
		N = _N;
		if (N == 1)
			pack.make1(Qmax), lambda = 1;
		else
			lambda = pack.make<0>(N, &Qmax_same_modulus);
		_int_EngineInit();
	}

	// first interface -- stream with same or distinct modulus
	void Encrypt(uint16_t* out, uint16_t* in)
	{
		__m192i x;

		if (N == 1)
		{	x.u[0] = *in;
			_int_Encrypt(x);
			*out = (uint16_t)x.u[0];
			return;
		}

		// pack the left side of the plaintext
		if (Qmax_distinct_ref)
			pack.pack<1>(x, in, Qmax_distinct_ref);
		else
			pack.pack<0>(x, in, &Qmax_same_modulus);


		if (lambda == N)
			_int_Encrypt(x);
		else
		{
			A.alf16.TweakInit(ktm, tweak_saved, N - lambda, in + lambda, 0x0101);
			_int_Encrypt(x);

			_int_EncDec_Ypart(x, out, in, 0x0201, 0);
			
			A.alf16.TweakInit(ktm, tweak_saved, N - lambda, out + lambda, 0x0301);
			_int_Encrypt(x);

			_int_EncDec_Ypart(x, out, out, 0x0401, 0);

			A.alf16.TweakInit(ktm, tweak_saved, N - lambda, out + lambda, 0x0501);
			_int_Encrypt(x);
		}

		// unpack the left part of the ciphertext
		if (Qmax_distinct_ref)
			pack.unpack<1>(x, out, Qmax_distinct_ref);
		else
			pack.unpack<0>(x, out, &Qmax_same_modulus);
	}

	void Decrypt(uint16_t* out, uint16_t* in)
	{
		__m192i x;

		if (N == 1)
		{
			x.u[0] = *in;
			_int_Decrypt(x);
			*out = (uint16_t)x.u[0];
			return;
		}

		// Pack the left side of the ciphertext
		if (Qmax_distinct_ref)
			pack.pack<1>(x, in, Qmax_distinct_ref);
		else
			pack.pack<0>(x, in, &Qmax_same_modulus);

		if (lambda == N)
			_int_Decrypt(x);
		else
		{
			A.alf16.TweakInit(ktm, tweak_saved, N - lambda, in + lambda, 0x0501);
			A.alf16.PrepareDecrypt();
			_int_Decrypt(x);

			_int_EncDec_Ypart(x, out, in, 0x0401, 1);

			A.alf16.TweakInit(ktm, tweak_saved, N - lambda, out + lambda, 0x0301);
			A.alf16.PrepareDecrypt();
			_int_Decrypt(x);

			_int_EncDec_Ypart(x, out, out, 0x0201, 1);

			A.alf16.TweakInit(ktm, tweak_saved, N - lambda, out + lambda, 0x0101);
			A.alf16.PrepareDecrypt();
			_int_Decrypt(x);
		}

		// Unpack the left side of the plaintext to the output
		if (Qmax_distinct_ref)
			pack.unpack<1>(x, out, Qmax_distinct_ref);
		else
			pack.unpack<0>(x, out, &Qmax_same_modulus);
	}

	// -------------------------------------------------------------
	// Radix-stream implementation with the use of AVX2 and __m256i
	// For the keystream we use Rocca-S
	// x is used as a "salt" value, between [128..144] bits long
	// -------------------------------------------------------------
	void _int_EncDec_Ypart(__m192i x, uint16_t* out, uint16_t* in, uint32_t C, char is_decrypt)
	{
		__m128i S0, S1, S2, S3, S4, S5, S6, S7;	// state of Rocca-S (S7 is a temp variable)
		do
		{
			// derive Rocca-S state based on (ktm||tweak||x||domain) using SMAC.InitFinal()
#if 1
			//... somewhat optimised but more difficult to read
			__m128i M, T, A1 = ktm.A1, A2 = ktm.A2, A3 = ktm.A3;
			__m128i C1 = _mm_setr_epi32((x.u[2] << 16) | C, 0, 0, 0);
			__m128i C2 = _mm_add_epi8(C1, smac_const);
			__m128i C3 = _mm_add_epi8(C2, smac_const);

			M = load128(tweak_saved);
			_smac_r1(A1, A2, A3, M);	// push Tweak
			M = load128(x.u);
			_smac_r1(A3, A1, A2, M);	// push X_lo
			// current state is (A2,A3,A1)

			S4 = M = T = A2;
			S5 = S2 = S0 = A3;
			S6 = S3 = S1 = A1;
			for (int i = 0; i < 3; i++)
			{
				_smac_r1(T, S0, S1, C1);
				_smac_r1(M, S2, S3, C2);
				_smac_r1(S4, S5, S6, C3);
				_smac_r1(S1, T, S0, C1);
				_smac_r1(S3, M, S2, C2);
				_smac_r1(S6, S4, S5, C3);
				_smac_r1(S0, S1, T, C1);
				_smac_r1(S2, S3, M, C2);
				_smac_r1(S5, S6, S4, C3);
			}
			S4 = _mm_xor_si128(S4, A2);
			S0 = _mm_xor_si128(S0, A3);
			S2 = _mm_xor_si128(S2, A3);
			S5 = _mm_xor_si128(S5, A3);
			S1 = _mm_xor_si128(S1, A1);
			S3 = _mm_xor_si128(S3, A1);
			S6 = _mm_xor_si128(S6, A1);
#else			
			// perhaps a bit slower code but easier to read
			__m128i M, T, A1 = ktm.A1, A2 = ktm.A2, A3 = ktm.A3;
			M = load128(tweak_saved);
			_smac_r0(A1, A2, A3, M);	// push Tweak
			M = load128(x.u);
			_smac_r0(A1, A2, A3, M);	// push X_lo
			M = _mm_setr_epi32((x.u[2] << 16) | C, 0, 0, 0);
			_smac_initfinal2(T, S0, S1, A1, A2, A3, M);
			M = _mm_add_epi8(M, smac_const); // +1 to the counter
			_smac_initfinal2(T, S2, S3, A1, A2, A3, M);
			M = _mm_add_epi8(M, smac_const); // +1 to the counter
			_smac_initfinal2(S4, S5, S6, A1, A2, A3, M);
#endif
		} while (0);
		
		uint32_t pool[8], pool_idx = 8;
		uint16_t tmpA[16], tmpB[16], tmpC[16];

		// Now when Rocca-S state is initialised, EncDec with Radix-stream		
		__m256i Q = _mm256_set1_epi16(Qmax_same_modulus + 1); /* modulus(16) */
		__m256i H = _mm256_cmpeq_epi16(Q, _mm256_setzero_si256()); // to handle Q=2^16
		__m256i c_one = _mm256_set1_epi16(1);

		for (uint64_t i = lambda; i < N; i += 16)
		{
			__m256i Z0, Z1 /* keystream(32) */;
			__m256i M0, M1, M2 /* M=Z*Q(48) */, F0, F1 /* cmp res */;
			__m256i T1;

			if ((i + 16) > N)
			{	// ending routine
				if (Qmax_distinct_ref)
				{
					memset(tmpA, -1, 32);
					memcpy(tmpA, Qmax_distinct_ref + i, (N - i) * 2);
					Q = _mm256_add_epi16(_mm256_loadu_si256((__m256i*)tmpA), c_one);
				}
				else
				{
					_mm256_storeu_si256((__m256i*)tmpA, Q);
					memset(tmpA + (N - i), 0, 32 - (N - i) * 2);
					Q = _mm256_loadu_si256((__m256i*)tmpA);
				}
				H = _mm256_cmpeq_epi16(Q, _mm256_setzero_si256()); // to handle Q=2^16
			}
			else if (Qmax_distinct_ref)
			{
				// H=0xffff where Q=0x0000 (meaning Q=2^16)
				Q = _mm256_add_epi16(_mm256_loadu_si256((__m256i*)(Qmax_distinct_ref + i)), c_one);
				H = _mm256_cmpeq_epi16(Q, _mm256_setzero_si256()); // to handle Q=2^16
			}

			// Retrieve two keystreams (Z1||Z0) -- representing 32-bit integers
			// compute (M2||M1||M0) = Q * (Z1||Z0) -- results in 48-bit integers
			__m128i C0, C1, zero = c_00;
			SC_Keystream(S, C0, C1);
			Z0 = _mm256_setr_m128i(C0, C1);
			M1 = _mm256_mulhi_epu16(Q, Z0);
			M0 = _mm256_mullo_epi16(Q, Z0);
			SC_Round(S, zero, zero);
			SC_Keystream(S, C0, C1);
			Z1 = _mm256_setr_m128i(C0, C1);
			M2 = _mm256_mulhi_epu16(Q, Z1);
			T1 = _mm256_mullo_epi16(Q, Z1);
			M2 = _mm256_blendv_epi8(M2, Z1, H); // to handle Q=2^16
			SC_Round(S, zero, zero);
			
			// Add the middle M1 += T1 and propagate the add-carry to M2
			M1 = _mm256_add_epi16(M1, T1); 
			M2 = _mm256_add_epi16(M2, c_one);
			M2 = _mm256_add_epi16(M2, _mm256_cmple_epu16(T1, M1));

			/*
				Check for a Bad case := (M1||M0) < Q
				I.e., to be Good it must be either M1!=0 or M0>=Q
				Note that for Q=2^16=> Q=0 it is always Good (M0>=0)
			*/
			F0 = _mm256_cmple_epu16(Q, M0); // ff=>good, 00=>bad
			F1 = _mm256_cmpeq_epi16(M1, _mm256_setzero_si256()); // 00=>good, ff=>bad
			uint32_t mask = _mm256_movemask_epi8(_mm256_andnot_si256(F0, F1)); // '11'=>bad

			// Actual enc/dec with the IN stream and store to the OUT stream
			__m256i IN = _mm256_loadu_si256((__m256i*)(in + i));

			if(is_decrypt)
				M2 = _mm256_submod_epu16(IN, M2, Q);
			else
				M2 = _mm256_addmod_epu16(IN, M2, Q);

			if ((i + 16) > N)
			{
				_mm256_storeu_si256((__m256i*)tmpA, M2);
				memcpy(out + i, tmpA, (N - i) * 2);
			}
			else
				_mm256_storeu_si256((__m256i*)(out + i), M2);

			if (!mask) continue;

			// Special routine in case (M1*2^16 + M0) < Q (ps: note M1 is zero)
			// Probability to get here is very small compared to the above critical loop
			_mm256_storeu_si256((__m256i*)tmpA, Q);
			_mm256_storeu_si256((__m256i*)tmpB, M0);
			_mm256_storeu_si256((__m256i*)tmpC, IN);

			for (int k = 0; mask; k++, mask >>= 2)
				if (mask & 1)
				{
					uint32_t s = tmpA[k]; // ((uint16_t*)&Q)[k];
					uint32_t l = tmpB[k]; // ((uint16_t*)&M0)[k];
					uint32_t t = ((uint32_t)-(int32_t)s) % s;
					if (l >= t) continue;
					uint32_t x = tmpC[k]; // ((uint16_t*)&IN)[k];
					uint64_t m = 0;
					do
					{
						if (pool_idx == 8)
						{
							pool_idx = 0;
							__m128i C0, C1, zero = c_00;
							SC_Keystream(S, C0, C1);
							SC_Round(S, zero, zero);
							store128(pool, C0);
							store128(pool + 4, C1);
						}
						m = (uint64_t)s * (uint64_t)pool[pool_idx++];
					} while ((uint32_t)m < t);
					
					m >>= 32;
					if (is_decrypt)	m = s - m;
					out[i + k] = (uint16_t)((m + x) % s);
				}
		}
	}

	void print_conf(char * conf)
	{
		const char* bn = pack.Qmax.popcnt() == pack.bit_width ? "b" : "n";
		conf += sprintf(conf, "N=%lld", N);
		if (N > 1) conf += sprintf(conf, " lambda=%d", (int)lambda);

		if (A_select == 0) conf += sprintf(conf, " ALF-0%s r=32", bn);
		if (A_select == 1) conf += sprintf(conf, " ALF-1-%d%s r=48", A.alf1.t, bn);
		if (A_select == 2) conf += sprintf(conf, " ALF-%d-%d%s r=%d", A.alfnt.n, A.alfnt.t, bn, A.alfnt.Rounds);
		if (A_select == 3) conf += sprintf(conf, " ALF-16-%d%s r=12", A.alf16.t, bn);

		if (N > 1)
			if (Qmax_distinct_ref)
				conf += sprintf(conf, " dist.Q[...]");
			else
				conf += sprintf(conf, " same.Q=0x%05x", 1 + (unsigned int)Qmax_same_modulus);

		const char* qn = N > 1 ? "maxlam" : "max";
		if (A_select == 0)
			conf += sprintf(conf, " Q%s=0x%02x", qn, (unsigned int)pack.Qmax.u[0]);
		if (A_select == 1)
			conf += sprintf(conf, " Q%s=0x%04x", qn, (unsigned int)pack.Qmax.u[0]);
		if (A_select == 2 || A_select == 3)
			conf += sprintf(conf, " Q%s=0x%04llx %016llx %016llx", qn, pack.Qmax.u[2], pack.Qmax.u[1], pack.Qmax.u[0]);

	}

};

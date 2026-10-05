// This is an addition module -- encryption when a Nonce is available
// The plaintext contains N 16-bit values
struct ALFNonce
{
	__m128i _A1, _A2, _A3;

	void KeyTweak_Init(uint8_t key[16], uint8_t tweak[16], uint64_t AppID)
	{
		__m128i A1, A2, A3;
		A1 = _mm_setr_epi64x(AppID, 0);
		A2 = load128(key);
		A3 = load128(tweak);
		_smac_initfinal1(A1, A2, A3);
		_A1 = A1, _A2 = A2, _A3 = A3;
	}

	// if Qmax_distinct==NULL then the same modulus is assumed
	void EncDec_Nonce(uint8_t nonce[16], uint64_t N, char is_decrypt, uint16_t* out, uint16_t* in,
		uint16_t Qmax_same_modulus, uint16_t* Qmax_distinct = NULL)
	{
		__m128i S0, S1, S2, S3, S4, S5, S6, S7;
		do
		{	__m128i A1 = _A1, A2 = _A2, A3 = _A3, T, M = load128(nonce);
			_smac_r0(A1, A2, A3, M);
			M = smac_const;
			for (int r = 0; r < 3; r++)
			{
				__m128i B1 = A1, B2 = A2, B3 = A3;
				for (int i = 0; i < 3; i++)
				{
					_smac_r1(B1, B2, B3, M);
					_smac_r1(B3, B1, B2, M);
					_smac_r1(B2, B3, B1, M);
				}
				B1 = _mm_xor_si128(B1, A1);
				B2 = _mm_xor_si128(B2, A2);
				B3 = _mm_xor_si128(B3, A3);
				if (r == 0) S0 = B2, S1 = B3;
				else if (r == 1) S2 = B2, S3 = B3;
				else S4 = B1, S5 = B2, S6 = B3;
				M = _mm_add_epi32(M, smac_const);
			}
		} while (0);

		__m256i Q = _mm256_set1_epi16(Qmax_same_modulus + 1); /* modulus(16) */
		__m256i H = _mm256_cmpeq_epi16(Q, _mm256_setzero_si256()); // to handle Q=2^16
		__m256i c_one = _mm256_set1_epi16(1);
		uint32_t pool[8], pool_idx = 8;
		uint16_t tmpA[16], tmpB[16], tmpC[16];

		for (uint64_t i = 0; i < N; i += 16)
		{
			__m256i Z0, Z1 /* keystream(32) */;
			__m256i M0, M1, M2 /* M=Z*Q(48) */, F0, F1 /* cmp res */;
			__m256i T1;

			if ((i + 16) > N)
			{	// ending routine
				if (Qmax_distinct)
				{
					memset(tmpA, -1, 32);
					memcpy(tmpA, Qmax_distinct + i, (N - i) * 2);
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
			else if (Qmax_distinct)
			{
				// H=0xffff where Q=0x0000 (meaning Q=2^16)
				Q = _mm256_add_epi16(_mm256_loadu_si256((__m256i*)(Qmax_distinct + i)), c_one);
				H = _mm256_cmpeq_epi16(Q, _mm256_setzero_si256()); // to handle Q=2^16
			}

			// Retrieve two keystreams (Z1||Z0) -- representing 32-bit integers
			// compute (M2||M1||M0) = Q * (Z1||Z0) -- results in 48-bit integers
			__m128i C0, C1;
			SC_Keystream(S, C0, C1);
			Z0 = _mm256_setr_m128i(C0, C1);
			M1 = _mm256_mulhi_epu16(Q, Z0);
			M0 = _mm256_mullo_epi16(Q, Z0);
			SC_Round(S, c_00, c_00);
			SC_Keystream(S, C0, C1);
			Z1 = _mm256_setr_m128i(C0, C1);
			M2 = _mm256_mulhi_epu16(Q, Z1);
			T1 = _mm256_mullo_epi16(Q, Z1);
			M2 = _mm256_blendv_epi8(M2, Z1, H); // to handle Q=2^16
			SC_Round(S, c_00, c_00);

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

			if (is_decrypt)
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
							__m128i C0, C1, z = c_00;
							SC_Keystream(S, C0, C1);
							SC_Round(S, z, z);
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
};


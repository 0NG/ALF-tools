// -----------------------------------------------------------
// Keyed Configuration State.
// Manages the secret SMAC state, key/twek-init, support for
// HMACing of longer stream modulus and data, derivation of RK[]
// -----------------------------------------------------------
struct KTM
{
	__m128i A1, A2, A3;

	// N=1, used by ALF-0 and ALF-1-t in stand-alone mode
	// Arbitrary modulus (1..15 bits)
	void KeyInit(uint8_t key[16], uint64_t AppID, uint64_t Qmax)
	{
		__m128i _A1, _A2, _A3;
		_A1 = _mm_setr_epi64x(AppID, 1ULL);
		_A2 = load128(key);
		_A3 = _mm_setr_epi64x(Qmax, 0);
		_smac_initfinal1(_A1, _A2, _A3);
		A1 = _A1, A2 = _A2, A3 = _A3;
	}

	// N=1, used by ALF-n-t and ALF-16-t in stand-alone mode
	// Arbitrary modulus (16..144 bits)
	void KeyInit(uint8_t key[16], uint64_t AppID, __m192i Qmax)
	{
		__m128i _A1, _A2, _A3;
		_A1 = _mm_setr_epi64x(AppID, 1ULL | (Qmax.u[2] << 48));
		_A2 = load128(key);
		_A3 = load128(Qmax.u);
		_smac_initfinal1(_A1, _A2, _A3);
		A1 = _A1, A2 = _A2, A3 = _A3;
	}

	// Same-modulus stream of N 16-bit symbols
	void KeyInit(uint8_t key[16], uint64_t AppID, uint64_t N, uint16_t Qmax)
	{
		__m128i _A1, _A2, _A3;
		_A1 = _mm_setr_epi64x(AppID, N);
		_A2 = load128(key);
		_A3 = _mm_setr_epi64x(Qmax, 0);
		_smac_initfinal1(_A1, _A2, _A3);
		A1 = _A1, A2 = _A2, A3 = _A3;
	}

	// Distinct-modulus stream of N 16-bit symbols
	void KeyInit(uint8_t key[16], uint64_t AppID, uint64_t N, const uint16_t* Qmax)
	{
		__m128i _A1, _A2, _A3;
		_A1 = _mm_setr_epi64x(AppID, N);
		_A2 = load128(key);
		_A3 = _mm_setzero_si128();
		_smac_initfinal1(_A1, _A2, _A3);
		SMAC_Compress_u16(_A1, _A2, _A3, N, Qmax);
		A1 = _A1, A2 = _A2, A3 = _A3;
	}

	// RK_num is the requested number of RK blocks to be generated from the current state
	// the space of RK must be a multiple of 3 in this implementation (no checks done)
	inline void TweakInit(__m128i* RK_out, int RK_num, uint32_t C, uint8_t tweak[16] /* [16] */, uint64_t Ysz, uint16_t* Y)
	{
		__m128i _A1 = A1, _A2 = A2, _A3 = A3, T, M;
		
		M = load128(tweak);
		_smac_r0(_A1, _A2, _A3, M);
		
		if (Ysz)
		{
			_smac_r0(_A1, _A2, _A3, smac_const);
			SMAC_Compress_u16(_A1, _A2, _A3, Ysz, Y);
		}

		M = _mm_setr_epi32(C, 0, 0, 0);
		int c = 0;

		while(1)
		{
			__m128i B1 = _A1, B2 = _A2, B3 = _A3;

			for (int i = 0; i < 3; i++)
			{
				_smac_r1(B1, B2, B3, M);
				_smac_r1(B3, B1, B2, M);
				_smac_r1(B2, B3, B1, M);
			}
			RK_out[c + 0] = _mm_xor_si128(B1, _A1);
			RK_out[c + 1] = _mm_xor_si128(B2, _A2);
			RK_out[c + 2] = _mm_xor_si128(B3, _A3);

			c += 3;
			if (c >= RK_num) break;
			M = _mm_add_epi8(M, smac_const); // +1 to the counter
		}
	}

	inline void TweakInit(uint8_t rk[48], uint8_t tweak[16])
	{
		__m128i _A1 = A1, _A2 = A2, _A3 = A3;
		__m128i T, M = load128(tweak);
		_smac_r0(_A1, _A2, _A3, M);
		_smac_initfinal1(_A1, _A2, _A3);
		store128(rk, _A1);
		store128(rk + 16, _A2);
		store128(rk + 32, _A3);
	}
};

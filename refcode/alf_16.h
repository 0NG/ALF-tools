// ------------------------------------------------------
// Provides cases for Modulus in range [2^127+1..2^144]
// Qmax must be in the width between 128 and 144 bits
// the total width is 128+t bits, where t=0..16
// ------------------------------------------------------
const int _ALF16_Rounds = 12;

struct ALF16;
typedef void (ALF16::* alf16_encdec_ft)(__m192i&);

struct ALF16
{
	__m128i RK[12];	// Round keys
	__m128i M;		// t-bit mask, t=0..16
	__m192i Qmax;	// stores bigInt = Qmax << 48
	uint64_t m2, m1, m0;
	int t, is_binary;
	alf16_encdec_ft enc_fptr, dec_fptr;	// enc/dec functions


	// -----------------------------------
	// internal helper functions
	// -----------------------------------
	inline __m128i _UpdateE(__m128i E, __m128i X, __m128i M)
	{
		__m128i U = _mm_clmulepi64_si128(X, _mm_set1_epi64x(0x01010101ULL), 0x00);
#if HAS_AVX512==1
		return _mm_ternarylogic_epi32(E, U, M, 0x28); // 0x28: (E+U)&M
#else
		return _mm_and_si128(xor2(E, U), M);
#endif
	}

	inline __m128i _AddEtoX(__m128i E, __m128i X)
	{
		return xor2(X, shuffle(E, _mm_setr_epi8(3, 3, 3, 3, 7, 7, 7, 7, -1, -1, -1, -1, -1, -1, -1, -1)));
	}

	inline __m128i _m192toE(__m192i& x)
	{
		return shuffle(load128(x.u + 1), _mm_setr_epi8(-1, -1, -1, 8, -1, -1, -1, 9, -1, -1, -1, -1, -1, -1, -1, -1));
	}

	inline uint64_t _Etom192(__m128i E)
	{
		return _mm_cvtsi128_si64x(shuffle(E, _mm_setr_epi8(3, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1)));
	}

	// returns 1 if (X, E) > Qmax
	template<int tnz>
	inline int _cmpgt_maxX(__m128i X, __m128i E)
	{
		const __m128i mergeXE = _mm_setr_epi8(10, 11, 12, 13, 14, 15, 0xf3, 0xf7, 2, 3, 4, 5, 6, 7, 8, 9);
		__m128i H;
		if (tnz)
#if HAS_AVX512==1
			H = _mm_permutex2var_epi8(X, mergeXE, E);
#else
			H = xor2(shuffle(X, mergeXE),
				shuffle(E, _mm_setr_epi8(-1, -1, -1, -1, -1, -1, 3, 7, -1, -1, -1, -1, -1, -1, -1, -1)));
#endif
		else H = shuffle(X, mergeXE);

		uint64_t v = _mm_cvtsi128_si64x(H);
		if (v < m2) return 0;
		if (v > m2) return 1;
		v = _mm_extract_epi64(H, 1);
		if (v < m1) return 0;
		if (v > m1) return 1;
		if ((uint16_t)_mm_extract_epi16(X, 0) > m0) return 1;
		return 0;
	}



	// -----------------------------------
	// Public API
	// -----------------------------------
	void EngineInit(__m192i _Qmax, int bit_width = 0)
	{
		if (bit_width) t = bit_width;
		else t = _Qmax.bitwidth();

		if (t < 128 || t > 144) 
			exit(-1);	// Critical error! Use case is only for [128..144] bits

		Qmax = _Qmax;
		uint64_t* q = (uint64_t*)(((uint16_t*)_Qmax.u) + 1);
		m0 = _Qmax.u[0] & 0xffff;
		m1 = q[0];
		m2 = q[1];
		is_binary = t == _Qmax.popcnt();
		t -= 128;
		uint32_t m = (1 << t) - 1;
		M = _mm_setr_epi8(0, 0, 0, (uint8_t)m, 0, 0, 0, (uint8_t)(m >> 8), 0, 0, 0, 0, 0, 0, 0, 0);
		
		if (is_binary)
			if (t)
				enc_fptr = &ALF16::_int_Encrypt_11, 
				dec_fptr = &ALF16::_int_Decrypt_11;
			else
				enc_fptr = &ALF16::_int_Encrypt_10,
				dec_fptr = &ALF16::_int_Decrypt_10;
		else
			if (t)
				enc_fptr = &ALF16::_int_Encrypt_01,
				dec_fptr = &ALF16::_int_Decrypt_01;
			else
				enc_fptr = &ALF16::_int_Encrypt_00,
				dec_fptr = &ALF16::_int_Decrypt_00;
	}

	void KeyInit(KTM& ktm_out, uint8_t key[16], uint64_t AppID)
	{	ktm_out.KeyInit(key, AppID, Qmax);
	}

	void TweakInit(KTM& ktm_in, uint8_t tweak[16], uint64_t Y_len = 0, uint16_t* Y = NULL, uint32_t C = 0x1)
	{	
		ktm_in.TweakInit(RK, 12, C, tweak, Y_len, Y);
	}

	// Non-binary, t=0
	void _int_Encrypt_00(__m192i& x)
	{
		__m128i X = load128(x.u);
		for (int r = 0; r < _ALF16_Rounds; r += 2)
			do {
				X = aesenc(X, RK[r]);
				X = aesenc(X, RK[r + 1]);
			} while (_cmpgt_maxX<0>(X, c_00));  // CW every 2 rounds
		store128(x.u, X);
	}

	void _int_Decrypt_00(__m192i& x)
	{
		__m128i U, X = load128(x.u);
		X = _mm_aesenclast_si128(X, c_00);
		for (int r = _ALF16_Rounds - 2; r >= 0; r -= 2)
			do {
				X = aesdec(X, RK[r + 1]);
				X = aesdec(X, RK[r]);
				U = _mm_aesdeclast_si128(X, c_00);
			} while (_cmpgt_maxX<0>(U, c_00));
		store128(x.u, U);
	}

	// Non-binary, t>0 -- Generic case that covers all cases
	void _int_Encrypt_01(__m192i& x)
	{
		__m128i U, X = load128(x.u), E = _m192toE(x);
		for (int r = 0; r < _ALF16_Rounds; r += 2)
			do {
				U = aesenc(X, RK[r]);
				X = _AddEtoX(E, U);
				E = _UpdateE(E, U, M);
				U = aesenc(X, RK[r + 1]);
				X = _AddEtoX(E, U);
				E = _UpdateE(E, U, M);
			} while (_cmpgt_maxX<1>(X, E));  // CW every 2 rounds
		x.u[2] = _Etom192(E);
		store128(x.u, X);
	}

	void _int_Decrypt_01(__m192i& x)
	{
		__m128i U, X = load128(x.u), E = _m192toE(x);
		X = _mm_aesenclast_si128(X, c_00);
		for (int r = _ALF16_Rounds - 2; r >= 0; r -= 2)
			do {
				X = aesdec(X, c_00);
				E = _UpdateE(E, X, M);
				X = _AddEtoX(E, xor2(X, RK[r + 1]));
				X = aesdec(X, c_00);
				E = _UpdateE(E, X, M);
				X = _AddEtoX(E, xor2(X, RK[r]));
				U = _mm_aesdeclast_si128(X, c_00);
			} while (_cmpgt_maxX<1>(U, E));
		x.u[2] = _Etom192(E);
		store128(x.u, U);
	}

	// Binary, t=0
	void _int_Encrypt_10(__m192i& x)
	{
		__m128i X = load128(x.u);
		for (int r = 0; r < _ALF16_Rounds; r += 2)
		{
			X = aesenc(X, RK[r]);
			X = aesenc(X, RK[r + 1]);
		}
		store128(x.u, X);
	}

	void _int_Decrypt_10(__m192i& x)
	{
		__m128i X = load128(x.u);
		X = _mm_aesenclast_si128(X, c_00);
		for (int r = _ALF16_Rounds - 1; r >= 0; r -= 2)
		{
			X = aesdec(X, RK[r]);
			X = aesdec(X, RK[r - 1]);
		}
		X = _mm_aesdeclast_si128(X, c_00);
		store128(x.u, X);
	}

	// Binary, t>0
	void _int_Encrypt_11(__m192i& x)
	{
		__m128i U, X = load128(x.u), E = _m192toE(x);
		for (int r = 0; r < _ALF16_Rounds; r += 2)
		{
			U = aesenc(X, RK[r]);
			X = _AddEtoX(E, U);
			E = _UpdateE(E, U, M);
			U = aesenc(X, RK[r + 1]);
			X = _AddEtoX(E, U);
			E = _UpdateE(E, U, M);
		}
		x.u[2] = _Etom192(E);
		store128(x.u, X);
	}

	void _int_Decrypt_11(__m192i& x)
	{
		__m128i X = load128(x.u), E = _m192toE(x);
		X = _mm_aesenclast_si128(X, c_00);
		for (int r = _ALF16_Rounds - 1; r >= 0; r -= 2)
		{
			X = aesdec(X, c_00);
			E = _UpdateE(E, X, M);
			X = _AddEtoX(E, xor2(X, RK[r]));
			X = aesdec(X, c_00);
			E = _UpdateE(E, X, M);
			X = _AddEtoX(E, xor2(X, RK[r - 1]));
		}
		X = _mm_aesdeclast_si128(X, c_00);
		x.u[2] = _Etom192(E);
		store128(x.u, X);
	}

	inline void PrepareDecrypt(void)
	{
		for (int r = 0; r < _ALF16_Rounds; r += 2)
			RK[r + 0] = _mm_aesimc_si128(RK[r + 0]),
			RK[r + 1] = _mm_aesimc_si128(RK[r + 1]);
	}

	inline void Encrypt(__m192i& x)
	{
		(this->*enc_fptr)(x);
	}

	inline void Decrypt(__m192i& x)
	{
		(this->*dec_fptr)(x);
	}
};

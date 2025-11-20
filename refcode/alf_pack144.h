/*
// Here is the method to handle packing/unpacking of smaller modulus
// into a bigInt integer.
// It first analyses the vector of smaller modulus to prepare for a
// faster packing afterwards.
// We assume all input modulus (Mod) are actually Qmax=(Mod-1) values
// Reason: to support modulus 2^16 in uint16_t data structures
*/

struct Pack144
{
	__m192i Qmax;		// Combined modulus minus 1
	int bit_width;		// Bit-width of a maximum combined value (=Qmax)

	// may be undefined if make1()|N=1 used, otherwise make()|N>1 defines these
	uint64_t M0, M1, M2;// 64-bit submodulus (without -1)
	int n0, n1, n2;		// how many modulus are combined into separate u64 blocks

	void make1(__m192i _Qmax)
	{
		Qmax = _Qmax;
		bit_width = Qmax.bitwidth();
	}

	void make1(uint64_t _Qmax)
	{
		Qmax.set1(_Qmax);
		bit_width = Qmax.bitwidth();
	}

	// -----------------------------------------
	// pack modulus into u64 as much as possible
	// returns the number of modulus managed to pack
	template<int is_distinct_modulus>
	int getmod64(uint64_t& M, int N, const uint16_t* Mod)
	{
		M = 1;
		if (is_distinct_modulus)
			for (int i = 0; i < N; i++)
			{
				uint64_t hi, m = _mulx_u64(M, (uint64_t)Mod[i] + 1ULL, (_GCC_U64_TYPE*)(&hi));
				if (hi) return i;
				M = m;
			}
		else
		{
			uint64_t mod = (uint64_t)*Mod + 1ULL;
			for (int i = 0; i < N; i++)
			{
				uint64_t hi, m = _mulx_u64(M, mod, (_GCC_U64_TYPE*)(&hi));
				if (hi) return i;
				M = m;
			}
		}
		return N;
	}

	// prepare data for packing, returns the number of processed modulus
	template<int is_distinct_modulus>
	int make(uint64_t N, const uint16_t* Mod)
	{
		if (is_distinct_modulus)
		{
			n0 = getmod64<1>(M0, N, Mod);
			n1 = getmod64<1>(M1, N - n0, Mod + n0);
		}
		else
		{
			n0 = getmod64<0>(M0, N, Mod);
			if ((N - n0) >= n0)
				n1 = n0, M1 = M0;
			else
				n1 = getmod64<0>(M1, N - n0, Mod);
		}
		
		Qmax.u[0] = _mulx_u64(M0, M1, (_GCC_U64_TYPE*)(&Qmax.u[1]));
		Qmax.u[2] = 0;

		// now the most complicated/heavy part -- determine n2
		// we want to break as soon as we reach the first <=144 bits
		n2 = 0, M2 = 1;

		__m192i tmp = Qmax;

		for (uint64_t i = n0 + n1; i < N; i++)
		{
			uint64_t m = is_distinct_modulus ? Mod[i] : Mod[0];
			m += 1ULL;
			tmp.mulc(m);
			int bits = tmp.bitwidth();
			if (bits > 145) break;
			if (bits < 145 || (!tmp.u[0] && !tmp.u[1] && tmp.u[2] == (1ULL << 16)))
			{
				Qmax = tmp;
				M2 *= m;
				n2++;
			}
		}

		Qmax.subc(1);	// now subtract 1 so that we get Qmax=Modulo-1
		bit_width = Qmax.bitwidth();	// since the index goes from 0
		return n0 + n1 + n2;
	}

	// -----------------------------------------
	// Pack/Unpack Data
	template<int is_distinct_modulus>
	uint64_t pack64(uint16_t* X, int N, const uint16_t* Mod)
	{
		if (!N) return 0;
		uint64_t P = X[0];
		if (is_distinct_modulus)
		{
			for (int i = 1; i < N; i++)
				P += P * (uint64_t)Mod[i] + (uint64_t)X[i];
		}
		else
		{
			uint64_t m = (uint64_t)Mod[0] + 1ULL;
			for (int i = 1; i < N; i++)
				P = P * m + (uint64_t)X[i];
		}
		return P;
	}

	template<int is_distinct_modulus>
	void unpack64(uint64_t P, uint16_t* X, int N, const uint16_t* Mod)
	{
		if (!N) return;
		if (is_distinct_modulus)
			for (int i = N - 1; i; i--)
			{
				uint64_t m = (uint64_t)Mod[i] + 1ULL;
				X[i] = (uint16_t)(P % m);
				P /= m;
			}
		else
		{
			uint64_t m = (uint64_t)Mod[0] + 1ULL;
			for (int i = N - 1; i; i--)
			{	X[i] = (uint16_t)(P % m);
				P /= m;
			}
		}
		X[0] = P;
	}

	// pack N symbols X modulo Mod into a single long integer (assuming it fits)
	template<int is_distinct_modulus>
	void pack(__m192i& Q, uint16_t* X, const uint16_t* Mod)
	{
		uint64_t X0 = pack64<is_distinct_modulus>(X, n0, Mod);
		uint64_t X1 = pack64<is_distinct_modulus>(X + n0, n1, Mod + (is_distinct_modulus ? n0 : 0));
		uint64_t X2 = pack64<is_distinct_modulus>(X + n0 + n1, n2, Mod + (is_distinct_modulus ? (n0 + n1) : 0));

		Q.u[0] = _mulx_u64(X0, M1, (_GCC_U64_TYPE*)(&Q.u[1]));
		Q.u[2] = 0;
		Q.addc(X1);

		if (M2 > 1ULL)
		{
			Q.mulc(M2);
			Q.addc(X2);
		}
	}
	
	template<int is_distinct_modulus>
	void unpack(__m192i& Q, uint16_t* X, const uint16_t* Mod)
	{
		uint64_t X0, X1, X2;

		if (M2 > 1ULL)
		{
			X2 = Q.divremc(M2);
			unpack64<is_distinct_modulus>(X2, X + n0 + n1, n2, Mod + (is_distinct_modulus ? (n0 + n1) : 0));
		}
		if (M1 > 1ULL)
		{
			X1 = Q.divremc(M1);
			unpack64<is_distinct_modulus>(X1, X + n0, n1, Mod + (is_distinct_modulus ? n0 : 0));
		}
		X0 = Q.u[0];
		unpack64<is_distinct_modulus>(X0, X, n0, Mod);
	}
};

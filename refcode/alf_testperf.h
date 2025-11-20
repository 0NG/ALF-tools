#include <stdio.h>
#include <time.h>

// #define DBG_PRINT 0
void dbg_print(void* ptr, int len, const char* name, int sz = 1)
{
	printf("%s=[", name);

	if (sz == 0 || sz == 1)
	{
		uint8_t* p = (uint8_t*)ptr;
		for (int i = 0; i < len; i++)
			printf(" %02x", (int)(uint32_t)p[i]);
		
		if (sz == 0) printf("...");
	}

	if (sz == 2)
	{
		uint16_t* p = (uint16_t*)ptr;
		for (int i = 0; i < len; i++)
			printf(" %04x", (int)(uint32_t)p[i]);
	}

	if (sz == 4)
	{
		uint32_t* p = (uint32_t*)ptr;
		for (int i = 0; i < len; i++)
			printf(" %08x", (unsigned int)p[i]);
	}

	if (sz == 8)
	{
		uint64_t* p = (uint64_t*)ptr;
		for (int i = 0; i < len; i++)
			printf(" %016llx", p[i]);
	}

	printf("]\n");
}



uint64_t rand64(void)
{
	uint64_t r0 = rand(), r1 = rand(), r2 = rand(), r3 = rand();
	return r0 ^ (r1 << 16) ^ (r2 << 32) ^ (r3 << 48);
}



// Vectors for tests
static uint64_t ref_AppID = 0xf1f2f3f4f5f6f7f8ULL;
static uint8_t ref_key[16] = { 0x05, 0x0a, 0x0f, 0x14, 0x19, 0x1e, 0x23, 0x28, 0x2d, 0x32, 0x37, 0x3c, 0x41, 0x46, 0x4b, 0x50 };
static uint8_t ref_tweak[16] = { 0x0b, 0x16, 0x21, 0x2c, 0x37, 0x42, 0x4d, 0x58, 0x63, 0x6e, 0x79, 0x84, 0x8f, 0x9a, 0xa5, 0xb0 };
static uint16_t ref_Qvec[128] = {
	0x0008, 0x0004, 0x0001, 0x0000, 0x0003, 0x000e, 0x0029, 0x0064, 0x00df, 0x01da, 0x03d5, 0x07d0, 0x0fcb, 0x1fc6, 0x3fc1, 0x7fbc,
	0x0005, 0x0000, 0xfffc, 0xfffa, 0xfffc, 0x0006, 0x0020, 0x005a, 0x00d4, 0x01ce, 0x03c8, 0x07c2, 0x0fbc, 0x1fb6, 0x3fb0, 0x7faa,
	0x0002, 0xfffc, 0xfff7, 0xfff4, 0xfff5, 0xfffe, 0x0017, 0x0050, 0x00c9, 0x01c2, 0x03bb, 0x07b4, 0x0fad, 0x1fa6, 0x3f9f, 0x7f98,
	0xffff, 0xfff8, 0xfff2, 0xffee, 0xffee, 0xfff6, 0x000e, 0x0046, 0x00be, 0x01b6, 0x03ae, 0x07a6, 0x0f9e, 0x1f96, 0x3f8e, 0x7f86,
	0xfffc, 0xfff4, 0xffed, 0xffe8, 0xffe7, 0xffee, 0x0005, 0x003c, 0x00b3, 0x01aa, 0x03a1, 0x0798, 0x0f8f, 0x1f86, 0x3f7d, 0x7f74,
	0xfff9, 0xfff0, 0xffe8, 0xffe2, 0xffe0, 0xffe6, 0xfffc, 0x0032, 0x00a8, 0x019e, 0x0394, 0x078a, 0x0f80, 0x1f76, 0x3f6c, 0x7f62,
	0xfff6, 0xffec, 0xffe3, 0xffdc, 0xffd9, 0xffde, 0xfff3, 0x0028, 0x009d, 0x0192, 0x0387, 0x077c, 0x0f71, 0x1f66, 0x3f5b, 0x7f50,
	0xfff3, 0xffe8, 0xffde, 0xffd6, 0xffd2, 0xffd6, 0xffea, 0x001e, 0x0092, 0x0186, 0x037a, 0x076e, 0x0f62, 0x1f56, 0x3f4a, 0x7f3e };

static const char* ref_name0[] = {
	"T-test, N=1, Q<=2^144",
	"S-test, N>1, same Q<=2^16",
	"D-test, N>1, distinct Q[]",
	"C-test, custom case"
};

// Names for testtype=3 -- Custom hard-coded tests
static const char* ref_name1[] = {
	"Yes/No, N=1, Radix=2",	// idx = 0
	"Credit-card, N=16, Radix=10", // idx=1
	"IPv6, N=6, Radix=2^16", // idx=2
	"IMSI, N=11, Radix=10", // idx=3
	"UTF-16 string, N=256, Radix=2^16", // idx=4
	"English letters, N=1000, Radix=26", // idx=5
	"Letters+digits, N=10000, Radix=36", // idx=6
};

struct AlfTest;
extern struct AlfTest AlfTestVec[];
extern int AlfTestVec_sz;

struct AlfTest
{
	// Input test vectors:
	int type, idx, N;
	uint16_t Qsame; // if N>1, type=1
	uint16_t Qvec_off;// if N>1, type=2, 0xffff means "not this case"
	__m192i Qmax;	// if N=1, type=0

	// Output test vectors:
	uint8_t AKey[3][16]; // KTM state after KeyInit

	union _AfterTweak
	{
		uint8_t _encoded[64];	// the encoded vector of the total structure
		uint8_t alf0_sbox[16];
		uint8_t alf1_RK[16];
		uint8_t alfnt_RK[4][16];
		uint8_t alf16_RK[4][16];
	} ATenc, ATdec;

	// Record only the first 32 bytes of the response.
	// If there are unused bytes, then set them to all 0s
	uint8_t Enc1[32];
	uint8_t Enc5[32];
	uint8_t Enc999[32];

	void get_ntc(int& n, int& t, int& c, int _idx)
	{
		c = _idx % 2, _idx /= 2;
		t = 7 * (_idx % 2), _idx /= 2;
		n = _idx + 1;
	}

	// --------------------------------------------------------
	// MAKE Test Case -- assign test inputs and compute corresponding output vectors
	// 
	// type: 0 for N=1, 1 for N>1 same, 2 for N>1 distinct, 3 for hard-coded use cases
	// return 0 if OK, otherwise -1 if the test does not exist
	// to_print = 0 -- no print
	// to_print = 1 -- print only the header
	// to_print = 2 -- print all vectors
	// to_print = 3 -- print all vectors & C-code
	// --------------------------------------------------------
	int make_test(int _type, int _idx, char to_print = 3)
	{
		// Prepare input vectors
		type = _type;
		idx = _idx;
		Qvec_off = 0xffff;
		Qmax.set1(0);
		Qsame = 0;


		if (type == 0)
		{
			if (_idx < 0) return -1;
			N = 1;
			int n, t, c;
			get_ntc(n, t, c, idx);
			if (n > 17) return -1;

			Qmax.set_pwr2(8 * n + t - c);
			Qmax.u[0] += (uint64_t)c;
			Qmax.subc(1);
		}
		else if (type == 1)
		{
			if (idx < 0 || idx >= 31) return -1;
			if (idx < 15) N = 2 + idx;
			else N = 112 + (idx - 15);
			Qsame = ref_Qvec[N - 1];
		}
		else if (type == 2)
		{
			if (idx < 0 || idx > 126) return -1;
			N = 2 + idx;
			Qvec_off = 128 - N;
		}
		else if (type == 3)
		{
			if (idx == 0)
				N = 1, Qsame = 2 - 1;
			else if (idx == 1)
				N = 16, Qsame = 10 - 1;
			else if (idx == 2)
				N = 6, Qsame = (1 << 16) - 1;
			else if (idx == 3)
				N = 11, Qsame = 10 - 1;
			else if (idx == 4)
				N = 256, Qsame = (1 << 16) - 1;
			else if (idx == 5)
				N = 1000, Qsame = 26 - 1;
			else if (idx == 6)
				N = 10000, Qsame = 36 - 1;
			else return -1;
		}
		else return -1;

		// Compute output vectors
		ALF A;
		if (N == 1)
			A.EngineInit(Qmax);
		else if (Qvec_off == 0xffff)
			A.EngineInit(N, Qsame);
		else
			A.EngineInit(N, ref_Qvec + Qvec_off);

		// Printing the header
		const char* tn[4] = { "T", "S", "D", "C" };
		if (to_print > 0)
		{
			printf("/* --------------------------------------------\n");
			printf("[%s%d] '%s'", tn[type], idx, (type < 3 ? ref_name0[type] : ref_name1[idx]));
			if (type == 0)
			{
				int n, t, c;
				get_ntc(n, t, c, idx);
				printf(" Q=2^%d+%d", 8 * n + t - c, c);
			}
			char conf[260];
			A.print_conf(conf);
			printf(" %s\n", conf);
		}

		A.KeyInit(ref_key, ref_AppID);
		memcpy(AKey[0], &A.ktm.A1, 16);
		memcpy(AKey[1], &A.ktm.A2, 16);
		memcpy(AKey[2], &A.ktm.A3, 16);

		memset(&ATenc, 0, sizeof(ATenc));
		memset(&ATdec, 0, sizeof(ATdec));
		memset(Enc1, 0, sizeof(Enc1));
		memset(Enc5, 0, sizeof(Enc5));
		memset(Enc999, 0, sizeof(Enc999));

		if (to_print > 1)
		{
			printf("after KeyInit:\n");
			dbg_print(AKey[0], 16, "ktm.A1");
			dbg_print(AKey[1], 16, "ktm.A2");
			dbg_print(AKey[2], 16, "ktm.A3");
		}

		A.TweakInit(ref_tweak);

		if (N == A.lambda)
		{
			if (to_print > 1) printf("after TweakInit:\n");

			if (A.A_select == 0)
			{
				int k = 1 + (unsigned int)A.A.alf0.Qmax, f = 1;
				if (k > sizeof(ATenc.alf0_sbox)) k = sizeof(ATenc.alf0_sbox), f = 0;
				memcpy(ATenc.alf0_sbox, A.A.alf0.sbox, k);
				if (to_print > 1) dbg_print(A.A.alf0.sbox, k, "Sbox[]", f);
			}
			else if (A.A_select == 1)
			{
				int k = sizeof(ATenc.alf1_RK);
				memcpy(ATenc.alf1_RK, A.A.alf1.RK, k);
				if (to_print > 1) dbg_print(A.A.alf1.RK, k, "RK[]", 0);
			}
			else if (A.A_select == 2)
			{
				for (int r = 0; r < 4; r++)
					memcpy(ATenc.alfnt_RK[r], A.A.alfnt.RK + r, A.A.alfnt.n);

				if (to_print > 1)
				{
					dbg_print(ATenc.alfnt_RK[0], A.A.alfnt.n, "RK[0]");
					dbg_print(ATenc.alfnt_RK[1], A.A.alfnt.n, "RK[1]");
					dbg_print(ATenc.alfnt_RK[2], A.A.alfnt.n, "RK[2]");
					dbg_print(ATenc.alfnt_RK[3], A.A.alfnt.n, "RK[3]");
				}

			}
			else if (A.A_select == 3)
			{
				for (int r = 0; r < 4; r++)
					memcpy(ATenc.alf16_RK[r], A.A.alf16.RK + r, 16);

				if (to_print > 1)
				{
					dbg_print(ATenc.alf16_RK[0], 16, "RK[0]");
					dbg_print(ATenc.alf16_RK[1], 16, "RK[1]");
					dbg_print(ATenc.alf16_RK[2], 16, "RK[2]");
					dbg_print(ATenc.alf16_RK[3], 16, "RK[3]");
				}
			}
		}

		// Encryption
		__m192i s;
		int sz = (sizeof(Enc1) >> 1);
		if (sz < N) sz = N;
		uint16_t* v = new uint16_t[sz];

		if (N == 1)
		{
			s.set1(0);
			A.Encrypt(s);
			memcpy(Enc1, &s, 24);
			if (to_print > 1) _print_ans(A, s, "Enc^1  (0)");
			for (int i = 0; i < (5 - 1); i++) A.Encrypt(s);
			memcpy(Enc5, &s, 24);
			if (to_print > 1) _print_ans(A, s, "Enc^5  (0)");
			for (int i = 0; i < (999 - 5); i++) A.Encrypt(s);
			memcpy(Enc999, &s, 24);
			if (to_print > 1) _print_ans(A, s, "Enc^999(0)");
		}
		else
		{
			memset(v, 0, sz * 2);
			A.Encrypt(v, v);
			memcpy(Enc1, v, sizeof(Enc1));
			if (to_print > 1) _print_ans(A, v, "Enc^1  (0)");
			for (int i = 0; i < (5 - 1); i++) A.Encrypt(v, v);
			memcpy(Enc5, v, sizeof(Enc5));
			if (to_print > 1) _print_ans(A, v, "Enc^5  (0)");
			for (int i = 0; i < (999 - 5); i++) A.Encrypt(v, v);
			memcpy(Enc999, v, sizeof(Enc999));
			if (to_print > 1) _print_ans(A, v, "Enc^999(0)");
		}

		// Decryption flow
		A.PrepareDecrypt();

		if (N == A.lambda)
		{
			if (to_print > 1) printf("after PrepareDecrypt:\n");

			if (A.A_select == 0)
			{
				int k = 1 + (unsigned int)A.A.alf0.Qmax, f = 1;
				if (k > sizeof(ATdec.alf0_sbox)) k = sizeof(ATdec.alf0_sbox), f = 0;
				memcpy(ATdec.alf0_sbox, A.A.alf0.sbox, k);
				if (to_print > 1) dbg_print(A.A.alf0.sbox, k, "Sbox[]", f);
			}
			else if (A.A_select == 1)
			{
				int k = sizeof(ATdec.alf1_RK);
				memcpy(ATdec.alf1_RK, A.A.alf1.RK, k);
				if (to_print > 1) dbg_print(A.A.alf1.RK, k, "RK[]", 0);

			}
			else if (A.A_select == 2)
			{
				for (int r = 0; r < 4; r++)
					memcpy(ATdec.alfnt_RK[r], A.A.alfnt.RK + r, A.A.alfnt.n);

				if (to_print > 1)
				{
					dbg_print(ATdec.alfnt_RK[0], A.A.alfnt.n, "RK[0]");
					dbg_print(ATdec.alfnt_RK[1], A.A.alfnt.n, "RK[1]");
					dbg_print(ATdec.alfnt_RK[2], A.A.alfnt.n, "RK[2]");
					dbg_print(ATdec.alfnt_RK[3], A.A.alfnt.n, "RK[3]");
				}
			}
			else if (A.A_select == 3)
			{
				for (int r = 0; r < 4; r++)
					memcpy(ATdec.alf16_RK[r], A.A.alf16.RK + r, 16);

				if (to_print > 1)
				{
					dbg_print(ATdec.alf16_RK[0], 16, "RK[0]");
					dbg_print(ATdec.alf16_RK[1], 16, "RK[1]");
					dbg_print(ATdec.alf16_RK[2], 16, "RK[2]");
					dbg_print(ATdec.alf16_RK[3], 16, "RK[3]");
				}

			}
		}
		// Decryption
		int ans = 0;
		if (N == 1)
		{
			for (int i = 0; i < 999; i++) A.Decrypt(s);
			ans = !!(s.u[0] | s.u[1] | s.u[2]);
		}
		else
		{
			for (int i = 0; i < 999; i++) A.Decrypt(v, v);
			for (int i = 0; i < N; i++)
				if (v[i]) { ans = -1; break; }
		}

		if (ans)
		{
			printf("FATAL ERROR: Decryption flow returned a wrong answer!\n");
			exit(0);
		}
		else if (to_print > 1)
			printf("Decryption test...OK\n");

		delete[]v;

		if (to_print > 0) printf("*/\n");
		if (to_print > 2)
		{
			printf("/*[%s%d]*/ {%d, %d, %d, 0x%x, 0x%x, {0x%llxULL,0x%llxULL,0x%llxULL}, ", tn[type], idx,
				type, idx, N, (unsigned int)Qsame, (unsigned int)Qvec_off, Qmax.u[0], Qmax.u[1], Qmax.u[2]);
			for (int i = 0; i < 3; i++)
			{
				if (i == 0) printf("{");
				if (i == 1) printf("},");
				if (i == 2) printf("},");
				for (int j = 0; j < 16; j++)
					printf("%s0x%02x", (j ? "," : "{"), (unsigned int)AKey[i][j]);
				if (i == 2) printf("}}, ");
			}

			for (int i = 0; i < 64; i++) printf("%s0x%02x", (i ? "," : "{"), (unsigned int)ATenc._encoded[i]);
			for (int i = 0; i < 64; i++) printf("%s0x%02x", (i ? "," : "},{"), (unsigned int)ATdec._encoded[i]);
			for (int i = 0; i < 32; i++) printf("%s0x%02x", (i ? "," : "},{"), (unsigned int)Enc1[i]);
			for (int i = 0; i < 32; i++) printf("%s0x%02x", (i ? "," : "},{"), (unsigned int)Enc5[i]);
			for (int i = 0; i < 32; i++) printf("%s0x%02x", (i ? "," : "},{"), (unsigned int)Enc999[i]);
			printf("}},\n");
		}
		return 0;
	}

	// --------------------------------------------------------
	// CHECK Test Case -- compare (*this)-test vector (of a single test case) 
	// against the expected ones precomputed earlier in the big table AlfTestVec[]
	// --------------------------------------------------------
	int check_vs_expected(void)
	{
		int i;
		const char* tn[4] = { "T", "S", "D", "C" };
		printf("// [%s%d] ", tn[type], idx);

		for (i = 0; i < AlfTestVec_sz; i++)
			if (AlfTestVec[i].type == type && AlfTestVec[i].idx == idx) break;
		if (i == AlfTestVec_sz)
		{
			printf("ERROR: Corresponding test vector is not found!\n");
			exit(0);
			return -1;
		}
		int cmp = memcmp(this, AlfTestVec + i, sizeof(*this));
		printf("TestVec check is...%s\n", cmp ? "FAILED" : "OK");
		return cmp;
	}

	void _print_ans(ALF& A, __m192i x, const char* name)
	{
		printf("%s=", name);
		/*
		if (A.A_select == 0)
			printf("0x%02x\n", (unsigned int)x.u[0]);
		else if (A.A_select == 1)
			printf("0x%04x\n", (unsigned int)x.u[0]);
		else
		*/
		printf("0x%04llx %016llx %016llx\n", x.u[2], x.u[1], x.u[0]);
	}

	void _print_ans(ALF& A, uint16_t* v, const char* name)
	{
		printf("%s=[", name);
		int sz = A.N;
		if (sz > 16) sz = 16;
		for (int i = 0; i < sz; i++)
			printf(" %04x", (unsigned int)v[i]);
		if (sz != A.N) printf("...");
		printf("]\n");
	}

	// --------------------------------------------------------
	// SPEED Test Case -- measure the speed for the current test case
	// --------------------------------------------------------
	/*
	* dir=0 enc, 1=dec, 2=full cycle
	* level=0 enc
	* level=1 +tweak
	* level=2 +key
	* level=3 +eng
	* Returns the speed in Msps = Million symbols per second
	*/
	template<int dir = 0, int level = 0>
	double speed(int test_num = 1000000, int to_print = 1)
	{
		ALF A;
		if (N == 1)	A.EngineInit(Qmax);
		else if (Qvec_off == 0xffff) A.EngineInit(N, Qsame);
		else A.EngineInit(N, ref_Qvec + Qvec_off);

		// Printing the header
		const char* tn[4] = { "T", "S", "D", "C" };
		if (to_print)
		{
			printf("/* --------------------------------------------\n");
			printf("[%s%d] '%s'", tn[type], idx, (type < 3 ? ref_name0[type] : ref_name1[idx]));
			if (type == 0)
			{
				int n, t, c;
				get_ntc(n, t, c, idx);
				printf(" Q=2^%d+%d", 8 * n + t - c, c);
			}
			char conf[260];
			A.print_conf(conf);
			printf(" %s\n", conf);

			printf("SPEED TEST: ");
			if (dir == 2) printf("Full cycle");
			else if (dir == 0) printf("Enc");
			else if (dir == 1) printf("Dec");
			else printf("UNKNOWN!!!");
			if (level >= 1) printf("+Twe");
			if (level >= 2) printf("+Key");
			if (level >= 3) printf("+Eng");
			printf("\n");
		}

		// For speed measurements we do: x=Enc(x) and y=Dec(y)
		__m192i x = { 0 }, y = { 0 };
		uint16_t* vx = new uint16_t[N + 32];
		uint16_t* vy = new uint16_t[N + 32];
		memset(vx, 0, (N + 32) * 2);
		memset(vy, 0, (N + 32) * 2);

		double maxspeed = 0;

		if (N == 1)
			for (int r = 0; r < test_num; r++)
			{

				if (dir < 2 && level < 3)
					A.EngineInit(Qmax);
				if (dir < 2 && level < 2)
					A.KeyInit(ref_key, ref_AppID);
				if (dir < 2 && level < 1)
					A.TweakInit(ref_tweak);
				if (dir < 2 && (dir == 1 && level < 1))
					A.PrepareDecrypt();

				long long count = 0;
				long long tm = time(NULL) + 1;
				while (time(NULL) < tm);
				tm += 1;
				while (time(NULL) < tm)
				{
					for (int t = 0; t < 1000; t++)
					{
						if (dir == 2 || level >= 3)
							A.EngineInit(Qmax);
						if (dir == 2 || level >= 2)
							A.KeyInit(ref_key, ref_AppID);
						if (dir == 2 || level >= 1)
							A.TweakInit(ref_tweak);
						if (dir == 2 || dir == 0)
							A.Encrypt(x);
						if (dir == 2) 
							y.u[0] += x.u[0];	// make y to be dependent on x
						if (dir == 2 || (dir == 1 && level >= 1))
							A.PrepareDecrypt();
						if (dir == 2 || dir == 1)
							A.Decrypt(y);
					}
					count += 1000;
				}

				double msps = (double)count / 1000. / 1000.;
				if (msps > maxspeed) maxspeed = msps;
				int tmp = (vx[0] + vy[0] + x.u[0] + y.u[0]) % 2;
				printf("[%d] Msps=%lf (max=%lf)\n", tmp, msps, maxspeed);
			}
		else
			for (int r = 0; r < test_num; r++)
			{
				if (dir < 2 && level < 3)
				{
					if (Qvec_off == 0xffff) A.EngineInit(N, Qsame);
					else A.EngineInit(N, ref_Qvec + Qvec_off);
				}
				if (dir < 2 && level < 2)
					A.KeyInit(ref_key, ref_AppID);
				if (dir < 2 && level < 1)
					A.TweakInit(ref_tweak);
				if (dir < 2 && (dir == 1 && level < 1))
					A.PrepareDecrypt();

				long long runs = 100;
				if (N >= (1 << 16)) runs = 1;
				long long count = 0;
				long long tm = time(NULL) + 1;
				while (time(NULL) < tm);
				tm += 1;
				while (time(NULL) < tm)
				{
					for (int t = 0; t < runs; t++)
					{
						if (dir == 2 || level >= 3)
						{
							if (Qvec_off == 0xffff) A.EngineInit(N, Qsame);
							else A.EngineInit(N, ref_Qvec + Qvec_off);
						}
						if (dir == 2 || level >= 2)
							A.KeyInit(ref_key, ref_AppID);
						if (dir == 2 || level >= 1)
							A.TweakInit(ref_tweak);
						if (dir == 2 || dir == 0)
							A.Encrypt(vx, vx);
						if (dir == 2) 
							*(uint64_t*)vy += *(uint64_t*)vx; // make y to be dependent on x
						if (dir == 2 || (dir == 1 && level >= 1))
							A.PrepareDecrypt();
						if (dir == 2 || dir == 1)
							A.Decrypt(vy, vy);
					}
					count += runs;
				}

				double msps = (double)N * (double)count / 1000. / 1000.;
				if (msps > maxspeed) maxspeed = msps;
				int tmp = (vx[0] + vy[0] + x.u[0] + y.u[0]) % 2;
				printf("[%d] Msps=%lf (max=%lf)\n", tmp, msps, maxspeed);
			}
		
		delete[]vx;
		delete[]vy;
		fflush(stdout);
		return maxspeed;
	}

};

struct AlfTest AlfTestVec[] = {
#include "testvec.hpp"
{} };
int AlfTestVec_sz = sizeof(AlfTestVec) / sizeof(AlfTestVec[0]);


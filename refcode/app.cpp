#include "alf.h"
#include "alf_testperf.h"
#include "alf_nonce.h"


void example1(void)
{
	uint8_t key[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	uint8_t tweak[16] = { 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
	uint64_t AppID = 0xf0f1f2f3f4f5f6f7ULL;

	__m192i Qmax; // Qmax = 2^102-99 => Q=2^102-98
	Qmax.set_pwr2(102);
	Qmax.subc(99);
	dbg_print(&Qmax, 18, "(Modulus-1)");

	__m192i x, y;
	do x.random(102); while(x.cmpgt(Qmax));	
	dbg_print(&x, 18, "Plaintext X");

	// Initialise ALF
	ALF A;
	A.EngineInit(Qmax);
	A.KeyInit(key, AppID);
	A.TweakInit(tweak);
	
	// Encryption
	y = x;
	A.Encrypt(y);
	dbg_print(&y, 18, "Encrypted X");
	
	// Decryption
	A.PrepareDecrypt();
	A.Decrypt(y);
	dbg_print(&y, 18, "Dec(Enc(X))");

	// check if the Enc-Dec cycle gave the same result as the initial plaintext
	int cmp = memcmp(&x, &y, sizeof(x));
	printf("Enc-Dec cycle result=%s\n", cmp ? "FAILED" : "OK");	
}


int main()
{
	example1();

	AlfTest T;

#if 0	/* SPEED [C-tests] */
	const int num = sizeof(ref_name1) / sizeof(ref_name1[0]);
	double speed[num][9];
	int runs = 10;
	for (int idx = 0; idx < num; idx++)
	{
		T.make_test(3, idx, 0);
		speed[idx][0] = T.speed<2, 0>(runs, 1);
		speed[idx][1] = T.speed<0, 0>(runs, 1);
		speed[idx][2] = T.speed<0, 1>(runs, 1);
		speed[idx][3] = T.speed<0, 2>(runs, 1);
		speed[idx][4] = T.speed<0, 3>(runs, 1);
		speed[idx][5] = T.speed<1, 0>(runs, 1);
		speed[idx][6] = T.speed<1, 1>(runs, 1);
		speed[idx][7] = T.speed<1, 2>(runs, 1);
		speed[idx][8] = T.speed<1, 3>(runs, 1);
		_print_speed_latex_row(idx, speed[idx]);
	}

	printf("\n----------------------------------\n");
	for (int idx = 0; idx < num; idx++)
		_print_speed_latex_row(idx, speed[idx]);
	printf("\n----------------------------------\n");
#endif

#if 1	/* To check all test vectors against the table in testvec.hpp */
	for (int type = 0; type < 4; type++)
		for (int idx = 0; idx < 1000; idx++)
		{
			if (T.make_test(type, idx, 0) < 0) break;
			if (T.check_vs_expected()) exit(0);
		}
#endif

#if 0	/* Generate all test vectors and create the file testvec.hpp */
	for (int type = 0; type < 4; type++)
		for (int idx = 0; idx < 1000; idx++)
			if (T.make_test(type, idx) < 0) break;
#endif

}

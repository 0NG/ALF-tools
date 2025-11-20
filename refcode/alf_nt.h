// ------------------------------------------------------
// Provides middle-cases:
// ALF-n-t for n=[2..15] and t=[0..7]
// the Modulus is in range [2^15+1..2^127]
// Qmax must be in the width [16..127] bits
// ------------------------------------------------------

const struct ALF_Profile
{
	//__m128i encPi;
	//__m128i decPi;
	//__m128i encAlpha;
	__m128i encBeta;
	__m128i encSigma;
	__m128i decAlpha;
	__m128i decBeta;
	__m128i decSigma;
	__m128i decTau;
	__m128i Rho;
	__m128i A;
	//__m128i mergeXE;
	int Rounds0, Rounds1;
}
_AlfProfile[14] = {

{ // -- n=2 -------------
// _mm_setr_epi8( 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8( 2, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 0,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encSigma
_mm_setr_epi8( 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8( 2, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1), // decSigma
_mm_setr_epi8( 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1), // decTau
_mm_setr_epi8( 3, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8(99,99,82,82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), // const A
//_mm_setr_epi8(-13,-13,-13,-13,-13, 0, 1,-13,-13,-13,-13,-13,-13,-13,-13,-13), // mergeXE
20, 28},
{ // -- n=3 -------------
// _mm_setr_epi8( 0, 2, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 0, 2, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8( 3, 3, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 0,-1,-1,-1,-1, 2,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1), // encSigma
_mm_setr_epi8( 0, 1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8( 3, 3, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 0,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 2,-1,-1), // decSigma
_mm_setr_epi8( 0,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1,-1, 2,-1,-1), // decTau
_mm_setr_epi8( 3,-1, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8(-91,-91,99,82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), // const A
//_mm_setr_epi8(-13,-13,-13,-13, 0, 1, 2,-13,-13,-13,-13,-13,-13,-13,-13,-13), // mergeXE
16, 24},
{ // -- n=4 -------------
// _mm_setr_epi8( 0, 1, 2, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 0, 1, 2, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8(-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 0,-1,-1,-1,-1, 1,-1,-1,-1,-1, 2,-1,-1,-1,-1, 3), // encSigma
_mm_setr_epi8( 0, 1, 2, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8(-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 0,-1,-1,-1,-1,-1,-1, 3,-1,-1, 2,-1,-1, 1,-1,-1), // decSigma
_mm_setr_epi8( 0,-1,-1,-1,-1,-1,-1, 3,-1,-1, 2,-1,-1, 1,-1,-1), // decTau
_mm_setr_epi8( 3, 3, 3, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), // const A
//_mm_setr_epi8(-13,-13,-13, 0, 1, 2, 3,-13,-13,-13,-13,-13,-13,-13,-13,-13), // mergeXE
14, 18},
{ // -- n=5 -------------
// _mm_setr_epi8( 1, 0, 4, 2, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 1, 0, 3, 4, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 1,-1, 1,-1,-1,-1,-1,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8( 4, 4, 4, 4, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 1,-1,-1, 0, 3, 0,-1,-1,-1, 0, 4,-1,-1,-1,-1, 2), // encSigma
_mm_setr_epi8( 0, 1, 2, 3, 4,-1, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8( 5, 5, 7, 5, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 1,-1,-1,-1, 2,-1,-1, 4,-1,-1, 3,-1,-1, 0, 2,-1), // decSigma
_mm_setr_epi8( 1,-1,-1,-1, 2,-1,-1, 4,-1,-1, 3,-1,-1, 0,-1,-1), // decTau
_mm_setr_epi8( 3,-1, 3,-1, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8(99,99,99,99, 0,82, 0,82, 0, 0, 0, 0, 0, 0, 0, 0), // const A
//_mm_setr_epi8(-13,-13, 0, 1, 2, 3, 4,-13,-13,-13,-13,-13,-13,-13,-13,-13), // mergeXE
14, 18},
{ // -- n=6 -------------
// _mm_setr_epi8( 2, 1, 0, 5, 4, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 2, 1, 0, 5, 4, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8( 2, 3, 6, 7, 2, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 2,-1,-1,-1, 4, 1,-1,-1,-1, 3, 0,-1,-1,-1,-1, 5), // encSigma
_mm_setr_epi8( 0, 1,-1,-1, 2, 3, 4, 5,-1,-1,-1,-1,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8( 4, 5, 4, 5, 6, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 2, 5,-1,-1, 0,-1,-1,-1,-1,-1,-1, 3,-1, 1, 4,-1), // decSigma
_mm_setr_epi8( 2, 3,-1,-1, 4,-1,-1, 5,-1,-1, 0,-1,-1, 1,-1,-1), // decTau
_mm_setr_epi8( 3, 3,-1,-1, 3, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8( 0, 0,82,82,99,-91, 0,-58, 0, 0, 0, 0, 0, 0, 0, 0), // const A
//_mm_setr_epi8(-13, 0, 1, 2, 3, 4, 5,-13,-13,-13,-13,-13,-13,-13,-13,-13), // mergeXE
14, 16},
{ // -- n=7 -------------
// _mm_setr_epi8( 0, 4, 3, 6, 2, 5, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 0, 6, 4, 2, 1, 5, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8( 1, 7, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 0,-1,-1,-1, 2, 4,-1,-1,-1, 5, 3,-1,-1,-1, 1, 6), // encSigma
_mm_setr_epi8( 0,-1, 2, 3, 4, 5, 6, 1,-1,-1,-1,-1,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8( 7, 7, 7, 7, 7, 7, 7,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 0, 5,-1,-1, 1,-1,-1, 2,-1,-1, 4, 6,-1,-1, 3,-1), // decSigma
_mm_setr_epi8( 0, 5,-1,-1, 1,-1,-1, 2,-1,-1, 4,-1,-1, 6, 3,-1), // decTau
_mm_setr_epi8( 3,-1, 3,-1, 3,-1, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8( 0,82, 0, 0,99,99,-91,-58, 0, 0, 0, 0, 0, 0, 0, 0), // const A
//_mm_setr_epi8(0, 1, 2, 3, 4, 5, 6,-13,-13,-13,-13,-13,-13,-13,-13,-13), // mergeXE
14, 16},
{ // -- n=8 -------------
// _mm_setr_epi8( 0, 1, 4, 5, 2, 3, 6, 7,-1,-1,-1,-1,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 0, 1, 4, 5, 2, 3, 6, 7,-1,-1,-1,-1,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7,-1,-1,-1,-1,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8(-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 0,-1,-1, 7, 2, 1,-1,-1,-1, 3, 4,-1,-1,-1, 6, 5), // encSigma
_mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7,-1,-1,-1,-1,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8(-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 0, 3,-1,-1, 2,-1,-1, 5,-1,-1, 4, 7,-1, 1, 6,-1), // decSigma
_mm_setr_epi8( 0, 3,-1,-1, 2,-1,-1, 5,-1,-1, 4, 7,-1, 1, 6,-1), // decTau
_mm_setr_epi8( 3, 3, 3, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), // const A
//_mm_setr_epi8(1, 2, 3, 4, 5, 6, 7,-13,-13,-13,-13,-13,-13,-13,-13, 0), // mergeXE
12, 16},
{ // -- n=9 -------------
// _mm_setr_epi8( 2, 4, 1, 5, 3, 6, 7, 8, 0,-1,-1,-1,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 8, 2, 0, 4, 1, 3, 5, 6, 7,-1,-1,-1,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 1,-1, 1,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8( 8, 8, 8, 8,-1,-1,-1,-1,11,-1,-1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 2,-1,-1, 8, 3, 4,-1, 4, 0, 6, 1,-1,-1, 4, 7, 5), // encSigma
_mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8,-1, 8,-1,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8( 9, 9,11, 9,-1,-1,-1,-1, 1,-1,-1,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 8, 3, 7,-1, 1,-1,-1, 4, 7,-1, 0, 6,-1, 2, 5,-1), // decSigma
_mm_setr_epi8( 8, 3,-1,-1, 1,-1,-1, 4, 7,-1, 0, 6,-1, 2, 5,-1), // decTau
_mm_setr_epi8( 3,-1, 3,-1,-1,-1,-1,-1, 3,-1,-1,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8(99,99,99,99, 0, 0, 0, 0, 0,82, 0,82, 0, 0, 0, 0), // const A
//_mm_setr_epi8(2, 3, 4, 5, 6, 7, 8,-13,-13,-13,-13,-13,-13,-13, 0, 1), // mergeXE
12, 16},
{ // -- n=10 -------------
// _mm_setr_epi8( 7, 4, 1, 3, 0, 2, 5, 9, 6, 8,-1,-1,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 4, 2, 5, 3, 1, 6, 8, 0, 9, 7,-1,-1,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8( 2, 3,10,11,-1,-1,-1,-1, 2, 3,-1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 7,-1,-1, 9, 0, 4,-1,-1, 6, 2, 1,-1,-1, 8, 5, 3), // encSigma
_mm_setr_epi8( 0, 1,-1,-1, 4, 5, 6, 7, 2, 3, 8, 9,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8( 8, 9, 8, 9,-1,-1,-1,-1,10,11,-1,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 4, 6, 9,-1, 1, 3,-1,-1, 5,-1,-1, 0,-1, 2, 8, 7), // decSigma
_mm_setr_epi8( 4, 6,-1,-1, 1, 7,-1, 3, 9,-1, 5, 0,-1, 2, 8,-1), // decTau
_mm_setr_epi8( 3, 3,-1,-1,-1,-1,-1,-1, 3, 3,-1,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8( 0, 0,82,82, 0, 0, 0, 0,99,-91, 0,-58, 0, 0, 0, 0), // const A
//_mm_setr_epi8(3, 4, 5, 6, 7, 8, 9,-13,-13,-13,-13,-13,-13, 0, 1, 2), // mergeXE
12, 14},
{ // -- n=11 -------------
// _mm_setr_epi8( 5, 7, 9, 8, 1, 0, 2, 4, 6, 3,10,-1,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 5, 4, 6, 9, 7, 0, 8, 1, 3, 2,10,-1,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,-1,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8( 1,11, 1, 1,-1,-1,-1,-1, 1, 1, 1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 5,-1,10, 4, 1, 7,-1,-1, 6, 0, 9,-1,-1, 3, 2, 8), // encSigma
_mm_setr_epi8( 0,-1, 2, 3, 4, 5, 6, 7, 8, 9,10, 1,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8(11,11,11,11,-1,-1,-1,-1,11,11,11,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 5, 0,10,-1, 7, 2,-1, 9, 3,-1, 6, 1,-1,-1, 8, 4), // decSigma
_mm_setr_epi8( 5, 0,10,-1, 7, 2,-1, 9, 3,-1, 6, 1,-1, 4, 8,-1), // decTau
_mm_setr_epi8( 3,-1, 3,-1,-1,-1,-1,-1, 3,-1, 3,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8( 0,82, 0, 0, 0, 0, 0, 0,99,99,-91,-58, 0, 0, 0, 0), // const A
//_mm_setr_epi8(4, 5, 6, 7, 8, 9,10,-13,-13,-13,-13,-13, 0, 1, 2, 3), // mergeXE
12, 14},
{ // -- n=12 -------------
// _mm_setr_epi8( 0, 5, 6,10, 4, 9, 2,11, 8, 1, 3, 7,-1,-1,-1,-1), // encPi
// _mm_setr_epi8( 0, 9, 6,10, 4, 1, 2,11, 8, 5, 3, 7,-1,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,-1,-1,-1,-1), // encAlpha
_mm_setr_epi8(-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // encBeta
_mm_setr_epi8( 0,-1, 3,11, 4, 5,-1, 7, 8, 9, 6,-1,-1, 1, 2,10), // encSigma
_mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,-1,-1,-1,-1), // decAlpha
_mm_setr_epi8(-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // decBeta
_mm_setr_epi8( 0, 1, 3,-1, 4, 5,-1,10, 8,-1, 6,11,-1, 9, 2, 7), // decSigma
_mm_setr_epi8( 0, 1, 3,-1, 4, 5,-1,10, 8,-1, 6,11,-1, 9, 2, 7), // decTau
_mm_setr_epi8( 3, 3, 3, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1), // Rho
_mm_setr_epi8( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), // const A
//_mm_setr_epi8(5, 6, 7, 8, 9,10,11,-13,-13,-13,-13, 0, 1, 2, 3, 4), // mergeXE
12, 14},
{ // -- n=13 -------------
// _mm_setr_epi8( 5, 7, 8, 1, 2, 9, 4,10,11, 0, 6,12, 3,-1,-1,-1), // encPi
// _mm_setr_epi8( 9, 3, 4,12, 6, 0,10, 1, 2, 5, 7, 8,11,-1,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, 1,-1, 1), // encAlpha
_mm_setr_epi8(12,12,12,12, 8, 9,10,11,-1,-1,-1,-1,15,-1,-1,-1), // encBeta
_mm_setr_epi8( 5, 7, 6,10, 2, 7,-1,12,11, 9, 8, 7, 3, 0, 4, 1), // encSigma
_mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,-1,12,-1), // decAlpha
_mm_setr_epi8(13,13,15,13, 8, 9,10,11,-1,-1,-1,-1, 1,-1,-1,-1), // decBeta
_mm_setr_epi8( 9, 0, 7,-1, 6, 5,11,12, 2,-1, 4, 1,11, 3,10, 8), // decSigma
_mm_setr_epi8( 9, 0, 7,-1, 6, 5,-1,12, 2,-1, 4, 1,11, 3,10, 8), // decTau
_mm_setr_epi8( 3,-1, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1, 3,-1,-1,-1), // Rho
_mm_setr_epi8(99,99,99,99, 0, 0, 0, 0, 0, 0, 0, 0, 0,82, 0,82), // const A
//_mm_setr_epi8(6, 7, 8, 9,10,11,12,-13,-13,-13, 0, 1, 2, 3, 4, 5), // mergeXE
12, 14},
{ // -- n=14 -------------
// _mm_setr_epi8( 4,11, 8, 3, 5, 1, 9, 2, 6, 0,10,13, 7,12,-1,-1), // encPi
// _mm_setr_epi8( 9, 5, 7, 3, 0, 4, 8,12, 2, 6,10, 1,13,11,-1,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,-1,-1), // encAlpha
_mm_setr_epi8( 2, 3,14,15,-1,-1,-1,-1,-1,-1,-1,-1, 2, 3,-1,-1), // encBeta
_mm_setr_epi8( 4,12,10, 2, 5,11,-1,13, 6, 1, 8,-1, 7, 0, 9, 3), // encSigma
_mm_setr_epi8( 0, 1,-1,-1, 4, 5, 6, 7, 8, 9,10,11, 2, 3,12,13), // decAlpha
_mm_setr_epi8(12,13,12,13,-1,-1,-1,-1,-1,-1,-1,-1,14,15,-1,-1), // decBeta
_mm_setr_epi8( 9, 4,10,11, 0, 6,13,-1, 2, 3,-1,12, 7, 5, 8, 1), // decSigma
_mm_setr_epi8( 9, 4,10,-1, 0, 6,-1, 3, 2,11, 7,12,13, 5, 8, 1), // decTau
_mm_setr_epi8( 3, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 3, 3,-1,-1), // Rho
_mm_setr_epi8( 0, 0,82,82, 0, 0, 0, 0, 0, 0, 0, 0,99,-91, 0,-58), // const A
//_mm_setr_epi8(7, 8, 9,10,11,12,13,-13,-13, 0, 1, 2, 3, 4, 5, 6), // mergeXE
12, 14},
{ // -- n=15 -------------
// _mm_setr_epi8( 4, 8,12, 7, 5, 9,13, 2, 6,10, 1, 3,14, 0,11,-1), // encPi
// _mm_setr_epi8(13,10, 7,11, 0, 4, 8, 3, 1, 5, 9,14, 2, 6,12,-1), // decPi
// _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,-1), // encAlpha
_mm_setr_epi8( 1,15, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1,-1), // encBeta
_mm_setr_epi8( 4, 0, 1, 2, 5, 8,11, 3, 6, 9,12,-1,14,10,13, 7), // encSigma
_mm_setr_epi8( 0,-1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14, 1), // decAlpha
_mm_setr_epi8(15,15,15,15,-1,-1,-1,-1,-1,-1,-1,-1,15,15,15,-1), // decBeta
_mm_setr_epi8(13, 4, 9,10, 0, 5,12,11, 1, 6, 7, 3, 2,-1, 8,14), // decSigma
_mm_setr_epi8(13, 4, 9,-1, 0, 5,12,11, 1, 6, 7, 3, 2,10, 8,14), // decTau
_mm_setr_epi8( 3,-1, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1, 3,-1, 3,-1), // Rho
_mm_setr_epi8( 0,82, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,99,99,-91,-58), // const A
//_mm_setr_epi8(8, 9,10,11,12,13,14,-13, 0, 1, 2, 3, 4, 5, 6, 7), // mergeXE
12, 12}

};


// -----------------------------------------------------------
// ALF-n-t
// -----------------------------------------------------------
struct ALFnt;
typedef void (ALFnt::*alfnt_encdec_ft)(uint8_t*, uint8_t*);

struct ALFnt
{
	__m128i RK[28];
	__m128i B, M, maskN, mergeXE;
	const ALF_Profile* F;
	alfnt_encdec_ft enc_fptr, dec_fptr;
	__m192i Qmax;
	uint64_t m1, m0;
	int n, t, Rounds;
	int is_binary;

	// -----------------------------------
	// internal helper functions
	// -----------------------------------
	template<int tnz>
	inline int _cmpgt_maxX(__m128i X, __m128i E)
	{
		__m128i G;
		if (tnz)
#if HAS_AVX512==1
			G = _mm_permutex2var_epi8(X, mergeXE, E);
#else
			G = xor2(shuffle(X, mergeXE), _mm_slli_epi64(E, 32));
#endif
		else
			G = shuffle(X, mergeXE);

		uint64_t v = _mm_cvtsi128_si64x(G);
		if (v < m1) return 0;
		if (v > m1) return 1;
		v = _mm_extract_epi64(G, 1);
		if (v > m0) return 1;
		return 0;
	}

	inline __m128i _UpdateE(__m128i E, __m128i U, __m128i M)
	{
		U = _mm_clmulepi64_si128(U, _mm_set1_epi64x(0x01010101), 0x00);
#if HAS_AVX512==1
		return _mm_ternarylogic_epi32(E, U, M, 0x28); // 0x28: (E+U)&M
#else
		return _mm_and_si128(xor2(E, U), M);
#endif
	}

	inline __m128i _SRF(__m128i X, __m128i Tau, __m128i endXor = c_00)
	{
		return _mm_aesdeclast_si128(shuffle(X, Tau), endXor);
	}

	// -----------------------------------
	// templates for Encryption/Decryption 
	// -----------------------------------
	
	// Non-binary, t=0
	void _int_Encrypt_00(uint8_t* out /* [16] */, uint8_t* in /* [16] */)
	{
		__m128i X = load128(in);
		for (int r = 0; r < Rounds; r += 2)
			do {
				X = aesenc(shuffle(X, F->encSigma), RK[r]);
				X = xor2(X, shuffle(X, F->encBeta));
				X = aesenc(shuffle(X, F->encSigma), RK[r + 1]);
				X = xor2(X, shuffle(X, F->encBeta));
			} while (_cmpgt_maxX<0>(X, c_00));
		X = _mm_blendv_epi8(load128(out), X, maskN);
		store128(out, X);
	}

	void _int_Decrypt_00(uint8_t* out /* [16] */, uint8_t* in /* [16] */)
	{
		__m128i Y, X = load128(in);
		X = _mm_aesenclast_si128(shuffle(X, F->encSigma), c_00);
		for (int r = Rounds - 2; r >= 0; r -= 2)
			do {
				X = aesdec(shuffle(X, F->decSigma), RK[r + 1]);
				X = xor2(X, shuffle(X, F->decBeta));
				X = aesdec(shuffle(X, F->decSigma), RK[r]);
				X = xor2(X, shuffle(X, F->decBeta));
				Y = _SRF(X, F->decTau);
			} while (_cmpgt_maxX<0>(Y, c_00));
		X = _mm_blendv_epi8(load128(out), Y, maskN);
		store128(out, X);
	}

	// Non-binary, t>0 -- Generic case that covers all other cases
	void _int_Encrypt_01(uint8_t* out /* [16] */, uint8_t* in /* [16] */)
	{
		__m128i U, E, X = load128(in);
		E = _mm_and_si128(_mm_bslli_si128(load128(in + n), 3), M);
		for (int r = 0; r < Rounds; r += 2)
			do {
				U = aesenc(shuffle(X, F->encSigma), RK[r]);
				X = xor3(U, shuffle(U, F->encBeta), shuffle(E, F->Rho));
				E = _UpdateE(E, U, M);
				U = aesenc(shuffle(X, F->encSigma), RK[r + 1]);
				X = xor3(U, shuffle(U, F->encBeta), shuffle(E, F->Rho));
				E = _UpdateE(E, U, M);
			} while (_cmpgt_maxX<1>(X, E));
		X = _mm_blendv_epi8(load128(out), X, maskN);
		store128(out, X);
		out[n] = _mm_extract_epi8(E, 3);
	}

	void _int_Decrypt_01(uint8_t* out /* [16] */, uint8_t* in /* [16] */)
	{
		__m128i E, Y, X = load128(in);
		X = _mm_aesenclast_si128(shuffle(X, F->encSigma), c_00);
		E = _mm_bslli_si128(load128(in + n), 3);
		for (int r = Rounds - 2; r >= 0; r -= 2)
			do {
				X = aesdec(shuffle(X, F->decSigma), c_00);
				E = _UpdateE(xor2(E, B), X, M);
				X = xor2(X, RK[r + 1]);
				X = xor3(X, shuffle(X, F->decBeta), shuffle(E, F->Rho));
				X = aesdec(shuffle(X, F->decSigma), c_00);
				E = _UpdateE(xor2(E, B), X, M);
				X = xor2(X, RK[r]);
				X = xor3(X, shuffle(X, F->decBeta), shuffle(E, F->Rho));
				Y = _SRF(X, F->decTau);
			} while (_cmpgt_maxX<1>(Y, E));
		X = _mm_blendv_epi8(load128(out), Y, maskN);
		store128(out, X);
		out[n] = _mm_extract_epi8(E, 3);
	}

	// Binary, t=0
	void _int_Encrypt_10(uint8_t* out /* [16] */, uint8_t* in /* [16] */)
	{
		__m128i U, X = shuffle(load128(in), F->encSigma);
		__m128i encBetaSigma = combine(F->encBeta, F->encSigma);
		for (int r = 0; r < Rounds - 2; r += 2)
		{
			U = aesenc(X, RK[r]);
			X = xor2(shuffle(U, F->encSigma), shuffle(U, encBetaSigma));
			U = aesenc(X, RK[r + 1]);
			X = xor2(shuffle(U, F->encSigma), shuffle(U, encBetaSigma));
		}
		U = aesenc(X, RK[Rounds - 2]);
		X = xor2(shuffle(U, F->encSigma), shuffle(U, encBetaSigma));
		U = aesenc(X, RK[Rounds - 1]);
		X = xor2(U, shuffle(U, F->encBeta));
		X = _mm_blendv_epi8(load128(out), X, maskN);
		store128(out, X);
	}

	void _int_Decrypt_10(uint8_t* out /* [16] */, uint8_t* in /* [16] */)
	{
		__m128i Y, X = load128(in);
		__m128i decBetaSigma = combine(F->decBeta, F->decSigma);
		X = _mm_aesenclast_si128(shuffle(X, F->encSigma), c_00);
		X = shuffle(X, F->decSigma);
		for (int r = Rounds - 1; r > 1; r -= 2)
		{
			X = aesdec(X, RK[r]);
			X = xor2(shuffle(X, F->decSigma), shuffle(X, decBetaSigma));
			X = aesdec(X, RK[r - 1]);
			X = xor2(shuffle(X, F->decSigma), shuffle(X, decBetaSigma));
		}
		X = aesdec(X, RK[1]);
		X = xor2(shuffle(X, F->decSigma), shuffle(X, decBetaSigma));
		X = aesdec(X, RK[0]);
		X = xor2(X, shuffle(X, F->decBeta));
		Y = _mm_andnot_si128(maskN, xor2(load128(out), _mm_set1_epi8(0x52)));
		X = _SRF(X, F->decTau, Y);
		store128(out, X);
	}

	// Binary, t>0
	void _int_Encrypt_11(uint8_t* out /* [16] */, uint8_t* in /* [16] */)
	{
		__m128i U, E, X = shuffle(load128(in), F->encSigma);
		__m128i encBetaSigma = combine(F->encBeta, F->encSigma);
		__m128i encRhoSigma = combine(F->Rho, F->encSigma);
		E = _mm_and_si128(_mm_bslli_si128(load128(in + n), 3), M);
		for (int r = 0; r < Rounds - 2; r += 2)
		{
			U = aesenc(X, RK[r]);
			X = xor3(shuffle(U, F->encSigma), shuffle(U, encBetaSigma), shuffle(E, encRhoSigma));
			E = _UpdateE(E, U, M);
			U = aesenc(X, RK[r + 1]);
			X = xor3(shuffle(U, F->encSigma), shuffle(U, encBetaSigma), shuffle(E, encRhoSigma));
			E = _UpdateE(E, U, M);
		}
		U = aesenc(X, RK[Rounds - 2]);
		X = xor3(shuffle(U, F->encSigma), shuffle(U, encBetaSigma), shuffle(E, encRhoSigma));
		E = _UpdateE(E, U, M);
		U = aesenc(X, RK[Rounds - 1]);
		X = xor3(U, shuffle(U, F->encBeta), shuffle(E, F->Rho));
		E = _UpdateE(E, U, M);
		X = _mm_blendv_epi8(load128(out), X, maskN);
		store128(out, X);
		out[n] = _mm_extract_epi8(E, 3);
	}

	void _int_Decrypt_11(uint8_t* out /* [16] */, uint8_t* in /* [16] */)
	{
		__m128i E, Y, X = load128(in);
		__m128i decBetaSigma = combine(F->decBeta, F->decSigma);
		__m128i decRhoSigma = combine(F->Rho, F->decSigma);
		X = _mm_aesenclast_si128(shuffle(X, F->encSigma), c_00);
		E = _mm_bslli_si128(load128(in + n), 3);
		X = shuffle(X, F->decSigma);
		for (int r = Rounds - 1; r > 1; r -= 2)
		{
			X = aesdec(X, c_00);
			E = _UpdateE(xor2(E, B), X, M);
			X = xor2(X, RK[r]);
			X = xor3(shuffle(X, F->decSigma), shuffle(X, decBetaSigma), shuffle(E, decRhoSigma));
			X = aesdec(X, c_00);
			E = _UpdateE(xor2(E, B), X, M);
			X = xor2(X, RK[r - 1]);
			X = xor3(shuffle(X, F->decSigma), shuffle(X, decBetaSigma), shuffle(E, decRhoSigma));
		}
		X = aesdec(X, c_00);
		E = _UpdateE(xor2(E, B), X, M);
		X = xor2(X, RK[1]);
		X = xor3(shuffle(X, F->decSigma), shuffle(X, decBetaSigma), shuffle(E, decRhoSigma));
		X = aesdec(X, c_00);
		E = _UpdateE(xor2(E, B), X, M);
		X = xor2(X, RK[0]);
		X = xor3(X, shuffle(X, F->decBeta), shuffle(E, F->Rho));
		Y = _mm_andnot_si128(maskN, xor2(load128(out), _mm_set1_epi8(0x52)));
		X = _SRF(X, F->decTau, Y);
		store128(out, X);
		out[n] = _mm_extract_epi8(E, 3);
	}



	// -----------------------------------
	// Public API
	// -----------------------------------
	void EngineInit(__m192i _Qmax, int bit_width = 0)
	{
		// determine the case (n,t,is_binary)
		if (bit_width) t = bit_width;
		else t = _Qmax.bitwidth();
		if (t < 16 || t > 127)
			exit(-1);	// Critical error! Use case is only for [16..127] bits
		is_binary = t == _Qmax.popcnt();
		n = t >> 3;
		t &= 7;

		// profile (precomputed vectors), and extra control vectors
		F = _AlfProfile + n - 2;
		Rounds = t ? F->Rounds1 : F->Rounds0;
		M = _mm_setr_epi32(((1 << t) - 1) << 24, 0, 0, 0);
		B = ((n & 3) == 3) ? _mm_set1_epi8(0x52) : c_00;
		const uint8_t q = 0xf3, tmp[32] = { q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
		mergeXE = _mm_alignr_epi8(_mm_set1_epi8(q), load128(tmp + n), 1);
		mergeXE = _mm_alignr_epi8(mergeXE, mergeXE, 8);	// make the higher 64 bits into LSB
		const uint8_t z = -1, tmp0[32] = { z,z,z,z,z,z,z,z,z,z,z,z,z,z,z,z,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
		maskN = load128(tmp0 + 16 - n);

		// manage Qmax
		Qmax = _Qmax;

		if (n >= 7)
		{
			m1 = *(uint64_t*)(((uint8_t*)_Qmax.u) + n - 7);
			m0 = _Qmax.u[0] << ((15 - n) << 3);
		}
		else
		{
			m1 = _Qmax.u[0] << ((7 - n) << 3);
			m0 = 0;
		}

		// select enc/dec instances depending on the parameters/case
		if (is_binary)
			if (t)	// Generic case, covers all other cases but less efficient
				enc_fptr = &ALFnt::_int_Encrypt_11, 
				dec_fptr = &ALFnt::_int_Decrypt_11;
			else 
				enc_fptr = &ALFnt::_int_Encrypt_10,
				dec_fptr = &ALFnt::_int_Decrypt_10;
		else
			if(t) 
				enc_fptr = &ALFnt::_int_Encrypt_01,
				dec_fptr = &ALFnt::_int_Decrypt_01;
			else 
				enc_fptr = &ALFnt::_int_Encrypt_00,
				dec_fptr = &ALFnt::_int_Decrypt_00;
	}

	// Stand-alone use case
	void KeyInit(KTM& ktm_out, uint8_t key[16], uint64_t AppID)
	{
		ktm_out.KeyInit(key, AppID, Qmax);
	}

	// General use case
	void TweakInit(KTM& ktm_in, uint8_t tweak[16])
	{
		int bytes = n * Rounds;
		ktm_in.TweakInit(RK, (bytes + 15) >> 4, 0x1, tweak, 0, NULL);
		uint8_t* u = (uint8_t*)RK + n * (Rounds - 1);
		for (int r = Rounds - 1; r >= 0; r -= 2, u -= 2 * n)
		{
			RK[r] = _mm_and_si128(load128(u), maskN);
			RK[r - 1] = _mm_and_si128(load128(u - n), maskN);
		}
	}

	void PrepareDecrypt(void)
	{
		__m128i keyBetaAlpha = combine(F->encBeta, F->decAlpha);
		for (int r = 0; r < Rounds; r += 2)
		{
			RK[r + 0] = _mm_aesimc_si128(xor3(shuffle(RK[r + 0], keyBetaAlpha), shuffle(RK[r + 0], F->decAlpha), F->A));
			RK[r + 1] = _mm_aesimc_si128(xor3(shuffle(RK[r + 1], keyBetaAlpha), shuffle(RK[r + 1], F->decAlpha), F->A));
		}
	}

	inline void Encrypt(uint8_t* out /* [16] */, uint8_t* in /* [16] */)
	{
		(this->*enc_fptr)(out, in);
	}

	inline void Decrypt(uint8_t* out /* [16] */, uint8_t* in /* [16] */)
	{
		(this->*dec_fptr)(out, in);
	}

};


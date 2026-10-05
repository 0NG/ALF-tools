# Materials for ALF.

## Compile

Make sure that you have CMake, clang++, and git available in your environment. 
```
git clone https://github.com/jarro2783/cxxopts.git ./src/3rd/cxxopts

git clone https://github.com/google/or-tools.git ./src/3rd/or-tools

mkdir build
cd build
CC=clang CXX=clang++ cmake ..
make search_diff
make search_linear
make search_integral
```

To compile the reference code, run
```
cd refcode
g++ app.cpp -o app -march=native -O3
./app
```

## Run

`search_diff`, `search_linear`, and `search_integral` are commandline tools and have the same inputs, except that `search_integral` doesn't need the number of rounds. The common arguments are:
1. `--nsize`: the value of `n`
2. `--tsize`: the value of `t`
3. `-n`: number of rounds
4. `--sigma`: the sigma permutation
5. `--alpha`: the alpha permuatation, optional, the tools take the ones in the paper as defaults
6. `--beta`: the beta permutation, optional, the tools take the ones in the paper as defaults
7. `--help`: the help manual to see the meaning of these cmd arguments.

## Update

2026-10-02. We thank Gustav Åkesson for verifying the implementation with AI tools, that lead to the following bug fixes:

- Fixed one major bug that was not possible to catch with existing test vectors:
  - The case when N>1 with the same modulus – in this case Q must be equal to q and not q^lambda. This was fixed and, as a consequence, the set of test vectors [S|C*] have been recreated. The test vectors [T|D*] are not affected.
- Fixed two platform-dependent bugs that were occurring on specific combination of a platform and compiler, these bugs were possible to catch while testing the library against known test vectors:
  - __builtin_clzll(x) is undefined when x=0 and in certain builds can produce a random value. Fixed by adding the check if x=0 before calling clzll-function.
  - __m256i Q;... followed by x= ((uint16_t*)&Q)[k], does not return the correct value on some platforms. Multiple locations where &Q (and similar) was used are rewritten to first drop the 256-bit value into a buffer then reading/operating on that.
- Fixed three minor warnings catched by AI tools:
  - ALF0::PrepareDecrypt copies the whole 256-byte buffer while bytes with the index>Qmax are not defined. This has no impact on the functionality but may trigger a warning for an external analysis tool. Fixed by replacing to memcpy(…, Qmax+1).
  - ALFnt loads the t-bits into the 3rd byte of the register E while reading “unused” bytes outside of the input buffer. This normally does not have an impact since it is unlikely to read from the heap area not designated to the whole running process. However, this was also fixed to make sure we only read the area within the given buffer.
  - ALFnt::EngineInit with n=7 would shift a 64-bit value by 64 bits to the left, which is undefined. This has no impact on functionality since the verified value is always zero in the lower 64 bits, which in turn is always <= than any other undefined value of m0, thus always returning the correct answer. Fixed by checking the case n=7 and forcing m0=0.

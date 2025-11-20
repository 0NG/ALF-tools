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

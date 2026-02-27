<p align="center">
	<img src="docs/tgen_white.svg" alt="tgen" width="300" onerror="this.onerror=null;this.src='./tgen_white.svg';">
</p>

<p align="center">
    <em>Testcase generation for random inputs.</em>
</p>

<p align="center">
	<a href="https://opensource.org/license/mit" target="_blank" rel="noopener noreferrer nofollow" style="display:inline-block"><img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="MIT License" style="max-width: 100%;"></a>
	<a href="https://github.com/brunomaletta/tgen/actions/workflows/static.yml" target="_blank" rel="noopener noreferrer nofollow" style="display:inline-block"><img src="https://img.shields.io/github/actions/workflow/status/brunomaletta/tgen/static.yml?label=gtest" alt="Tests" style="max-width: 100%;"></a>
	<a href="https://github.com/brunomaletta/tgen/blob/main/src/tgen.h" target="_blank" rel="noopener noreferrer nofollow" style="display:inline-block"><img src="https://img.shields.io/badge/C%2B%2B-17-blue" alt="C++17" style="max-width: 100%;"></a>
	<a href="https://brunomaletta.github.io/tgen/index.html" target="_blank" rel="noopener noreferrer nofollow" style="display:inline-block"><img src="https://img.shields.io/badge/doxygen-2C4AA8?style=default&logo=doxygen&logoColor=white" alt="Documentation" style="max-width: 100%;"></a>
</p>

## 🎯 Overview

<img src="docs/tgen_white.svg" alt="tgen" height="24"
 style="vertical-align:-0.15em; margin-right:0.35em;" onerror="this.onerror=null;this.src='./tgen_white.svg';">
is a C++ header for writing random testcase generators quickly and safely.

Instead of manually coding ad-hoc generators, use powerful algorithmic machinery to guarantee simple, correct and uniform generation.

There is support for arrays, permutations, maths, and more.

## ⚡ Quick examples

<img src="docs/tgen_white.svg" alt="tgen" height="24" onerror="this.onerror=null;this.src='./tgen_white.svg';"> provides two complementary styles:

- **Operations**: sample and operate on data directly.

```cpp
// Generates random prime in [1, 1e6]
std::cout << tgen::math::gen_prime(1, 1e6) << std::endl;
```

- **Generators**: describe constraints and sample random instances.

```cpp
// Generates 20 distincts two-digit numbers.
std::cout << tgen::sequence<int>(20, 10, 99).distinct().gen() << std::endl;
```

No loops. No backtracking. No custom generator code.

## ⚖️ Why not [testlib](https://github.com/MikeMirzayanov/testlib) / [jngen](https://github.com/ifsmirnov/jngen)?

<img src="docs/tgen_white.svg" alt="tgen" height="24" onerror="this.onerror=null;this.src='./tgen_white.svg';"> works in a similar way as traditional generators, but has support for declarative generation, worst-case sampling, and many useful and powerful helpers.

## 📦 Instalation

Header-only. Download

```bash
wget https://raw.githubusercontent.com/brunomaletta/tgen/main/src/tgen.h
```

and include

```cpp
#include "tgen.h"
```

## 🎲 Generation

Similar to traditional generators, there is registration and opts (arguments).

```cpp
int main(int argc, char** argv) {
    tgen::register_gen(argc, argv);
	int n = tgen::opt<int>("n");
	std::cout << tgen::permutation(tgen::next(1, n)).gen().add_1() << std::endl;
}
``` 

We can run with `n=100` by calling `./gen -n 100`.

## 📚 Documentation

<a href="https://brunomaletta.github.io/tgen/index.html">
<img src="https://img.shields.io/badge/📚%20Documentation-Read%20here-informational?style=for-the-badge" />
</a>


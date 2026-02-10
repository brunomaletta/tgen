![gtest](https://img.shields.io/github/actions/workflow/status/brunomaletta/tgen/static.yml?label=gtest)
<img src="https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white" height="20">
<img src="https://img.shields.io/badge/doxygen-2C4AA8?style=for-the-badge&logo=doxygen&logoColor=white" height="20">

# Overview

`tgen` is a `C++` library to help you generate random stuff, useful for testcase generation (such as [jngen](https://github.com/ifsmirnov/jngen) or [testlib](https://github.com/MikeMirzayanov/testlib)). The code is in a single file [tgen.h](https://github.com/brunomaletta/tgen/blob/main/src/tgen.h), that should be added to your directory.

```cpp
#include "tgen.h"
```

The first thing is to [register the generator](https://brunomaletta.github.io/tgen/group__opts.html). That defines the seed for random generation and parses the opts.

There are [general operations](https://brunomaletta.github.io/tgen/group__general.html) and operations for specific data types:

- [Sequences](https://brunomaletta.github.io/tgen/group__sequence.html)

### Type generators and instances

All types specified above define a **generator**, that when called upon will generate a uniformly random **instance** with the given constraints. Let's see an example with `tgen::sequence`:

```cpp
tgen::sequence<int> seq_gen = tgen::sequence<int>(/*size=*/10, /*value_l=*/1, /*value_r=*/100);
```

This will create a sequence generator representing the set of all sequences with 10 values from 1 to 100.

Every generator of type `GEN` has a method `gen()`, that returns a `GEN::instance` representing an element chosen uniformly at random from the set of all valid elements from the current state of the generator. A `GEN::instance` can be fed to `std::cout` to be printed.

In our example, we can call `gen()` to generate and print a random sequence of 10 elements from 1 to 100.

```cpp
std::cout <<
	seq_gen.gen() << std::endl;
```

The nice thing is that we can add restrictions (specific to each type) to the generator, shrinking the set of valid arrays. For example, we can add the restriction that the first and second elements of the sequence have to be the same.

```cpp
tgen::sequence<int>::instance inst = seq_gen.equal(/*idx_1=*/0, /*idx_2=*/1).gen();
```

The returned instance can also be modified by some deterministic operations (specific to each type).

```cpp
inst.reverse();
```

Finally, there can be random operations defined for the type instance.

```cpp
std::cout <<
	tgen::sequence_op::any(inst) << std::endl;
```

Combining everything into one line:

```cpp
std::cout << tgen::sequence_op::any(
	tgen::sequence<int>(10, 1, 100).equal(0, 1).gen()
	.reverse()
) << std::endl;
```

## Examples

### Opts configuration

```cpp
#include "tgen.h"

#include <iostream>

int main(int argc, char** argv) {
	tgen::register_gen(argc, argv);

	int n_max = tgen::opt<int>("n");

	std::cout << tgen::next(1, n) << std::endl;
}
```

Calling this code with `./a.out -n 100` will generate a random number from 1 to 100.

### Generation

Random 20 distinct values from 1 to 100.

```cpp
std::cout <<
    tgen::sequence<int>(20, 1, 100).distinct().gen() << std::endl;
// "67 96 80 11 46 52 42 2 93 1 28 3 48 82 90 99 53 98 94 88"
```

Random Palindrome of length 7.

```cpp
auto s = tgen::sequence<int>(7, 0, 9);
for (int i = 0; i <= 2; ++i) s.equal(i, 6-i);
std::cout << s.gen() << std::endl;
// "3 1 9 6 9 1 3"
```

Random 3 runs of 4 equal numbers. Values between runs are distinct.

```cpp
std::cout <<
    tgen::sequence<int>(12, 1, 10)
    .equal_range(0, 3).equal_range(4, 7).equal_range(8, 11)
    .distinct({0, 4, 8}).gen() << std::endl;
// "3 3 3 3 2 2 2 2 9 9 9 9"
```

Random DNA sequence of length 8 with no equal adjacent values.

```cpp
auto s2 = tgen::sequence(8, {'A','C','G','T'});
for (int i = 1; i < 8; i++) s2.different(i-1, i);
std::cout << s2.gen() << std::endl;
// "T C T G T G A C"
```

Random binary sequence of length 10 with 5 1's that start with 1.

```cpp
std::cout <<
    tgen::sequence<int>(10, 0, 1)
    .set(0, 1)
    .gen_until([](const auto& inst) {
        auto vec = inst.to_std();
        return std::accumulate(vec.begin(), vec.end(), 0) == 5;
    }, 100) << std::endl;
// "1 0 0 1 0 1 1 0 1 0"
```


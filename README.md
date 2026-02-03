# Overview

`tgen` is a `C++` library to help you generate random stuff, useful for testcase generation (such as [jngen](https://github.com/ifsmirnov/jngen) or [testlib](https://github.com/MikeMirzayanov/testlib)).

The first thing is to [register the generator](https://brunomaletta.github.io/tgen/group__opts.html). That defines the seed for random generation and parses the opts.

There are [general operations](https://brunomaletta.github.io/tgen/group__general.html) and operations for specific data types:

- [Sequences](https://brunomaletta.github.io/tgen/group__sequence.html)

### Type generators and instances

All types specified above define a **generator**, that when called upon will generate a uniformly random **instance** with the given constraints. Let's see an example with `tgen::sequence`:

```cpp
auto seq_gen = tgen::sequence<int>(/*size=*/10, /*value_l=*/1, /*value_r=*/100);
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
std::sequence<int>::instance inst = seq_gen.equal_idx_pair(/*idx_1=*/0, /*idx_2=*/1).gen();
```

The returned instance can also be modified by some deterministic operations (specific to each type).

```cpp
inst.reverse();
```

Finally, there can be random operations defined for the type.

```cpp
std::cout <<
	tgen::sequence_op::any(inst) << std::endl;
```

Combining everything into one line:

```cpp
std::cout <<
	tgen::sequence_op::any(
		tgen::sequence<int>(10, 1, 100).equal_idx_pair(0, 1).gen().reverse()
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

```cpp
std::cout <<
	tgen::sequence<int>(100, 1, 100).distinct().gen() << std::endl;
```

This code generates a random sequence of 100 distinct numbers.

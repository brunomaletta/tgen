<div style="text-align:center;">
    <img id="logo" src="tgen_logo_black.svg" width="300"><br>
    <em>Testcase generation for random inputs.</em>
</div>

# Overview

@tt{tgen} is a @tt{C++} library to help you generate random stuff, useful for testcase generation (such as [jngen](https://github.com/ifsmirnov/jngen) or [testlib](https://github.com/MikeMirzayanov/testlib)). The code is in a single file [tgen.h](https://github.com/brunomaletta/tgen/blob/main/single_include/tgen.h), that should be added to your directory.

```cpp
#include "tgen.h"
```

The first thing is to [register the generator](https://brunomaletta.github.io/tgen/group__opts.html). That defines the seed for random generation and parses the opts.

There are:

- [Base operations](https://brunomaletta.github.io/tgen/group__base.html)
  - [Generator value operations](https://brunomaletta.github.io/tgen/group__generator__value__op.html)
- [Math operations](https://brunomaletta.github.io/tgen/group__math.html)
- [Hacks (adversarial generation)](https://brunomaletta.github.io/tgen/group__hack.html)
- [Miscellaneous operations](https://brunomaletta.github.io/tgen/group__misc.html)

and operations for specific data types:

- [Pairs](https://brunomaletta.github.io/tgen/group__pair.html)
- [Lists](https://brunomaletta.github.io/tgen/group__list.html)
- [Permutations](https://brunomaletta.github.io/tgen/group__permutation.html)
- [Strings](https://brunomaletta.github.io/tgen/group__str.html)
- [Trees](https://brunomaletta.github.io/tgen/group__tree.html)
- [Graphs](https://brunomaletta.github.io/tgen/group__graph.html)

### Type generators and values

All data types specified above define a **generator**, that when called upon will generate a uniformly random **value** with the given constraints. Let's see an example with @tt{tgen::list}:

```cpp
tgen::list<int> t = tgen::list<int>(/*size=*/10, /*value_l=*/1, /*value_r=*/100);
```

This will create a list generator representing the set of all lists with 10 values from 1 to 100.

Every generator of type @tt{@type{Gen}} has a method @tt{gen()}, that returns a @tt{@type{Gen}\::value} representing an element chosen uniformly at random from the set of all valid elements from the current state of the generator. A @tt{@type{Gen}\::value} can be fed to @tt{@namespace{std}\::cout} to be printed.

In our example, we can call @tt{gen()} to generate and print a random list of 10 elements from 1 to 100.

```cpp
std::cout << t.gen() << std::endl;
```

The nice thing is that we can add restrictions (specific to each type) to the generator, shrinking the set of valid lists. For example, we can add the restriction that the first and second elements of the list have to be the same.

```cpp
tgen::list<int>::value inst = t.equal(/*idx_1=*/0, /*idx_2=*/1).gen();
```

The returned value can also be modified by some deterministic operations (specific to each type).

```cpp
inst.reverse();
```

Finally, there can be random operations defined for the generator value.

```cpp
std::cout << tgen::pick(inst) << std::endl;
```

Combining everything into one line:

```cpp
std::cout << tgen::pick(
	tgen::list<int>(10, 1, 100)
	.equal(0, 1)
	.gen()
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

	std::cout << tgen::next(1, n_max) << std::endl;
}
```

Calling this code with @tt{./a.out -n 100} will generate a random number from 1 to 100.

### Generation

Random 20 distinct values from 1 to 100.

```cpp
std::cout <<
    tgen::list<int>(20, 1, 100).all_different().gen() << std::endl;
```

```
67 96 80 11 46 52 42 2 93 1 28 3 48 82 90 99 53 98 94 88
```

Random palindrome of length 7.

```cpp
std::cout << tgen::str(7, 'a', 'z').palindrome().gen() << std::endl;
```

```
iczpzci
```

Random 3 runs of 4 equal numbers. Values between runs are different.

```cpp
std::cout <<
    tgen::list<int>(12, 1, 10)
    .equal_range(0, 3).equal_range(4, 7).equal_range(8, 11)
    .different({0, 4, 8}).gen() << std::endl;
```

```
3 3 3 3 2 2 2 2 9 9 9 9
```

Random DNA sequence of length 8 with no equal adjacent values.

```cpp
auto s2 = tgen::list(8, {'A','C','G','T'});
for (int i = 1; i < 8; i++) s2.different(i-1, i);
std::cout << s2.gen() << std::endl;
```

```
T C T G T G A C
```

Random binary sequence of length 10 with 5 1's that starts with 1.

```cpp
std::cout <<
    tgen::list<int>(10, 0, 1)
    .fix(0, 1)
    .gen_until([](const auto& inst) {
        auto vec = inst.to_std();
        return std::accumulate(vec.begin(), vec.end(), 0) == 5;
    }, 100) << std::endl;
```

```
1 0 0 1 0 1 1 0 1 0
```

Random 1-based permutation of size 5 with only one cycle.

```cpp
std::cout << tgen::permutation(5).cycles({5}).gen().add_1() << std::endl;
```

```
2 5 4 1 3
```

Inverse of a random odd permutation of size 5.

```cpp
std::cout << tgen::permutation(5)
    .gen_until([](const auto &perm) { return perm.parity() == -1; }, 100)
    .inverse() << std::endl;
```

```
4 2 3 1 0
```

Random prime in @tt{[1, 1e18]}.

```cpp
std::cout << tgen::math::gen_prime(1, 1e18) << std::endl;
```

```
104297037245455381
```

Largest prime gap that fits in @type{uint64_t}.

```cpp
auto [l, r] = tgen::math::prime_gap_upto(std::numeric_limits<uint64_t>::max());
std::cout << l << " " << r << " " << r - l << std::endl;
```

```
6787988999657777798 6787988999657779306 1508
```

Random partition of 10 into 2 parts in @tt{[3, 7]}.

```cpp
std::cout << tgen::print(tgen::math::gen_partition_fixed_size(10, 2, 3, 7)) << std::endl;
```

```
6 4
```

Random numbers in @tt{[0, 1e30]}.

```cpp
std::cout << tgen::str("0 | [1-9][0-9]{0,%d} | 10{%d}", 30 - 1, 30).gen_list(3) << std::endl;
```

```
395192209976851520716904879188 507650968099964477977292350849 549612473618635975427061717252
```

Random perfect matching of @tt{K_10}.

```cpp
auto g = tgen::distinct([&]() { return tgen::next(0, 9); });
for (int i = 0; i < 5; ++i)
    std::cout << g.gen() << "," << g.gen() << " ";
```

```
9,5 3,1 0,4 7,6 8,2 
```

All primes in @tt{[1, 10]}, in order.

```cpp
std::cout << tgen::distinct(tgen::math::gen_prime, 1, 10).gen_all().sort() << std::endl;
```

```
2 3 5 7
```

5 random square numbers in @tt{[1, 1e4]}.

```cpp
std::cout << tgen::distinct([&]() {
    int x = tgen::next(1, 100);
    return x * x;
}).gen_list(5) << std::endl;
```

```
676 1936 484 3481 9604
```

Some random parenthesis sequences.

```cpp
std::cout << tgen::distinct(tgen::misc::gen_parenthesis, 6).gen_list(5) << std::endl;
```

```
()(()) (())() (()()) ((())) ()()()
```

All pairs @tt{(a, b)} in @tt{[1, 3]} with @tt{a <= b}.

```cpp
std::cout << tgen::pair<int>(1, 3).leq().distinct().gen_all().separator('|') << std::endl;
```

```
1 3|1 2|3 3|1 1|2 2|2 3
```

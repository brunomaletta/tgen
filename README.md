# A library to generate testcases

There are [general operations](group__general.html) and operations on specific data types:

- Sequences

## Examples

### Opts configuration

```cpp
#include "tgen.h"

#include <iostream>

int main(int argc, char** argv) {
	tgen::register_gen(argc, argv);

	int n_max = get<int>("n");

	std::cout << tgen::next(1, n) << std;:endl;
}
```

Calling this code with `./a.out -n 100` will generate a random number from 1 to 100.

### Generation

```cpp
std::cout << tgen::sequence<int>(100, 1, 100).distinct().gen() << std::endl;
```

This code generates a random sequence of 100 distinct numbers.

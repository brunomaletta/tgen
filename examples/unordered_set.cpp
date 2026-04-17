#include "../single_include/tgen.h"

#include <unordered_map>

const int N = 2e6;

void insert_numbers_single(long long x) {
	clock_t begin = clock();
	std::unordered_map<long long, int> numbers;

	for (int i = 1; i <= N; i++)
		numbers[i * x] = i;

	long long sum = 0;

	for (auto &entry : numbers)
		sum += (entry.first / x) * entry.second;

	printf("x = %lld: %.3lf seconds, sum = %lld\n", x,
		   (double)(clock() - begin) / CLOCKS_PER_SEC, sum);
}

void insert_numbers(std::vector<long long> v) {
	clock_t begin = clock();
	std::unordered_map<long long, long long> numbers;

	for (long long i : v)
		numbers[i] = i;

	long long sum = 0;

	for (auto &entry : numbers)
		sum += entry.first / v[0] * entry.second;

	printf("%.3lf seconds, sum = %lld\n",
		   (double)(clock() - begin) / CLOCKS_PER_SEC, sum);
}

int main(int argc, char **argv) {
	tgen::register_gen(argc, argv);
	// tgen::set_cpp_version(17);
	// tgen::set_compiler(tgen::gcc());

	std::vector<long long> vec = tgen::hack::std_unordered(N);

	insert_numbers(vec);
	for (int i = 0; i < 10; i++)
		std::cout << vec[i] << " \n"[i + 1 == 10];

	insert_numbers_single(85229);
	insert_numbers_single(107897);
	insert_numbers_single(126271);

#ifdef __GLIBCXX__
	std::cout << "Using libstdc++\n";
#endif

#ifdef _LIBCPP_VERSION
	std::cout << "Using libc++\n";
#endif
}

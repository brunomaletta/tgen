#include <cmath>
#include <gtest/gtest.h>
#include <map>
#include <type_traits>
#include <vector>

#include "../single_include/tgen.h"

// Expects an error to be thrown, and asserts
// that the error message starts with some prefix.
#define EXPECT_THROW_TGEN_PREFIX(stmt, prefix)                                 \
	EXPECT_THROW(                                                              \
		{                                                                      \
			try {                                                              \
				stmt;                                                          \
				FAIL()                                                         \
					<< "Expected std::runtime_error, but no error occurred";   \
			} catch (const std::runtime_error &e) {                            \
				std::string msg = e.what();                                    \
				std::string tgen_pref = std::string("tgen: ") + prefix;        \
				EXPECT_TRUE(msg.rfind(tgen_pref, 0) == 0)                      \
					<< "Expected message to start with: \"" << tgen_pref       \
					<< "\"\n"                                                  \
					<< "Actual message: \"" << msg << "\"";                    \
				throw e;                                                       \
			}                                                                  \
		},                                                                     \
		std::runtime_error)

// Generates 'argv', for initialization.
inline std::vector<char *> get_argv(std::initializer_list<const char *> list) {
	std::vector<char *> v;
	for (auto s : list)
		v.push_back(const_cast<char *>(s));
	v.push_back(nullptr);
	return v;
}

// Given a count of elements `counts`, checks it was generated uniformly from
// `num_tests` tests and `num_elements` total elements. If it was generated
// uniformly, the probability of the failure is at most 1e-9.
template <typename T>
bool expect_uniform(const std::map<T, int> &counts, int num_elements,
					int num_tests) {
	EXPECT_TRUE(static_cast<int>(counts.size()) <= num_elements)
		<< "Expected at most " << num_elements << " unique values, but got "
		<< counts.size();

	double expected = double(num_tests) / num_elements;

	double chi2 = 0;
	for (auto [element, count] : counts) {
		double d = count - expected;
		chi2 += d * d / expected;
	}
	chi2 += (num_elements - counts.size()) * expected;

	double df = num_elements - 1;

	double z = 6.109410; // phi(z) = 1 - 1e-9
	return chi2 <= df + z * std::sqrt(2 * df);
}

// Checks if the generator is uniform, assuming there are `num_elements`
// possible values. If the generator is uniform, the probability of the test
// failing is at most 1e-27.
// If the generator is not uniform, the test will usually fail,
// but no strict guarantee is made (it depends on how biased it is).
template <typename Gen>
void expect_generator_uniform(const Gen &gen, int num_elements) {
	int repeats = 3, count_fail = 0;
	std::map<typename Gen::value::std_type, int> example_counts;
	for (int i = 0; i < repeats; ++i) {
		long long num_tests = std::max(1000, 20 * num_elements);

		std::map<typename Gen::value::std_type, int> counts;
		for (long long j = 0; j < num_tests; ++j)
			counts[gen.gen().to_std()]++;

		if (!expect_uniform(counts, num_elements, num_tests))
			++count_fail;

		example_counts = counts;
	}
	EXPECT_TRUE(count_fail < repeats)
		<< "Distribution not uniform\nNum elements: " << num_elements
		<< "\nCounts:\n"
		<< tgen::print(example_counts);
}

// Checks if a function `func(args)` returns a random element, out of
// `num_elements` possible values. Assumes the return value of `func` has
// operator <. If the function is uniform, the probability of the test failing
// is at most 1e-27.
// If the function is not uniform, the test will usually fail,
// but no strict guarantee is made (it depends on how biased it is).
template <typename F, typename... Args>
void expect_function_uniform(F func, int num_elements, Args... args) {
	using T = std::invoke_result_t<F, Args...>;
	int repeats = 3, count_fail = 0;
	std::map<T, int> example_counts;
	for (int i = 0; i < repeats; ++i) {
		long long num_tests = std::max(1000, 20 * num_elements);

		std::map<T, int> counts;
		for (long long j = 0; j < num_tests; ++j)
			counts[func(args...)]++;

		if (!expect_uniform(counts, num_elements, num_tests)) {
			++count_fail;
			example_counts = counts;
		}
	}
	EXPECT_TRUE(count_fail < repeats)
		<< "Distribution not uniform\nNum elements: " << num_elements
		<< "\nCounts:\n"
		<< tgen::print(example_counts);
}

// Checks that `func(args...)` produces values whose proportions match the
// requested `weights`, within 6 standard deviations per category. The
// false-fail probability is below 2e-9 per category, so the test is reliable.
// `func(args...)` must return a value that can index into `weights` (typically
// an integer in `[0, weights.size())`).
template <typename F, typename... Args>
void expect_distribution(F func, const std::vector<double> &weights,
						 Args... args) {
	using T = std::invoke_result_t<F, Args...>;
	int num_tests = 100000;
	double total = std::accumulate(weights.begin(), weights.end(), 0.0);

	std::map<T, int> counts;
	for (int i = 0; i < num_tests; ++i)
		counts[func(args...)]++;

	for (size_t i = 0; i < weights.size(); ++i) {
		double p = weights[i] / total;
		double expected = num_tests * p;
		double std_dev = std::sqrt(num_tests * p * (1 - p));
		auto it = counts.find(static_cast<T>(i));
		int count = (it == counts.end()) ? 0 : it->second;
		EXPECT_LE(std::abs(count - expected), 6 * std_dev)
			<< "Index " << i
			<< " count off by too much; weights: " << tgen::print(weights)
			<< "; counts: " << tgen::print(counts);
	}
}
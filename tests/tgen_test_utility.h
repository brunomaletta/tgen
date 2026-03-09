#include <cmath>
#include <gtest/gtest.h>
#include <map>
#include <vector>

// Expects an error to be thrown, and asserts
// that the error message starts with some prefix.
#define EXPECT_THROW_TGEN_PREFIX(stmt, prefix)                                 \
	EXPECT_THROW(                                                              \
		{                                                                      \
			try {                                                              \
				stmt;                                                          \
				FAIL() << "Expected std::runtime_error, but no error ocurred"; \
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

// Checks if the generator is uniform, assuming there are `num_elements`
// possible instances. If the generator is uniform, the probability of the test
// failing is at most 1e-27.
template <typename Gen>
void check_generator_uniform(const Gen &gen, int num_elements) {
	int repeats = 3, count_fail = 0;
	for (int i = 0; i < repeats; ++i) {
		long long num_tests = std::max(1000, 20 * num_elements);

		std::map<typename Gen::instance::std_type, int> counts;

		for (long long j = 0; j < num_tests; ++j)
			counts[gen.gen().to_std()]++;

		EXPECT_TRUE(static_cast<int>(counts.size()) <= num_elements)
			<< "Expected at most " << num_elements
			<< " unique instances, but got " << counts.size();

		double expected = double(num_tests) / num_elements;

		double chi2 = 0;
		for (auto [std_instance, count] : counts) {
			double d = count - expected;
			chi2 += d * d / expected;
		}
		chi2 += (num_elements - counts.size()) * expected;

		double df = num_elements - 1;

		double z = 6.109410; // phi(z) = 1 - 1e-9
		if (chi2 > df + z * std::sqrt(2 * df))
			++count_fail;
	}
	EXPECT_TRUE(count_fail < repeats) << "Distribution not uniform";
}
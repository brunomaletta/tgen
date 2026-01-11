#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <random>

namespace tgen {


/*
 * Basic random generation.
 *
 */
struct random {

	// Variable to generate numbers.
	std::mt19937 rng;


	// Incompatible with testlib!!
	// Returns an equiprobable integer in [l, r).
	int next(int l, int r) {
		// TODO
		return 0;
	}

	// Compatible with testlib.
	// Returns an equiprobable integer in [l, r).
	void next(double l, double r) {
		// TODO
	}

	// Compatible with testlib.
	// Returns an equiprobable value from the string s.
	// For example, next("one|two|three") returns an equiprobable element from
	// {"one", "two", "three"}.
	void next(std::string s) {
		// TODO
	}

	// Compatible with testlib.
	// Returns an equiprobable element from c.
	template<typename Container>
	void any(const Container& c) {
		// TODO
	}

	// Compatible with testlib.
	// Shuffles [first, last) uniformly.
	template<typename It>
	void shuffle(It first, It last) {
		if (first == last) return;

		for (It i = first + 1; i != last; ++i)
			std::iter_swap(i, first + next(0, int(i - first) + 1));
	}

	// Compatible with testlib.
	// Returns uniformly element from [first, last).
	template<typename It>
	typename It::value_type any(const It& first, const It& last) {
		int size = std::distance(first, last);
		typename It::const_iterator it = first;
        std::advance(it, next(0, size));
        return *it;
	}

	// Sets the seed.
	void seed(const std::vector<uint32_t>& seed) {
		std::seed_seq seq(seed.begin(), seed.end());
		rng.seed(seq);
	}
};

/*
 * Global random generation.
 */
random rnd;


/*
 * Opts - options given to the generator.
 *
 * Incompatible with testlib.
 *
 * Opts are a list of either positional or named options.
 *
 * Positional options are number from 0 sequentially.
 * For example, for "10 -n=20 str" positional option 1 is the string "str".
 *
 * Named options is given in one of the following formats:
 * 1) -keyname=value or --keyname=value (ex. -n=10   , --test-count=20)
 * 2) -keyname value or --keyname value (ex. -n 10   , --test-count 20)
 */

/* INTERNAL
 * Dictionary containing the positional parsed opts.
 */
std::vector<std::string> __pos_opts;

/* INTERNAL
 * Global dictionary the named parsed opts.
 */
std::map<std::string, std::string> __named_opts;

/*
 * Returns true if there is an opt at a given index.
 */
bool has_opt(int index) {
	return 0 <= index and index < __pos_opts.size();
}

/*
 * Returns true if there is an opt with a given key.
 */
bool has_opt(const std::string& key) {
    return __named_opts.count(key) != 0;
}

/* INTERNAL
 * Parses 'value' into integral type T.
 */
 template<typename T>
 T __get_opt_integral(std::string value) {
	 // TODO
	 return T();
 }

/* INTERNAL
 * Parses 'value' into floating type T.
 */
 template<typename T>
 T __get_opt_floating(std::string value) {
	 // TODO
	 return T();
 }

/* INTERNAL
 * Parses 'value' into type T.
 */
 template<typename T>
 T __get_opt(std::string value) {
	if (std::is_integral<T>()) return __get_opt_integral<T>(value);
	if (std::is_floating_point<T>()) return __get_opt_floating<T>(value);
	throw();
 }

/* INTERNAL
 * Parses 'value' into bool.
 */
 template<>
 bool __get_opt(std::string value) {
	if (value == "true" or value == "1") return true;
	if (value == "false" or value == "0") return false;
	throw();
}

/*
 * Returns the parsed opt by a given index.
 */
template<typename T>
T opt(int index) {
    if (!has_opt(key)) throw();
    return __get_opt<T>(__pos_opts[index]);
}

/*
 * Return the parsed opt by a given index. If no opts with the index are
 * found, returns the given default_value.
 */
template<typename T>
T opt(int index, const T& default_value) {
    if (!has_opt(index)) return default_value;
    return __get_opt<T>(__pos_opts[index]);
}

/*
 * Returns the parsed opt by a given key.
 */
template<typename T>
T opt(const std::string& key) {
    if (!has_opt(key)) throw();
    return __get_opt<T>(__named_opts[key]);
}

/*
 * Returns the parsed opt by a given key. If no opts with the given key are
 * found, returns the given default_value.
 */
template<typename T>
T opt(const std::string& key, const T& default_value) {
    if (!has_opt(key)) return default_value;
    return __get_opt<T>(__named_opts[key]);
}

/* INTERNAL
 * TODO
 */
void __prepare_opts(int argc, char** argv) {
	// TODO
}

/*
 * Registers generator by initializing rnd and parsing opts.
 * Argument 'version' is unused.
 */
void register_gen(int argc, char** argv, int version = 1) {
	std::vector<uint32_t> seed;

	// Starting from 1 to ignore the name of the executable.
	for (int i = 1; i < argc; ++i) {
		// We append the number of chars, and then the list of chars.
		int size_pos = seed.size();
		seed.push_back(0);
		for (char* s = argv[i]; *s != '\0'; ++s) {
			++seed[size_pos];
			seed.push_back(*s);
		}
	}
	rnd.seed(seed);

	__prepar_oOpts(argc, argv);
}

}; // namespace tgen


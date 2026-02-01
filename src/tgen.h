#pragma once

#include <cctype>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace tgen {

/*
 * Error handling.
 */
void __throw_assertion_error(const std::string &condition,
							 const std::string &msg) {
	throw std::runtime_error("tgen: " + msg + " (assertion `" + condition +
							 "` failed).");
}
void __throw_assertion_error(const std::string &condition) {
	throw std::runtime_error("tgen: assertion `" + condition + "` failed.");
}
std::runtime_error __error(const std::string &msg) {
	return std::runtime_error("tgen: " + msg);
}

/*
 * Ensures condition is true, with nice debug.
 */
#define ensure(cond, ...)                                                      \
	if (!(cond))                                                               \
		__throw_assertion_error(#cond, __VA_ARGS__);

/*
 * Global random generation.
 */
std::mt19937 __rng;

// Fetches next integral/double
template <typename T> T __next_integral(T l, T r) {
	ensure(l <= r, "range for `next` bust be valid");
	std::uniform_int_distribution<T> dist(l, r);
	return dist(__rng);
}
double __next_double(double l, double r) {
	ensure(l <= r, "range for `next` bust be valid");
	std::uniform_real_distribution<double> dist(l, r);
	return dist(__rng);
}

// Returns a 'random' number in [l, r].
template <typename T> T next(T l, T r) {
	if (std::is_integral<T>())
		return __next_integral<T>(l, r);
	if (std::is_floating_point<T>())
		return __next_double(l, r);
	throw __error("invalid type for next (" + std::string(typeid(T).name()) +
				  ")");
}

// Returns an equiprobable value from the regex.
// For example, next("(a|b|c){10}|[a-c]{1,5}") returns either 10 chars in {'a,
// 'b', 'c'}, or from 1 to 5 'c's.
void next(std::string regex) {
	// TODO
}

// Shuffles [first, last) inplace uniformly.
template <typename It> void shuffle(It first, It last) {
	if (first == last)
		return;

	for (It i = first + 1; i != last; ++i)
		std::iter_swap(i, first + next(0, int(i - first)));
}

// Shuffles container uniformly. Returns a copy.
template <typename C> C shuffle(C container) {
	shuffle(container.begin(), container.end());
	return container;
}

// Returns uniformly element from [first, last).
template <typename It> typename It::value_type any(It first, It last) {
	int size = std::distance(first, last);
	It it = first;
	std::advance(it, next(0, size - 1));
	return *it;
}

// Returns uniformly element from container.
template <typename C> typename C::value_type any(const C &container) {
	return any(container.begin(), container.end());
}

// Chooses k values from the container, as in a subsequence of size k. Returns a
// copy.
template <typename C> C choose(int k, const C &container) {
	ensure(0 < k and k <= container.size(),
		   "number of elements to choose must be valid");
	std::vector<typename C::value_type> new_vec;
	C new_container;
	int need = k, left = container.size();
	for (auto cur_it = container.begin(); cur_it != container.end(); ++cur_it) {
		if (next(1, left--) <= need) {
			new_container.insert(new_container.end(), *cur_it);
			need--;
		}
	}
	return new_container;
}
template <typename T>
std::vector<T> choose(int k, const std::initializer_list<T> &list) {
	return choose(k, std::vector<T>(list.begin(), list.end()));
}

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

/*
 * Dictionary containing the positional parsed opts.
 */
std::vector<std::string> __pos_opts;

/*
 * Global dictionary the named parsed opts.
 */
std::map<std::string, std::string> __named_opts;

/*
 * Returns true if there is an opt at a given index.
 */
bool has_opt(int index) { return 0 <= index and index < __pos_opts.size(); }

/*
 * Returns true if there is an opt with a given key.
 */
bool has_opt(const std::string &key) { return __named_opts.count(key) != 0; }

/*
 * Parses 'value' into bool.
 */
template <typename T>
T __get_opt(const std::string &value, std::true_type, std::false_type,
			std::false_type) {
	if (value == "true" or value == "1")
		return true;
	if (value == "false" or value == "0")
		return false;
	throw __error("invalid value " + value + " for type bool");
}

/*
 * Parses 'value' into integral type T.
 */
template <typename T>
T __get_opt(const std::string &value, std::false_type, std::true_type,
			std::false_type) {
	if (std::is_unsigned<T>())
		return static_cast<T>(std::stoull(value));
	return static_cast<T>(std::stoll(value));
}

/*
 * Parses 'value' into floating type T.
 */
template <typename T>
T __get_opt(const std::string &value, std::false_type, std::false_type,
			std::true_type) {
	return static_cast<T>(std::stold(value));
}

/*
 * Parses 'value' into std::String.
 */
template <typename T>
T __get_opt(const std::string &value, std::false_type, std::false_type,
			std::false_type) {
	return value;
}

/*
 * Parses 'value' into type T.
 */
template <typename T> T __get_opt(const std::string &value) {
	try {
		return __get_opt<T>(value, std::is_same<T, bool>(),
							std::is_integral<T>(), std::is_floating_point<T>());
	} catch (const std::invalid_argument &er) {
	}
	throw __error("invalid value " + value + " for type " + typeid(T).name());
}

/*
 * Returns the parsed opt by a given index.
 */
template <typename T> T opt(int index) {
	ensure(has_opt(index), "cannot find key with index " + index);
	return __get_opt<T>(__pos_opts[index]);
}

/*
 * Return the parsed opt by a given index. If no opts with the index are
 * found, returns the given default_value.
 */
template <typename T> T opt(int index, const T &default_value) {
	if (!has_opt(index))
		return default_value;
	return __get_opt<T>(__pos_opts[index]);
}

/*
 * Returns the parsed opt by a given key.
 */
template <typename T> T opt(const std::string &key) {
	ensure(has_opt(key), "cannot find key with key " + key);
	return __get_opt<T>(__named_opts[key]);
}

/*
 * Returns the parsed opt by a given key. If no opts with the given key are
 * found, returns the given default_value.
 */
template <typename T> T opt(const std::string &key, const T &default_value) {
	if (!has_opt(key))
		return default_value;
	return __get_opt<T>(__named_opts[key]);
}

/*
 * Tries to fetch char from c string.
 */
char __fetch_char(char *s, int idx) {
	ensure(s[idx] != '\n', "tried to fetch end of string");
	return s[idx];
}

/*
 * Reads non-empty string until it hits a ' ' or the string ends.
 */
std::string __read_until(char *s) {
	std::string read_str;
	int idx = 0;
	while (s[idx] != '\0') {
		char nxt_char = __fetch_char(s, idx++);
		if (nxt_char == ' ')
			break;
		read_str += nxt_char;
	}

	ensure(read_str.size() > 0, "read string cannot be empty");
	return read_str;
}

/*
 * Parses the opts into __pos_opts vector and __named_opts map.
 */
void __parse_opts(int argc, char **argv) {
	// Starting from 1 to ignore the name of the executable.
	for (int i = 1; i < argc; i++) {
		std::string key = __read_until(argv[i]);

		if (key[0] == '-') {
			ensure(key.size() > 1,
				   "invalid opt (" + std::string(argv[i]) + ")");
			if (isdigit(key[1])) {
				// This case is a positional negative number argument
				__pos_opts.push_back(key);
				continue;
			}

			// pops first char '-'
			key = key.substr(1);
		} else {
			// This case is a positional argument that does not start with '-'
			__pos_opts.push_back(key);
			continue;
		}

		// Pops a possible second char '-'.
		if (key[0] == '-') {
			ensure(key.size() > 1,
				   "invalid opt (" + std::string(argv[i]) + ")");

			// pops first char '-'
			key = key.substr(1);
		}

		// Assumes that, if it starts with '-' and second char is not a digit,
		// then it is a <key, value> pair.
		// 1 or 2 chars '-' have already been poped.

		size_t eq = key.find('=');
		if (eq != std::string::npos) {
			// This is the '--key=value' case.
			std::string value = key.substr(eq + 1);
			key = key.substr(0, eq);
			ensure(key.size() > 0 and value.size() > 0, "expected non-empty\
						key/value in opt (" + std::string(argv[1]));
			ensure(__named_opts.count(key) == 0, "cannot have repeated keys");
			__named_opts[key] = value;
		} else {
			// This is the '--key value' case.
			ensure(__named_opts.count(key) == 0, "cannot have repeated keys");
			ensure(argv[i + 1], "value cannot be empty");
			__named_opts[key] = __read_until(argv[i + 1]);
			i++;
		}
	}
}

/*
 * Registers generator by initializing rnd and parsing opts.
 * Argument 'version' is unused.
 *
 */
void register_gen(int argc, char **argv) {
	std::vector<uint32_t> seed;

	// Starting from 1 to ignore the name of the executable.
	for (int i = 1; i < argc; ++i) {
		// We append the number of chars, and then the list of chars.
		int size_pos = seed.size();
		seed.push_back(0);
		for (char *s = argv[i]; *s != '\0'; ++s) {
			++seed[size_pos];
			seed.push_back(*s);
		}
	}
	std::seed_seq seq(seed.begin(), seed.end());
	__rng.seed(seq);

	__parse_opts(argc, argv);
}

}; // namespace tgen

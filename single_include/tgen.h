/*
 * Copyright (c) 2026 Bruno Monteiro
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace tgen {

/**************************
 *                        *
 *   GENERAL OPERATIONS   *
 *                        *
 **************************/

namespace _detail {

/*
 * Error handling.
 */

inline void throw_assertion_error(const std::string &condition,
								  const std::string &msg) {
	throw std::runtime_error("tgen: " + msg + " (assertion `" + condition +
							 "` failed)");
}
inline void throw_assertion_error(const std::string &condition) {
	throw std::runtime_error("tgen: assertion `" + condition + "` failed");
}
inline std::runtime_error error(const std::string &msg) {
	return std::runtime_error("tgen: " + msg);
}
inline std::runtime_error contradiction_error(const std::string &type,
											  const std::string &msg = "") {
	// Tried to generate a contradicting type.
	std::string error_msg =
		type + ": invalid " + type + " (contradicting restrictions)";
	if (!msg.empty())
		error_msg += ": " + msg;
	return error(error_msg);
}
inline std::runtime_error
complex_restrictions_error(const std::string &type,
						   const std::string &msg = "") {
	// Tried to generate a type with too many distinct restrictions.
	std::string error_msg =
		type + ": cannot represent " + type + " (complex restrictions)";
	if (!msg.empty())
		error_msg += ": " + msg;
	return error(error_msg);
}
inline void tgen_ensure_against_bug(bool cond, const std::string &msg = "") {
	if (!cond) {
		std::cerr << "tgen: " << msg << std::endl;
		throw std::runtime_error(
			"tgen: THERE IS A BUG IN TGEN; PLEASE CONTACT MAINTAINERS");
	}
}

// Ensures condition is true, with nice debug.
#define tgen_ensure(cond, ...)                                                 \
	if (!(cond))                                                               \
		tgen::_detail::throw_assertion_error(#cond, ##__VA_ARGS__);

// Registering checks.
inline bool registered = false;
inline void ensure_registered() {
	tgen_ensure(registered,
				"tgen was not registered! You should call "
				"tgen::register_gen(argc, argv) before running tgen functions");
}

// Template magic to detect types in compile time.

// Detects containers != std::string.
template <typename T, typename = void> struct is_container : std::false_type {};
template <typename T>
struct is_container<T, std::void_t<typename T::value_type,
								   decltype(std::begin(std::declval<T>())),
								   decltype(std::end(std::declval<T>()))>>
	: std::true_type {};
template <> struct is_container<std::string> : std::false_type {};
// Detects std::pair.
template <typename T> struct is_pair : std::false_type {};
template <typename A, typename B>
struct is_pair<std::pair<A, B>> : std::true_type {};
// Detects std::tuple.
template <typename T> struct is_tuple : std::false_type {};
template <typename... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type {};
// Detects scalar (printed atomically).
template <typename T>
struct is_scalar
	: std::bool_constant<!is_container<T>::value and !is_tuple<T>::value and
						 !is_pair<T>::value> {};
// Detects complex container.
template <typename T>
struct is_container_multiline
	: std::bool_constant<is_container<T>::value and
						 !is_scalar<typename T::value_type>::value> {};
// Detects complex std::pair.
template <typename T> struct is_pair_multiline : std::false_type {};
template <typename A, typename B>
struct is_pair_multiline<std::pair<A, B>>
	: std::bool_constant<!is_scalar<A>::value or !is_scalar<B>::value> {};
// Detects complex std::tuple.
template <typename Tuple> struct is_tuple_multiline : std::false_type {};
template <typename... Ts>
struct is_tuple_multiline<std::tuple<Ts...>>
	: std::bool_constant<(!is_scalar<Ts>::value or ...)> {};

/*
 * Properties of custom types.
 */

// If type is sequence-like.
using is_sequential_tag = void;

// If it makes sense to have a subset of the type.
using has_subset_defined_tag = void;

/*
 * Unique rng to use.
 */

// The single rng to be used by the library.
inline std::mt19937 rng;

/*
 * Base classes.
 */

// Used to return false in compile time only if evaluated.
template <typename> inline constexpr bool dependent_false_v = false;

// Base struct for generators.
template <typename Gen> struct gen_base {
	// Calls the generator until predicate is true.
	template <typename Pred, typename... Args>
	auto gen_until(Pred predicate, int max_tries, Args &&...args) const {
		for (int i = 0; i < max_tries; ++i) {
			auto inst = static_cast<const Gen *>(this)->gen(
				std::forward<Args>(args)...);

			if (predicate(inst))
				return inst;
		}

		throw error("could not generate instance matching predicate");
	}
	template <typename Pred, typename T, typename... Args>
	auto gen_until(Pred predicate, int max_tries, std::initializer_list<T> il,
				   Args &&...args) const {
		return gen_until(predicate, max_tries, std::vector<T>(il),
						 std::forward<Args>(args)...);
	}

	// Nice error for `out << generator`.
	friend std::ostream &operator<<(std::ostream &out, const gen_base &) {
		static_assert(
			dependent_false_v<gen_base>,
			"you cannot print a generator. Maybe you forgot to call `gen()`?");
		return out;
	}
};

// Base class for generator instances.
struct generator_instance_base {};

} // namespace _detail

// Detects associative containers.
template <typename T, typename = void>
struct is_associative_container : std::false_type {};
template <typename T>
struct is_associative_container<
	T, std::void_t<typename T::key_type, typename T::key_compare>>
	: std::true_type {};

// Detects generator instances.
template <typename T>
struct is_generator_instance
	: std::is_base_of<_detail::generator_instance_base, std::decay_t<T>> {};

// Detects sequential generator instances.
template <typename T, typename = void>
struct is_sequential : std::false_type {};
template <typename T>
struct is_sequential<
	T, std::void_t<typename std::decay_t<T>::tgen_is_sequential_tag>>
	: std::true_type {};

// Detects generator instances with defined subset.
template <typename T, typename = void>
struct has_subset_defined : std::false_type {};
template <typename T>
struct has_subset_defined<
	T, std::void_t<typename std::decay_t<T>::tgen_has_subset_defined_tag>>
	: std::true_type {};

/*
 * Easier printing.
 */

// Struct to print standard types to std::ostream;
struct print {
	std::string s_;

	template <typename T> print(const T &x) {
		std::ostringstream oss;
		write(oss, x);
		s_ = oss.str();
	}
	template <typename T> print(const std::initializer_list<T> &il) {
		std::ostringstream oss;
		write(oss, std::vector<T>(il.begin(), il.end()));
		s_ = oss.str();
	}
	template <typename T>
	print(const std::initializer_list<std::initializer_list<T>> &il) {
		std::ostringstream oss;
		std::vector<std::vector<T>> mat;
		for (const auto &i : il)
			mat.push_back(i);
		write(oss, mat);
		s_ = oss.str();
	}

	template <typename T> void write(std::ostream &os, const T &x) {
		if constexpr (_detail::is_pair<T>::value) {
			if constexpr (_detail::is_pair_multiline<T>::value) {
				write(os, x.first);
				os << '\n';
				write(os, x.second);
			} else {
				write(os, x.first);
				os << ' ';
				write(os, x.second);
			}
		} else if constexpr (_detail::is_tuple<T>::value)
			write_tuple(os, x);
		else if constexpr (_detail::is_container<T>::value)
			write_container(os, x);
		else
			os << x;
	}

	// Writes container, checking separator.
	template <typename C> void write_container(std::ostream &os, const C &c) {
		bool first = true;

		for (const auto &e : c) {
			if (!first)
				os << (_detail::is_container_multiline<C>::value ? '\n' : ' ');
			first = false;
			write(os, e);
		}
	}

	// Writes tuple, checking separator.
	template <typename Tuple, size_t... I>
	void write_tuple_impl(std::ostream &os, const Tuple &tp,
						  std::index_sequence<I...>) {
		bool first = true;
		((os << (first ? (first = false, "")
					   : (_detail::is_tuple_multiline<Tuple>::value ? "\n"
																	: " ")),
		  write(os, std::get<I>(tp))),
		 ...);
	}
	template <typename T> void write_tuple(std::ostream &os, const T &tp) {
		write_tuple_impl(os, tp,
						 std::make_index_sequence<std::tuple_size<T>::value>{});
	}

	friend std::ostream &operator<<(std::ostream &out, const print &pr) {
		return out << pr.s_;
	}
};

// Prints in a new line.
struct println : print {
	template <typename T> println(const T &x) : print(x) {}

	template <typename T>
	println(const std::initializer_list<T> &il) : print(il) {}

	template <typename T>
	println(const std::initializer_list<std::initializer_list<T>> &il)
		: print(il) {}

	friend std::ostream &operator<<(std::ostream &out, const println &pr) {
		return out << pr.s_ << '\n';
	}
};

/*
 * Global random operations.
 */

// Returns a random number in [0, n)
// O(1).
template <typename T> T next(T n) {
	_detail::ensure_registered();
	if constexpr (std::is_integral_v<T>) {
		tgen_ensure(n >= 1, "value for `next` bust be valid");
		return std::uniform_int_distribution<T>(0, n - 1)(_detail::rng);
	} else if constexpr (std::is_floating_point_v<T>) {
		tgen_ensure(n >= 0, "value for `next` bust be valid");
		return std::uniform_real_distribution<T>(0, n)(_detail::rng);
	} else
		throw _detail::error("invalid type for next (" +
							 std::string(typeid(T).name()) + ")");
}

// Returns a random number in [l, r].
// O(1).
template <typename T> T next(T l, T r) {
	_detail::ensure_registered();
	tgen_ensure(l <= r, "range for `next` bust be valid");
	if constexpr (std::is_integral_v<T>)
		return std::uniform_int_distribution<T>(l, r)(_detail::rng);
	else if constexpr (std::is_floating_point_v<T>)
		return std::uniform_real_distribution<T>(l, r)(_detail::rng);
	else
		throw _detail::error("invalid type for next (" +
							 std::string(typeid(T).name()) + ")");
}

// Returns i with probability proportional to distribution[i].
template <typename T>
size_t next_by_distribution(const std::vector<T> &distribution) {
	tgen_ensure(distribution.size() > 0, "distribution must be non-empty");

	T r = next<T>(
		std::accumulate(distribution.begin(), distribution.end(), T(0)));
	for (size_t i = 0; i < distribution.size(); ++i) {
		if (r < distribution[i])
			return i;
		r -= distribution[i];
	}
	return distribution.size() - 1;
}
template <typename T>
size_t next_by_distribution(const std::initializer_list<T> &distribution) {
	return next_by_distribution(
		std::vector<T>(distribution.begin(), distribution.end()));
}

// Shuffles [first, last) inplace uniformly, for RandomAccessIterator.
// O(|container|).
template <typename It> void shuffle(It first, It last) {
	if (first == last)
		return;

	for (It i = first + 1; i != last; ++i)
		std::iter_swap(i, first + next(0, static_cast<int>(i - first)));
}

// Shuffles sequential generator instance uniformly.
// O(|inst|).
template <typename Inst, std::enable_if_t<is_sequential<Inst>::value, int> = 0>
void shuffle(Inst &inst) {
	for (int i = 0; i < inst.size(); ++i)
		std::swap(inst[i], inst[next(0, inst.size() - 1)]);
}

// Shuffles container uniformly.
// O(|container|).
template <typename C, std::enable_if_t<!is_associative_container<C>::value and
										   !is_generator_instance<C>::value,
									   int> = 0>
[[nodiscard]] C shuffled(const C &container) {
	auto new_container = container;
	shuffle(new_container.begin(), new_container.end());
	return new_container;
}
template <typename C,
		  std::enable_if_t<is_associative_container<C>::value, int> = 0>
[[nodiscard]] std::vector<typename C::value_type> shuffled(const C &container) {
	return shuffled(std::vector<typename C::value_type>(container.begin(),
														container.end()));
}
template <typename T>
[[nodiscard]] std::vector<T> shuffled(const std::initializer_list<T> &il) {
	return shuffled(std::vector<T>(il.begin(), il.end()));
}

// Shuffles sequential generator instance uniformly.
// O(n).
template <typename Inst, std::enable_if_t<is_sequential<Inst>::value, int> = 0>
[[nodiscard]] Inst shuffled(const Inst &inst) {
	Inst new_inst = inst;
	shuffle(new_inst);
	return new_inst;
}

// Returns a random element from [first, last) uniformly.
// O(1) for random_access_iterator, O(|last - first|) otherwise.
template <typename It> typename It::value_type any(It first, It last) {
	int size = std::distance(first, last);
	It it = first;
	std::advance(it, next(0, size - 1));
	return *it;
}

// Returns a random element from container uniformly.
// O(1) for random_access_iterator, O(|container|) otherwise.
template <typename C,
		  std::enable_if_t<!is_generator_instance<C>::value, int> = 0>
typename C::value_type any(const C &container) {
	return any(container.begin(), container.end());
}
template <typename T> T any(const std::initializer_list<T> &il) {
	return any(std::vector<T>(il.begin(), il.end()));
}

// Returns a random element from sequential generator instance uniformly.
// O(1).
template <typename Inst, std::enable_if_t<is_sequential<Inst>::value, int> = 0>
typename Inst::value_type any(const Inst &inst) {
	return inst[next<int>(0, inst.size() - 1)];
}

// Returns container[i] with probability proportional to distribution[i].
// O(1) for random_access_iterator, O(|container|) otherwise.
template <typename C, typename T,
		  std::enable_if_t<!is_generator_instance<C>::value, int> = 0>
typename C::value_type any_by_distribution(const C &container,
										   std::vector<T> distribution) {
	tgen_ensure(container.size() == distribution.size(),
				"container and distribution must have the same size");
	auto it = container.begin();
	std::advance(it, next_by_distribution(distribution));
	return *it;
}
template <typename C, typename T,
		  std::enable_if_t<!is_generator_instance<C>::value, int> = 0>
typename C::value_type
any_by_distribution(const C &container,
					const std::initializer_list<T> &distribution) {
	return any_by_distribution(
		container, std::vector<T>(distribution.begin(), distribution.end()));
}
template <typename T, typename U>
T any_by_distribution(const std::initializer_list<T> &il,
					  const std::vector<U> &distribution) {
	return any_by_distribution(std::vector<T>(il.begin(), il.end()),
							   distribution);
}
template <typename T, typename U>
T any_by_distribution(const std::initializer_list<T> &il,
					  const std::initializer_list<U> &distribution) {
	return any_by_distribution(
		std::vector<T>(il.begin(), il.end()),
		std::vector<U>(distribution.begin(), distribution.end()));
}

// Returns inst[i] with probability proportional to distribution[i].
// O(1).
template <typename Inst, typename T,
		  std::enable_if_t<is_sequential<Inst>::value, int> = 0>
typename Inst::value_type
any_by_distribution(const Inst &inst, const std::vector<T> &distribution) {
	tgen_ensure(inst.size() == distribution.size(),
				"inst and distribution must have the same size");
	return inst[next_by_distribution(distribution)];
}
template <typename Inst, typename T,
		  std::enable_if_t<is_sequential<Inst>::value, int> = 0>
typename Inst::value_type
any_by_distribution(const Inst &inst,
					const std::initializer_list<T> &distribution) {
	return any_by_distribution(
		inst, std::vector<T>(distribution.begin(), distribution.end()));
}

// Chooses k values uniformly from container, as in a subsequence of size k.
// Returns a copy. O(|container|).
template <typename C,
		  std::enable_if_t<!is_generator_instance<C>::value, int> = 0>
C choose(int k, const C &container) {
	tgen_ensure(0 < k and k <= static_cast<int>(container.size()),
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
std::vector<T> choose(int k, const std::initializer_list<T> &il) {
	return choose(k, std::vector<T>(il.begin(), il.end()));
}

// Chooses k values uniformly from sequential generator instance, as in a
// subsequence of size k.
// O(n).
template <typename Inst, std::enable_if_t<is_sequential<Inst>::value and
											  has_subset_defined<Inst>::value,
										  int> = 0>
Inst choose(int k, const Inst &inst) {
	tgen_ensure(0 < k and k <= static_cast<int>(inst.size()),
				"number of elements to choose must be valid");
	std::vector<typename Inst::value_type> new_vec;
	int need = k;
	for (int i = 0; need > 0; ++i) {
		int left = inst.size() - i;
		if (next(1, left) <= need) {
			new_vec.push_back(inst[i]);
			need--;
		}
	}
	return Inst(new_vec);
}

// Number distinct generator for integral types.
template <typename T> struct distinct_range {
	T l_, r_;
	T num_available_;
	std::map<T, T> virtual_list_;

	// Generator of distinct values in [l_, r_].
	distinct_range(T l, T r) : l_(l), r_(r), num_available_(r - l + 1) {}

	// Returns the number of distinct values left to generate.
	T size() const { return num_available_; }

	// Generates a random value in [l_, r_] that has not been generated yet.
	// O(log n).
	T gen() {
		// One iteration of Fisher–Yates.
		tgen_ensure(size() > 0, "distinct_range: no more values to generate");

		T i = next<T>(0, size() - 1);
		T j = size() - 1;

		T vi = virtual_list_.count(i) ? virtual_list_[i] : i;
		T vj = virtual_list_.count(j) ? virtual_list_[j] : j;
		virtual_list_[i] = vj;

		--num_available_;

		return vi + l_;
	}
};

// Distinct generator for containers.
template <typename T> struct distinct_container {
	std::vector<T> list_;
	distinct_range<size_t> idx_;

	// Creates a distinct container generator for the given container.
	template <typename C>
	distinct_container(const C &container)
		: list_(container.begin(), container.end()),
		  idx_(0, static_cast<int>(container.size()) - 1) {}
	distinct_container(const std::initializer_list<T> &il)
		: distinct_container(std::vector<T>(il.begin(), il.end())) {}

	// Returns the number of distinct elements left to generate.
	size_t size() const { return idx_.size(); }

	// Generates a random element from container uniformly.
	// O(log n).
	T gen() { return list_[idx_.gen()]; }
};
template <typename C>
distinct_container(const C &) -> distinct_container<typename C::value_type>;

/************
 *          *
 *   OPTS   *
 *          *
 ************/

/*
 * Opts - options given to the generator.
 *
 * Incompatible with testlib.
 *
 * Opts are a list of either positional or named options.
 *
 * Named options is given in one of the following formats:
 * 1) -keyname=value or --keyname=value (ex. -n=10   , --test-count=20)
 * 2) -keyname value or --keyname value (ex. -n 10   , --test-count 20)
 *
 * Positional options are number from 0 sequentially.
 * For example, for "10 -n=20 str" positional option 1 is the string "str".
 */

namespace _detail {

inline std::vector<std::string>
	pos_opts; // Dictionary containing the positional parsed opts.
inline std::map<std::string, std::string>
	named_opts; // Global dictionary the named parsed opts.

template <typename T> T get_opt(const std::string &value) {
	try {
		if constexpr (std::is_same_v<T, bool>) {
			if (value == "true" or value == "1")
				return true;
			if (value == "false" or value == "0")
				return false;
		} else if constexpr (std::is_integral_v<T>) {
			if constexpr (std::is_unsigned_v<T>)
				return static_cast<T>(std::stoull(value));
			else
				return static_cast<T>(std::stoll(value));
		} else if constexpr (std::is_floating_point_v<T>)
			return static_cast<T>(std::stold(value));
		else
			return value; // default: std::string
	} catch (...) {
	}

	throw _detail::error("invalid value `" + value + "` for type " +
						 typeid(T).name());
}

inline void parse_opts(int argc, char **argv) {
	// Parses the opts into `pos_opts` vector and `named_opts`
	// map. Starting from 1 to ignore the name of the executable.
	for (int i = 1; i < argc; ++i) {
		std::string key(argv[i]);

		if (key[0] == '-') {
			tgen_ensure(key.size() > 1,
						"invalid opt (" + std::string(argv[i]) + ")");
			if ('0' <= key[1] and key[1] <= '9') {
				// This case is a positional negative number argument
				_detail::pos_opts.push_back(key);
				continue;
			}

			// pops first char '-'
			key = key.substr(1);
		} else {
			// This case is a positional argument that does not start with '-'
			_detail::pos_opts.push_back(key);
			continue;
		}

		// Pops a possible second char '-'.
		if (key[0] == '-') {
			tgen_ensure(key.size() > 1,
						"invalid opt (" + std::string(argv[i]) + ")");

			// pops first char '-'
			key = key.substr(1);
		}

		// Assumes that, if it starts with '-' and second char is not a digit,
		// then it is a <key, value> pair.
		// 1 or 2 chars '-' have already been poped.

		std::size_t eq = key.find('=');
		if (eq != std::string::npos) {
			// This is the '--key=value' case.
			std::string value = key.substr(eq + 1);
			key = key.substr(0, eq);
			tgen_ensure(!key.empty() and !value.empty(),
						"expected non-empty key/value in opt (" +
							std::string(argv[1]));
			tgen_ensure(_detail::named_opts.count(key) == 0,
						"cannot have repeated keys");
			_detail::named_opts[key] = value;
		} else {
			// This is the '--key value' case.
			tgen_ensure(_detail::named_opts.count(key) == 0,
						"cannot have repeated keys");
			tgen_ensure(argv[i + 1], "value cannot be empty");
			_detail::named_opts[key] = std::string(argv[i + 1]);
			++i;
		}
	}
}

inline void set_seed(int argc, char **argv) {
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
	_detail::rng.seed(seq);
}

} // namespace _detail

// Returns true if there is an opt at a given index.
inline bool has_opt(std::size_t index) {
	tgen::_detail::ensure_registered();
	return 0 <= index and index < _detail::pos_opts.size();
}

// Returns true if there is an opt with a given key.
inline bool has_opt(const std::string &key) {
	tgen::_detail::ensure_registered();
	return _detail::named_opts.count(key) != 0;
}

// Returns the parsed opt by a given index. If no opts with the given index are
// found, returns the given default_value.
template <typename T>
T opt(size_t index, std::optional<T> default_value = std::nullopt) {
	tgen::_detail::ensure_registered();
	if (!has_opt(index)) {
		if (default_value)
			return *default_value;
		throw _detail::error("cannot find key with index " +
							 std::to_string(index));
	}
	return _detail::get_opt<T>(_detail::pos_opts[index]);
}

// Returns the parsed opt by a given key. If no opts with the given key are
// found, returns the given default_value.
template <typename T>
T opt(const std::string &key, std::optional<T> default_value = std::nullopt) {
	tgen::_detail::ensure_registered();
	if (!has_opt(key)) {
		if (default_value)
			return *default_value;
		throw _detail::error("cannot find key with key " + key);
	}
	return _detail::get_opt<T>(_detail::named_opts[key]);
}

// Registers generator by initializing rnd and parsing opts.
inline void register_gen(int argc, char **argv) {
	_detail::set_seed(argc, argv);

	_detail::pos_opts.clear();
	_detail::named_opts.clear();
	_detail::parse_opts(argc, argv);

	_detail::registered = true;
}

/****************
 *              *
 *   SEQUENCE   *
 *              *
 ****************/

/*
 * Sequence generator.
 */

template <typename T> struct sequence : _detail::gen_base<sequence<T>> {
	int size_;			  // Size of sequence.
	T value_l_, value_r_; // Range of defined values.
	std::set<T> values_;  // Set of values. If empty, use range. if not,
						  // represents the possible values, and the range
						  // represents the index in this set)
	std::map<T, int>
		value_idx_in_set_; // Index of every value in the set above.
	std::vector<std::pair<T, T>> val_range_; // Range of values of each index.
	std::vector<std::vector<int>> neigh_;	 // Adjacency list of equality.
	std::vector<std::set<int>>
		distinct_restrictions_; // All distinct restrictions.

	// Creates generator for sequences of size 'size', with random T in [l, r].
	sequence(int size, T value_l, T value_r)
		: size_(size), value_l_(value_l), value_r_(value_r), neigh_(size) {
		tgen_ensure(size_ > 0, "sequence: size must be positive");
		tgen_ensure(value_l_ <= value_r_,
					"sequence: value range must be valid");
		for (int i = 0; i < size_; ++i)
			val_range_.emplace_back(value_l_, value_r_);
	}

	// Creates sequence with value set.
	sequence(int size, std::set<T> values)
		: size_(size), values_(values), neigh_(size) {
		tgen_ensure(size_ > 0, "sequence: size must be positive");
		tgen_ensure(!values.empty(), "sequence: value set must be non-empty");
		value_l_ = 0, value_r_ = values.size() - 1;
		for (int i = 0; i < size_; ++i)
			val_range_.emplace_back(value_l_, value_r_);
		int idx = 0;
		for (T value : values_)
			value_idx_in_set_[value] = idx++;
	}

	// Restricts sequences for sequence[idx] = value.
	sequence &fix(int idx, T value) {
		tgen_ensure(0 <= idx and idx < size_, "sequence: index must be valid");
		if (values_.size() == 0) {
			auto &[left, right] = val_range_[idx];
			if (left == right and value_l_ != value_r_) {
				tgen_ensure(left == value,
							"sequence: must not set to two different values");
			} else {
				tgen_ensure(left <= value and value <= right,
							"sequence: value must be in the defined range");
			}
			left = right = value;
		} else {
			tgen_ensure(values_.count(value),
						"sequence: value must be in the set of values");
			auto &[left, right] = val_range_[idx];
			int new_val = value_idx_in_set_[value];
			tgen_ensure(left <= new_val and new_val <= right,
						"sequence: must not set to two different values");
			left = right = new_val;
		}
		return *this;
	}

	// Restricts sequences for sequence[idx_1] = sequence[idx_2].
	sequence &equal(int idx_1, int idx_2) {
		tgen_ensure(0 <= std::min(idx_1, idx_2) and
						std::max(idx_1, idx_2) < size_,
					"sequence: indices must be valid");
		if (idx_1 == idx_2)
			return *this;

		neigh_[idx_1].push_back(idx_2);
		neigh_[idx_2].push_back(idx_1);
		return *this;
	}

	// Restricts sequences for sequence[left..right] to have all equal values.
	sequence &equal_range(int left, int right) {
		tgen_ensure(0 <= left and left <= right and right < size_,
					"sequence: range indices bust be valid");
		for (int i = left; i < right; ++i)
			equal(i, i + 1);
		return *this;
	}

	// Restricts sequences for sequence[S] to be distinct, for given subset S of
	// indices.
	// You can not add two of these restrictions with intersection.
	sequence &distinct(std::set<int> indices) {
		if (!indices.empty())
			distinct_restrictions_.push_back(indices);
		return *this;
	}

	// Restricts sequences for sequence[idx_1] != sequence[idx_2].
	sequence &different(int idx_1, int idx_2) {
		std::set<int> indices = {idx_1, idx_2};
		return distinct(indices);
	}

	// Restricts sequences with distinct elements.
	sequence &distinct() {
		std::vector<int> indices(size_);
		std::iota(indices.begin(), indices.end(), 0);
		return distinct(std::set<int>(indices.begin(), indices.end()));
	}

	// Sequence instance.
	// Operations on an instance are not random.
	struct instance : _detail::generator_instance_base {
		using tgen_is_sequential_tag = _detail::is_sequential_tag;
		using tgen_has_subset_defined_tag = _detail::has_subset_defined_tag;

		using value_type = T;			 // Value type, for templates.
		using std_type = std::vector<T>; // std type for instance.
		std::vector<T> vec_;			 // Sequence.

		instance(const std::vector<T> &vec) : vec_(vec) {}
		instance(const std::initializer_list<T> &il)
			: vec_(il.begin(), il.end()) {}

		// Fetches size.
		int size() const { return vec_.size(); }

		// Fetches position idx.
		T &operator[](int idx) {
			tgen_ensure(0 <= idx and idx < size(),
						"sequence: instance: index out of bounds");
			return vec_[idx];
		}
		const T &operator[](int idx) const {
			tgen_ensure(0 <= idx and idx < size(),
						"sequence: instance: index out of bounds");
			return vec_[idx];
		}

		// Sorts values in non-decreasing order.
		// O(n log n).
		instance &sort() {
			std::sort(vec_.begin(), vec_.end());
			return *this;
		}

		// Reverses sequence.
		// O(n).
		instance &reverse() {
			std::reverse(vec_.begin(), vec_.end());
			return *this;
		}

		// Concatenates two instances.
		// Linear.
		instance operator+(const instance &rhs) {
			std::vector<T> new_vec = vec_;
			for (int i = 0; i < rhs.size(); ++i)
				new_vec.push_back(rhs[i]);
			return instance(new_vec);
		}

		// Prints to std::ostream, separated by spaces.
		friend std::ostream &operator<<(std::ostream &out,
										const instance &inst) {
			for (int i = 0; i < inst.size(); ++i) {
				if (i > 0)
					out << ' ';
				out << inst[i];
			}
			return out;
		}

		// Gets a std::vector representing the instance.
		std::vector<T> to_std() const { return vec_; }
	};

	// Generates a uniformly random list of k distinct values in `[value_l,
	// value_r]`, such that no value is in `forbidden_values`.
	std::vector<T>
	generate_distinct_values(int k, const std::set<T> &forbidden_values) const {
		for (auto forbidden : forbidden_values)
			tgen_ensure(value_l_ <= forbidden and forbidden <= value_r_);
		// We generate our numbers in the range [0, num_available) with
		// num_available = (r-l+1)-(forbidden_values.size()), and then map them
		// to the correct range. We will run k steps of Fisher–Yates, using a
		// map to store a virtual sequence that starts with a[i] = i.
		T num_available = (value_r_ - value_l_ + 1) - forbidden_values.size();
		if (num_available < k)
			throw _detail::complex_restrictions_error(
				"sequence", "not enough distinct values");
		std::map<T, T> virtual_list;
		std::vector<T> gen_list;
		for (int i = 0; i < k; ++i) {
			T j = next<T>(i, num_available - 1);
			T vj = virtual_list.count(j) ? virtual_list[j] : j;
			T vi = virtual_list.count(i) ? virtual_list[i] : i;

			virtual_list[j] = vi, virtual_list[i] = vj;

			gen_list.push_back(virtual_list[i]);
		}

		// Shifts back to correct range, but there might still be values
		// that we can not use.
		for (T &value : gen_list)
			value += value_l_;

		// Now for every generated value, we shift it by how many forbidden
		// values are <= to it.
		std::vector<std::pair<T, int>> values_sorted;
		for (std::size_t i = 0; i < gen_list.size(); ++i)
			values_sorted.emplace_back(gen_list[i], i);
		// We iterate through them in increasing order.
		std::sort(values_sorted.begin(), values_sorted.end());
		auto cur_it = forbidden_values.begin();
		int smaller_forbidden_count = 0;
		for (auto [val, idx] : values_sorted) {
			while (cur_it != forbidden_values.end() and
				   *cur_it <= val + smaller_forbidden_count)
				++cur_it, ++smaller_forbidden_count;
			gen_list[idx] += smaller_forbidden_count;
		}

		return gen_list;
	}

	// Generates sequence instance.
	// O(n log n).
	instance gen() const {
		std::vector<T> vec(size_);
		std::vector<bool> defined_idx(
			size_, false); // For every index, if it has been set in `vec`.

		std::vector<int> comp_id(size_, -1); // Component id of each index.
		std::vector<std::vector<int>> comp(size_); // Component of each comp-id.
		int comp_count = 0; // Number of different components.

		// Defines value of entire component.
		auto define_comp = [&](int cur_comp, T val) {
			for (int idx : comp[cur_comp]) {
				tgen_ensure(!defined_idx[idx]);
				vec[idx] = val;
				defined_idx[idx] = true;
			}
		};

		// Groups = components.
		{
			std::vector<bool> vis(size_, false); // Visited for each index.
			for (int idx = 0; idx < size_; ++idx)
				if (!vis[idx]) {
					T new_value;
					bool value_defined = false;

					// BFS to visit the connected component, grouping equal
					// values.
					std::queue<int> q({idx});
					vis[idx] = true;
					std::vector<int> component;
					while (!q.empty()) {
						int cur_idx = q.front();
						q.pop();

						component.push_back(cur_idx);

						// Checks value.
						auto [l, r] = val_range_[cur_idx];
						if (l == r) {
							if (!value_defined) {
								// We found the value.
								value_defined = true;
								new_value = l;
							} else if (new_value != l) {
								// We found a contradiction
								throw _detail::contradiction_error(
									"sequence",
									"tried to set value to `" +
										std::to_string(new_value) +
										"`, but it was already set as `" +
										std::to_string(l) + "`");
							}
						}

						for (int nxt_idx : neigh_[cur_idx]) {
							if (!vis[nxt_idx]) {
								vis[nxt_idx] = true;
								q.push(nxt_idx);
							}
						}
					}

					// Group entire component, checking if value is defined.
					for (int cur_idx : component) {
						comp_id[cur_idx] = comp_count;
						comp[comp_id[cur_idx]].push_back(cur_idx);
					}

					// Defines value if needed.
					if (value_defined)
						define_comp(comp_count, new_value);

					++comp_count;
				}
		}

		// Initial parsing of distinct restrictions.
		std::vector<std::set<int>> distinct_containing_comp_idx(comp_count);
		{
			int dist_id = 0;
			for (const std::set<int> &distinct : distinct_restrictions_) {
				// Checks if there are too many distinct values.
				if (static_cast<uint64_t>(distinct.size() - 1) +
						static_cast<uint64_t>(value_l_) >
					static_cast<uint64_t>(value_r_))
					throw _detail::contradiction_error(
						"sequence",
						"tried to generate " + std::to_string(distinct.size()) +
							" distinct values, but the maximum is " +
							std::to_string(value_r_ - value_l_ + 1));

				// Checks if two values in same component are marked as
				// different.
				std::set<int> comp_ids;
				for (int idx : distinct) {
					if (comp_ids.count(comp_id[idx]))
						throw _detail::contradiction_error(
							"sequence", "tried to set two indices as equal and "
										"different");
					comp_ids.insert(comp_id[idx]);

					distinct_containing_comp_idx[comp_id[idx]].insert(dist_id);
				}
				++dist_id;
			}
		}

		// If some value is in >= 3 sets, then there is a cycle.
		for (auto &distinct_containing : distinct_containing_comp_idx)
			if (distinct_containing.size() >= 3)
				throw _detail::complex_restrictions_error(
					"sequence",
					"one index can not be in >= 3 distinct restrictions");

		std::vector<bool> vis_distinct(distinct_restrictions_.size(), false);
		std::vector<bool> initially_defined_comp_idx(comp_count, false);

		// Fills the value in a tree defined by distinct restrictions.
		auto define_tree = [&](int distinct_id) {
			// The set `distinct_restrictions_[distinct_id]` can have some
			// values that are defined.

			// Generates set of already defined values.
			std::set<T> defined_values;
			for (int idx : distinct_restrictions_[distinct_id])
				if (defined_idx[idx]) {
					// Checks if two values in `distinct_restrictions_[dist_id]`
					// have been set to the same value
					if (defined_values.count(vec[idx]))
						throw _detail::contradiction_error(
							"sequence",
							"tried to set two indices as equal and different");

					defined_values.insert(vec[idx]);
				}

			// Generates values in this root distinct restriction.
			{
				int new_value_count =
					distinct_restrictions_[distinct_id].size() -
					static_cast<int>(defined_values.size());
				std::vector<T> generated_values =
					generate_distinct_values(new_value_count, defined_values);
				auto val_it = generated_values.begin();
				for (int idx : distinct_restrictions_[distinct_id])
					if (defined_idx[idx]) {
						// The root can cover these components, but there should
						// not be any other defined in this tree.
						initially_defined_comp_idx[comp_id[idx]] = false;
					} else {
						define_comp(comp_id[idx], *val_it);
						++val_it;
					}
			}

			// BFS on the tree of distinct restrictions.
			std::queue<std::pair<int, int>> q; // {id, parent id}
			q.emplace(distinct_id, -1);
			vis_distinct[distinct_id] = true;
			while (!q.empty()) {
				auto [cur_distinct, parent] = q.front();
				q.pop();

				std::set<int> neigh_distinct;
				for (int idx : distinct_restrictions_[cur_distinct])
					for (int nxt_distinct :
						 distinct_containing_comp_idx[comp_id[idx]]) {
						if (nxt_distinct == cur_distinct or
							nxt_distinct == parent)
							continue;

						// Cycle found.
						if (vis_distinct[nxt_distinct])
							throw _detail::complex_restrictions_error(
								"sequence",
								"cycle found in distinct restrictions");

						neigh_distinct.insert(nxt_distinct);
					}

				for (int nxt_distinct : neigh_distinct) {
					vis_distinct[nxt_distinct] = true;
					q.emplace(nxt_distinct, cur_distinct);

					// Generates this distinct restriction.
					std::set<T> nxt_defined_values;
					for (int idx2 : distinct_restrictions_[nxt_distinct])
						if (defined_idx[idx2]) {
							// There can not be any more defined. This case is
							// when there are values not coverered by a single
							// distinct restriction in the tree.
							if (initially_defined_comp_idx[comp_id[idx2]])
								throw _detail::complex_restrictions_error(
									"sequence");

							nxt_defined_values.insert(vec[idx2]);
						}
					int new_value_count =
						distinct_restrictions_[nxt_distinct].size() -
						static_cast<int>(nxt_defined_values.size());
					std::vector<T> generated_values = generate_distinct_values(
						new_value_count, nxt_defined_values);
					auto val_it = generated_values.begin();
					for (int idx2 : distinct_restrictions_[nxt_distinct])
						if (!defined_idx[idx2]) {
							define_comp(comp_id[idx2], *val_it);
							++val_it;
						}
				}
			}
		};

		// Loops through distinct restrictions, sorts distinct restrictions
		// by number of defined components (non-increasing). This guarantees
		// that if there is a valid root (that covers all 'defined'), we find
		// it.
		{
			std::vector<std::pair<int, int>> defined_cnt_and_distinct_idx;
			int dist_id = 0;
			for (const std::set<int> &distinct : distinct_restrictions_) {
				int defined_cnt = 0;
				for (int idx : distinct)
					if (defined_idx[idx]) {
						++defined_cnt;
						initially_defined_comp_idx[comp_id[idx]] = true;
					}
				defined_cnt_and_distinct_idx.emplace_back(defined_cnt, dist_id);
				++dist_id;
			}

			std::sort(defined_cnt_and_distinct_idx.rbegin(),
					  defined_cnt_and_distinct_idx.rend());
			for (auto [defined_cnt, distinct_idx] :
				 defined_cnt_and_distinct_idx)
				if (!vis_distinct[distinct_idx])
					define_tree(distinct_idx);
		}

		// Loops through distinct restrictions do define the rest.
		for (std::size_t dist_id = 0; dist_id < distinct_restrictions_.size();
			 ++dist_id)
			if (!vis_distinct[dist_id])
				define_tree(dist_id);

		// Define final values. These values all should be random in [l, r], and
		// the distinct restrictions have already been processed. However, there
		// can be still equality restrictions, so we define entire components.
		for (int idx = 0; idx < size_; ++idx)
			if (!defined_idx[idx])
				define_comp(comp_id[idx], next<T>(value_l_, value_r_));

		if (!values_.empty()) {
			// Needs to fetch the values from the value set.
			std::vector<T> value_vec(values_.begin(), values_.end());
			for (T &val : vec)
				val = value_vec[val];
		}

		return instance(vec);
	}
};

/*******************
 *                 *
 *   PERMUTATION   *
 *                 *
 *******************/

/*
 * Permutation generation.
 *
 * Permutation are defined always as numbers in [0, n), that is, 0-based.
 */

struct permutation : _detail::gen_base<permutation> {
	int size_;								// Size of permutation.
	std::vector<std::pair<int, int>> defs_; // {idx, value}.

	// Creates generator for permutation of size 'size'.
	permutation(int size) : size_(size) {
		tgen_ensure(size_ > 0, "permutation: size must be positive");
	}

	// Restricts sequences for permutation[idx] = value.
	permutation &fix(int idx, int value) {
		tgen_ensure(0 <= idx and idx < size_,
					"permutation: index must be valid");
		defs_.emplace_back(idx, value);
		return *this;
	}

	// Permutation instance.
	// Operations on an instance are not random.
	struct instance : _detail::generator_instance_base {
		using tgen_is_sequential_tag = _detail::is_sequential_tag;

		using std_type = std::vector<int>; // std type for instance.
		std::vector<int> vec_;			   // Permutation.
		bool add_1_;					   // If should add 1, for printing.

		instance(const std::vector<int> &vec) : vec_(vec), add_1_(false) {
			tgen_ensure(!vec_.empty(),
						"permutation: instance: cannot be empty");
			std::vector<bool> vis(vec_.size(), false);
			for (int i = 0; i < size(); ++i) {
				tgen_ensure(0 <= vec_[i] and
								vec_[i] < static_cast<int>(vec_.size()),
							"permutation: instance: values must be from `0` to "
							"`size-1`");
				tgen_ensure(
					!vis[vec_[i]],
					"permutation: instance: cannot have repeated values");
				vis[vec_[i]] = true;
			}
		}
		instance(const std::initializer_list<int> &il)
			: instance(std::vector<int>(il.begin(), il.end())) {}

		// Fetches size.
		int size() const { return vec_.size(); }

		// Fetches position idx.
		const int &operator[](int idx) const {
			tgen_ensure(0 <= idx and idx < size(),
						"permutation: instance: index out of bounds");
			return vec_[idx];
		}

		// Returns parity of the permutation (+1 if even, -1 if odd).
		// O(n).
		int parity() const {
			std::vector<bool> vis(size(), false);
			int cycles = 0;

			for (int i = 0; i < size(); ++i)
				if (!vis[i]) {
					cycles++;
					for (int j = i; !vis[j]; j = vec_[j])
						vis[j] = true;
				}
			// Even iff (n - cycles) is even.
			return ((size() - cycles) % 2 == 0) ? +1 : -1;
		}

		// Sorts values in increasign order.
		// O(n).
		instance &sort() {
			for (int i = 0; i < size(); ++i)
				vec_[i] = i;
			return *this;
		}

		// Reverses permutation.
		// O(n).
		instance &reverse() {
			std::reverse(vec_.begin(), vec_.end());
			return *this;
		}

		// Inverse of the permutation.
		// O(n).
		instance &inverse() {
			std::vector<int> inv(size());
			for (int i = 0; i < size(); ++i)
				inv[vec_[i]] = i;
			swap(vec_, inv);
			return *this;
		}

		// Sets that should print values 1-based.
		// O(1).
		instance &add_1() {
			add_1_ = true;
			return *this;
		}

		// Prints to std::ostream, separated by spaces.
		friend std::ostream &operator<<(std::ostream &out,
										const instance &inst) {
			for (int i = 0; i < inst.size(); ++i) {
				if (i > 0)
					out << ' ';
				out << inst[i] + inst.add_1_;
			}
			return out;
		}

		// Gets a std::vector representing the instance.
		std::vector<int> to_std() const { return vec_; }
	};

	// Generates permutation instance.
	// O(n).
	instance gen() const {
		std::vector<int> idx_to_val(size_, -1), val_to_idx(size_, -1);
		for (auto [idx, val] : defs_) {
			tgen_ensure(0 <= val and val < size_,
						"permutation: value in permutation must be in [0, " +
							std::to_string(size_) + ")");

			if (idx_to_val[idx] != -1) {
				tgen_ensure(
					idx_to_val[idx] == val,
					"permutation: cannot set an idex to two different values");
			} else
				idx_to_val[idx] = val;

			if (val_to_idx[val] != -1) {
				tgen_ensure(
					val_to_idx[val] == idx,
					"permutation: cannot set two indices to the same value");
			} else
				val_to_idx[val] = idx;
		}

		std::vector<int> perm(size_);
		std::iota(perm.begin(), perm.end(), 0);
		shuffle(perm.begin(), perm.end());
		int cur_idx = 0;
		for (int &i : idx_to_val)
			if (i == -1) {
				// While this value is used, skip.
				while (val_to_idx[perm[cur_idx]] != -1)
					++cur_idx;
				i = perm[cur_idx++];
			}
		return idx_to_val;
	}

	// Generates permutation instance, given cycle sizes.
	// O(n).
	instance gen(std::vector<int> cycle_sizes) const {
		tgen_ensure(
			size_ == std::accumulate(cycle_sizes.begin(), cycle_sizes.end(), 0),
			"permutation: cycle sizes must add up to size of permutation");

		// Creates cycles.
		std::vector<int> order(size_);
		std::iota(order.begin(), order.end(), 0);
		shuffle(order.begin(), order.end());
		int idx = 0;
		std::vector<std::vector<int>> cycles;
		for (int cycle_size : cycle_sizes) {
			cycles.emplace_back();
			for (int i = 0; i < cycle_size; ++i)
				cycles.back().push_back(order[idx++]);
		}

		// Retrieves permutation from cycles.
		std::vector<int> perm(size_, -1);
		for (const std::vector<int> &cycle : cycles) {
			int cur_size = cycle.size();
			for (int i = 0; i < cur_size; ++i)
				perm[cycle[i]] = cycle[(i + 1) % cur_size];
		}

		return instance(perm);
	}
};

/************
 *          *
 *   MATH   *
 *          *
 ************/

namespace math {

namespace _detail {

inline int ctzll(uint64_t x) {
	// Mistery code found on the internet.
	// Uses de Brujin sequence.
	static const unsigned char index64[64] = {
		0,	1,	2,	53, 3,	7,	54, 27, 4,	38, 41, 8,	34, 55, 48, 28,
		62, 5,	39, 46, 44, 42, 22, 9,	24, 35, 59, 56, 49, 18, 29, 11,
		63, 52, 6,	26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
		51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12};
	return index64[((x & -x) * 0x022FDD63CC95386D) >> 58];
}

inline uint64_t mul_mod(uint64_t a, uint64_t b, uint64_t m) {
	return static_cast<unsigned __int128>(a) * b % m;
}

// O(log n).
// 0 <= x < m.
inline uint64_t expo_mod(uint64_t x, uint64_t y, uint64_t m) {
	if (!y)
		return 1;
	uint64_t ans = expo_mod(mul_mod(x, x, m), y / 2, m);
	return y % 2 ? mul_mod(x, ans, m) : ans;
}

} // namespace _detail

// O(log^2 n).
inline bool is_prime(uint64_t n) {
	if (n < 2)
		return false;
	if (n == 2 or n == 3)
		return true;
	if (n % 2 == 0)
		return false;

	uint64_t r = _detail::ctzll(n - 1), d = n >> r;
	// These bases are guaranteed to work for n <= 2^64.
	for (int a : {2, 325, 9375, 28178, 450775, 9780504, 1795265022}) {
		uint64_t x = _detail::expo_mod(a, d, n);
		if (x == 1 or x == n - 1 or a % n == 0)
			continue;

		for (uint64_t j = 0; j < r - 1; ++j) {
			x = _detail::mul_mod(x, x, n);
			if (x == n - 1)
				break;
		}
		if (x != n - 1)
			return false;
	}
	return true;
}

namespace _detail {

inline uint64_t pollard_rho(uint64_t n) {
	if (n == 1 or is_prime(n))
		return n;
	auto f = [n](uint64_t x) { return mul_mod(x, x, n) + 1; };

	uint64_t x = 0, y = 0, t = 30, prd = 2, x0 = 1, q;
	while (t % 40 != 0 or std::gcd(prd, n) == 1) {
		if (x == y)
			x = ++x0, y = f(x);
		q = mul_mod(prd, x > y ? x - y : y - x, n);
		if (q != 0)
			prd = q;
		x = f(x), y = f(f(y)), t++;
	}
	return std::gcd(prd, n);
}

inline std::vector<uint64_t> factor(uint64_t n) {
	if (n == 1)
		return {};
	if (is_prime(n))
		return {n};
	uint64_t d = _detail::pollard_rho(n);
	std::vector<uint64_t> l = factor(d), r = factor(n / d);
	l.insert(l.end(), r.begin(), r.end());
	return l;
}

// Error handling.
template <typename T>
std::runtime_error there_is_no_in_range_error(const std::string &type, T l,
											  T r) {
	return tgen::_detail::error("math: there is no " + type + " in range [" +
								std::to_string(l) + ", " + std::to_string(r) +
								"]");
}
template <typename T>
std::runtime_error there_is_no_upto_error(const std::string &type, T r) {
	return tgen::_detail::error("math: there is no " + type + " up to " +
								std::to_string(r));
}

// O(log mod).
// 0 < a < mod.
// gcd(a, mod) = 1.
inline __int128 modular_inverse_128(__int128 a, __int128 mod) {
	tgen_ensure(0 < a and a < mod,
				"math: remainder must be positive and smaller than the mod");

	__int128 t = 0, new_t = 1;
	__int128 r = mod, new_r = a;

	while (new_r != 0) {
		__int128 q = r / new_r;

		auto tmp_t = t - q * new_t;
		t = new_t;
		new_t = tmp_t;

		auto tmp_r = r - q * new_r;
		r = new_r;
		new_r = tmp_r;
	}

	tgen_ensure(r == 1, "math: remainder and mod must be coprime");

	if (t < 0)
		t += mod;
	return t;
}

// checks if a * b <= limit, for positive numbers.
inline bool mul_leq(uint64_t a, uint64_t b, uint64_t limit) {
	if (a == 0)
		return true;
	return a <= limit / b;
}

// base^exp, or null if base^exp > limit.
inline std::optional<uint64_t> expo(uint64_t base, uint64_t exp,
									uint64_t limit) {
	uint64_t result = 1;

	while (exp) {
		if (exp & 1) {
			if (!mul_leq(result, base, limit))
				return std::nullopt;
			result *= base;
		}

		exp >>= 1;
		// Necesary for correctness.
		if (!exp)
			break;

		if (!mul_leq(base, base, limit))
			return std::nullopt;
		base *= base;
	}
	return result;
}

// O(log n log k).
// 0 <= n.
// 0 < k.
inline uint64_t kth_root_floor(uint64_t n, uint64_t k) {
	tgen::_detail::tgen_ensure_against_bug(k > 0 and n >= 0,
										   "math: values must be valid");
	if (k == 1 or n <= 1)
		return n;

	uint64_t lo = 1, hi = 1ULL << ((64 + k - 1) / k);

	while (lo < hi) {
		uint64_t mid = lo + (hi - lo + 1) / 2;

		if (expo(mid, k, n)) {
			lo = mid;
		} else {
			hi = mid - 1;
		}
	}
	return lo;
}

// gcd(a, b).
// O(log a).
inline __int128 gcd128(__int128 a, __int128 b) {
	if (a < 0)
		a = -a;
	if (b < 0)
		b = -b;
	while (b != 0) {
		__int128 t = a % b;
		a = b;
		b = t;
	}
	return a;
}

// min(2^64, a*b).
// O(log a).
// a, b >= 0.
inline __int128 mul_saturate(__int128 a, __int128 b) {
	tgen_ensure(a >= 0 and b >= 0);
	static const __int128 LIMIT = (__int128)1 << 64;
	if (a == 0 or b == 0)
		return 0;
	if (a > LIMIT / b)
		return LIMIT;
	return a * b;
}

struct crt {
	using T = __int128;
	T a, m;

	crt() : a(0), m(1) {}
	crt(T a_, T m_) : a(a_), m(m_) {}
	crt operator*(crt C) {
		if (m == 0 or C.m == 0)
			return {-1, 0};

		T g = gcd128(m, C.m);
		if ((C.a - a) % g != 0)
			return {-1, 0};

		T m1 = m / g;
		T m2 = C.m / g;

		if (m2 == 1)
			return {a, m};

		T inv = modular_inverse_128(m1 % m2, m2);

		T k = ((C.a - a) / g) % m2;
		if (k < 0)
			k += m2;

		k = static_cast<unsigned __int128>(k) * inv % m2;

		T lcm = mul_saturate(m, m2);

		T res = (a + static_cast<T>((static_cast<unsigned __int128>(k) * m) %
									lcm)) %
				lcm;
		if (res < 0)
			res += lcm;

		return {res, lcm};
	}
};

// Math hacks to operate on log space.

inline constexpr long double LOG_ZERO = -INFINITY;
inline constexpr long double LOG_ONE = 0.0;

inline long double log_space(long double x) {
	return x == 0.0 ? LOG_ZERO : std::log(x);
}

// Math hack to add two values in log space.
inline long double add_log_space(long double a, long double b) {
	if (a == LOG_ZERO)
		return b;
	if (b == LOG_ZERO)
		return a;
	if (a < b)
		std::swap(a, b);
	if (b == LOG_ZERO)
		return a;
	return a + log1p(exp(b - a));
}

// Math hack to subtract two values in log space.
// a >= b.
inline long double sub_log_space(long double a, long double b) {
	if (b >= a)
		return LOG_ZERO;
	if (b == LOG_ZERO)
		return a;
	return a + log1p(-exp(b - a));
}

} // namespace _detail

// Sorted.
// O(n^(1/4) log n) expected.
// 0 < n.
inline std::vector<uint64_t> factor(uint64_t n) {
	tgen_ensure(n > 0, "math: number to factor must be positive");
	auto factors = _detail::factor(n);
	std::sort(factors.begin(), factors.end());
	return factors;
}

// Sorted.
// O(n^(1/4) log n) expected.
// 0 < n.
inline std::vector<std::pair<uint64_t, int>> factor_by_prime(uint64_t n) {
	tgen_ensure(n > 0, "math: number to factor must be positive");
	std::vector<std::pair<uint64_t, int>> primes;
	for (uint64_t p : factor(n)) {
		if (!primes.empty() and primes.back().first == p)
			++primes.back().second;
		else
			primes.emplace_back(p, 1);
	}
	return primes;
}

// O(log mod).
// 0 < a < mod.
// gcd(a, mod) = 1.
inline uint64_t modular_inverse(uint64_t a, uint64_t mod) {
	return _detail::modular_inverse_128(a, mod);
}

// O(n^(1/4) log n) expected.
// 0 < n.
inline uint64_t totient(uint64_t n) {
	tgen_ensure(n > 0, "math: totient(0) is undefined");
	uint64_t phi = n;

	for (auto [p, e] : factor_by_prime(n))
		phi -= phi / p;

	return phi;
}

// Returns `(p_i, g_i)`: `p_i` is the prime, `g_i` is the gap.
inline const std::pair<std::vector<uint64_t>, std::vector<uint64_t>> &
prime_gaps() {
	// From https://en.wikipedia.org/wiki/Prime_gap.
	static const std::pair<std::vector<uint64_t>, std::vector<uint64_t>> value{
		/* clang-format off */ {
			2, 3, 7, 23, 89, 113, 523, 887, 1129, 1327, 9551, 15683, 19609,
			31397, 155921, 360653, 370261, 492113, 1349533, 1357201, 2010733,
			4652353, 17051707, 20831323, 47326693, 122164747, 189695659,
			191912783, 387096133, 436273009, 1294268491, 1453168141,
			2300942549, 3842610773, 4302407359, 10726904659, 20678048297,
			22367084959, 25056082087, 42652618343, 127976334671, 182226896239,
			241160624143, 297501075799, 303371455241, 304599508537,
			416608695821, 461690510011, 614487453523, 738832927927,
			1346294310749, 1408695493609, 1968188556461, 2614941710599,
			7177162611713, 13829048559701, 19581334192423, 42842283925351,
			90874329411493, 171231342420521, 218209405436543, 1189459969825483,
			1686994940955803, 1693182318746371, 43841547845541059,
			55350776431903243, 80873624627234849, 203986478517455989,
			218034721194214273, 305405826521087869, 352521223451364323,
			401429925999153707, 418032645936712127, 804212830686677669,
			1425172824437699411, 5733241593241196731, 6787988999657777797
		}, /* clang-format on */
		{1,	   2,	 4,	   6,	 8,	   14,	 18,   20,	 22,   34,	 36,
		 44,   52,	 72,   86,	 96,   112,	 114,  118,	 132,  148,	 154,
		 180,  210,	 220,  222,	 234,  248,	 250,  282,	 288,  292,	 320,
		 336,  354,	 382,  384,	 394,  456,	 464,  468,	 474,  486,	 490,
		 500,  514,	 516,  532,	 534,  540,	 582,  588,	 602,  652,	 674,
		 716,  766,	 778,  804,	 806,  906,	 916,  924,	 1132, 1184, 1198,
		 1220, 1224, 1248, 1272, 1328, 1356, 1370, 1442, 1476, 1488, 1510}};

	return value;
}

// Returns pair (first_composite_in_gap, last_composite_in_gap).
// O(log r) approximately.
inline std::pair<uint64_t, uint64_t> prime_gap_upto(uint64_t r) {
	if (r < 4)
		throw _detail::there_is_no_upto_error("prime gap", r);

	const auto &[P, G] = prime_gaps();
	for (int i = P.size() - 1;; --i) {
		if (P[i] >= r)
			continue;

		uint64_t right = std::min(r, P[i] + G[i] - 1);
		uint64_t prev = i > 0 ? G[i - 1] : 0;
		uint64_t curr = right - P[i];

		if (curr >= prev)
			return {P[i] + 1, right};
	}
}

// From https://oeis.org/A002182/b002182.txt.
inline const std::vector<uint64_t> &highly_composites() {
	/* clang-format off */
	static const std::vector<uint64_t> highly_composites = {
	1, 2, 4, 6, 12, 24, 36, 48, 60, 120, 180, 240, 360, 720, 840, 1260, 1680,
	2520, 5040, 7560, 10080, 15120, 20160, 25200, 27720, 45360, 50400, 55440,
	83160, 110880, 166320, 221760, 277200, 332640, 498960, 554400, 665280,
	720720, 1081080, 1441440, 2162160, 2882880, 3603600, 4324320, 6486480,
	7207200, 8648640, 10810800, 14414400, 17297280, 21621600, 32432400,
	36756720, 43243200, 61261200, 73513440, 110270160, 122522400, 147026880,
	183783600, 245044800, 294053760, 367567200, 551350800, 698377680, 735134400,
	1102701600, 1396755360, 2095133040, 2205403200, 2327925600, 2793510720,
	3491888400, 4655851200, 5587021440, 6983776800, 10475665200, 13967553600,
	20951330400, 27935107200, 41902660800, 48886437600, 64250746560,
	73329656400, 80313433200, 97772875200, 128501493120, 146659312800,
	160626866400, 240940299600, 293318625600, 321253732800, 481880599200,
	642507465600, 963761198400, 1124388064800, 1606268664000, 1686582097200,
	1927522396800, 2248776129600, 3212537328000, 3373164194400, 4497552259200,
	6746328388800, 8995104518400, 9316358251200, 13492656777600, 18632716502400,
	26985313555200, 27949074753600, 32607253879200, 46581791256000,
	48910880818800, 55898149507200, 65214507758400, 93163582512000,
	97821761637600, 130429015516800, 195643523275200, 260858031033600,
	288807105787200, 391287046550400, 577614211574400, 782574093100800,
	866421317361600, 1010824870255200, 1444035528936000, 1516237305382800,
	1732842634723200, 2021649740510400, 2888071057872000, 3032474610765600,
	4043299481020800, 6064949221531200, 8086598962041600, 10108248702552000,
	12129898443062400, 18194847664593600, 20216497405104000, 24259796886124800,
	30324746107656000, 36389695329187200, 48519593772249600, 60649492215312000,
	72779390658374400, 74801040398884800, 106858629141264000,
	112201560598327200, 149602080797769600, 224403121196654400,
	299204161595539200, 374005201994424000, 448806242393308800,
	673209363589963200, 748010403988848000, 897612484786617600,
	1122015605983272000, 1346418727179926400, 1795224969573235200,
	2244031211966544000, 2692837454359852800, 3066842656354276800,
	4381203794791824000, 4488062423933088000, 6133685312708553600,
	8976124847866176000, 9200527969062830400, 12267370625417107200ULL,
	15334213281771384000ULL, 18401055938125660800ULL}; /* clang-format on */
	return highly_composites;
}

// O(log r) approximately.
inline uint64_t highly_composite_upto(uint64_t r) {
	for (int i = highly_composites().size() - 1; i >= 0; --i)
		if (highly_composites()[i] <= r)
			return highly_composites()[i];

	throw _detail::there_is_no_upto_error("highly composite number", r);
}

// O(log^3 r) expected.
// Generates a random prime in [l, r].
inline uint64_t gen_prime(uint64_t l, uint64_t r) {
	if (r < l or r < 2)
		throw _detail::there_is_no_in_range_error("prime", l, r);
	l = std::max<uint64_t>(l, 2);
	auto [l_gap, r_gap] = prime_gap_upto(r);
	if (r - l + 1 <= r_gap - l_gap + 1) {
		// There might be no primes in the range.
		std::vector<uint64_t> vals(r - l + 1);
		iota(vals.begin(), vals.end(), l);
		shuffle(vals.begin(), vals.end());
		for (uint64_t i : vals)
			if (is_prime(i))
				return i;
		throw _detail::there_is_no_in_range_error("prime", l, r);
	}

	uint64_t n;
	do {
		n = next(l, r);
	} while (!is_prime(n));
	return n;
}

// O(log^3 l) expected.
// l <= 2^64 - 59.
inline uint64_t prime_from(uint64_t l) {
	tgen_ensure(l <= std::numeric_limits<uint64_t>::max() - 58,
				"math: invalid bound");
	for (uint64_t i = std::max<uint64_t>(2, l);; ++i)
		if (is_prime(i))
			return i;
}

// O(log^3 r) expected.
inline uint64_t prime_upto(uint64_t r) {
	if (r >= 2)
		for (uint64_t i = r; i >= 2; --i)
			if (is_prime(i))
				return i;
	throw _detail::there_is_no_upto_error("prime", r);
}

// O(n^(1/4) log n) expected.
// 0 < n.
inline int num_divisors(uint64_t n) {
	int divisors = 1;
	for (auto [p, e] : factor_by_prime(n))
		divisors *= (e + 1);
	return divisors;
}

// O(log(r) log(divisor_count)).
// divisor_count is prime.
inline uint64_t gen_divisor_count(uint64_t l, uint64_t r, int divisor_count) {
	tgen_ensure(divisor_count > 0 and is_prime(divisor_count),
				"math: divisor count must be prime");
	int root = divisor_count - 1;
	uint64_t p = gen_prime(_detail::kth_root_floor(l, root),
						   _detail::kth_root_floor(r, root));
	return *_detail::expo(p, root, r);
}

// O(|mods| + log r).
// |rems| = |mods|.
// rems_i < mods_i.
inline uint64_t gen_congruent(uint64_t l, uint64_t r,
							  std::vector<uint64_t> rems,
							  std::vector<uint64_t> mods) {
	if (l > r)
		throw _detail::there_is_no_in_range_error("congruent number", l, r);
	tgen_ensure(rems.size() == mods.size(),
				"math: number of remainders and mods must be the same");

	_detail::crt crt;
	for (int i = 0; i < static_cast<int>(rems.size()); ++i) {
		tgen_ensure(rems[i] < mods[i],
					"math: remainder must be smaller than the mod");
		crt = crt * _detail::crt(rems[i], mods[i]);

		if (crt.a == -1 or crt.m > r) {
			if (!(l <= crt.a and crt.a <= r))
				throw _detail::there_is_no_in_range_error("congruent number", l,
														  r);

			bool valid_candidate = true;
			for (int j = 0; j < static_cast<int>(rems.size()); ++j)
				if (crt.a % mods[j] != rems[j])
					valid_candidate = false;
			if (valid_candidate)
				return crt.a;
			throw _detail::there_is_no_in_range_error("congruent number", l, r);
		}
	}

	uint64_t k_min = crt.a >= l ? 0 : ((l - crt.a) + crt.m - 1) / crt.m;
	uint64_t k_max = (r - crt.a) / crt.m;

	if (k_min > k_max)
		throw _detail::there_is_no_in_range_error("congruent number", l, r);

	return crt.a + next(k_min, k_max) * crt.m;
}

// O(1).
// rem < mod.
inline uint64_t gen_congruent(uint64_t l, uint64_t r, uint64_t rem,
							  uint64_t mod) {
	return gen_congruent(l, r, std::vector<uint64_t>({rem}),
						 std::vector<uint64_t>({mod}));
}

// O(log r).
inline uint64_t gen_even(uint64_t l, uint64_t r) {
	return gen_congruent(l, r, 0, 2);
}

// O(log r).
inline uint64_t gen_odd(uint64_t l, uint64_t r) {
	return gen_congruent(l, r, 1, 2);
}

// Mod used for FFT/NTT.
inline constexpr int FFT_MOD = 998244353;

// Fibonacci sequence up to 2^64.
inline const std::vector<uint64_t> &fibonacci() {
	static const std::vector<uint64_t> fib = [] {
		std::vector<uint64_t> v = {0, 1};
		while (v.back() <=
			   std::numeric_limits<uint64_t>::max() - v[v.size() - 2])
			v.push_back(v.back() + v[v.size() - 2]);
		return v;
	}();
	return fib;
}

// Parition is ordered (composition), that is, (1, 1, 2) != (1, 2, 1).
// O(n).
// 0 < n.
// 0 < part_l.
inline std::vector<int> gen_partition(int n, int part_l = 1, int part_r = -1) {
	if (part_r == -1)
		part_r = n;
	part_r = std::min(part_r, n);
	tgen_ensure(n > 0 and part_l > 0,
				"math: invalid parameters to gen_partition");
	tgen_ensure(part_l <= n and part_r > 0, "math: no such partition");

	// dp[i] = log(numbers of ways to add to i).
	std::vector<long double> dp(n + 1, _detail::LOG_ZERO);
	dp[0] = _detail::LOG_ONE;
	long double window = _detail::LOG_ZERO;
	for (int i = 1; i <= n; ++i) {
		if (i >= part_l)
			window = _detail::add_log_space(window, dp[i - part_l]);
		if (i >= part_r + 1)
			window = _detail::sub_log_space(window, dp[i - part_r - 1]);
		dp[i] = window;
	}
	tgen_ensure(dp[n] >= 0, "math: no such partition");

	// Crazy math tricks ahead.
	auto dp_pref = dp;
	for (int i = 1; i <= n; ++i)
		dp_pref[i] = _detail::add_log_space(dp_pref[i - 1], dp[i]);

	std::vector<int> part;
	int sum = n;
	while (sum > 0) {
		// Will generate a number such that what remains is in [l, r].
		int l = std::max(0, sum - part_r), r = sum - part_l;
		tgen::_detail::tgen_ensure_against_bug(r >= 0,
											   "math: r < 0 in gen_partition");

		int nxt_sum = std::min(sum, r);
		long double random = next<long double>(0, 1);

		// We generate a value X (log space), and then choose nxt_sum such that
		// dp_pref[nxt_sum-1] < X <= dp_pref[nxt_sum].

		// Math hack:
		// Let A = pref[l-1], B = pref[r], U = rand().
		// X = log[exp(A) + U * (exp(B) - exp(A))]
		//   = log{exp(B) * [exp(A) / exp(B) + U * (1 - exp(A) / exp(B))]}
		//   = B + log[exp(A - B) + U - U * exp(A - B))]
		//   = B + log[U + (1 - U) * exp(A - B)].
		long double val_l = l ? dp_pref[l - 1] : _detail::LOG_ZERO,
					val_r = dp_pref[r];
		while (nxt_sum > l and
			   dp_pref[nxt_sum - 1] >=
				   val_r + _detail::log_space(random + (1 - random) *
														   exp(val_l - val_r)))
			--nxt_sum;

		part.push_back(sum - nxt_sum);
		sum = nxt_sum;
	}

	return part;
}

// Parition is ordered (composition), that is, (1, 1, 2) != (1, 2, 1).
// O(n) time/memory if part_r = -1, O(n * k) time/memory otherwise.
// 0 < k <= n.
// 0 <= part_l.
inline std::vector<int> gen_partition_fixed_size(int n, int k, int part_l = 0,
												 int part_r = -1) {
	if (part_r == -1)
		part_r = n;
	part_r = std::min(part_r, n);
	tgen_ensure(0 < k and k <= n and part_l >= 0,
				"math: invalid parameters to gen_partition_fixed_size");
	tgen_ensure(static_cast<long long>(k) * part_l <= n and
					n <= static_cast<long long>(k) * part_r,
				"math: no such partition");

	// What we need to distribute to the parts.
	int s = n - k * part_l;

	std::vector<int> part(k);
	if (part_r == n) {
		// Stars and bars - O(n).
		std::vector<int> cuts = {-1};

		int total = s + k - 1, bars = k - 1;
		for (int i = 0; i < total and bars > 0; ++i)
			if (next<long double>(0, 1) <
				static_cast<long double>(bars) / (total - i)) {
				cuts.push_back(i);
				--bars;
			}
		cuts.push_back(total);

		// Recovers parts.
		for (int i = 0; i < k; ++i)
			part[i] = cuts[i + 1] - cuts[i] - 1;
	} else {
		// DP with log trick - O(nk).
		int u = part_r - part_l;

		// dp[i][j] = log(#ways to fill i parts with sum j)
		std::vector<std::vector<long double>> dp(
			k + 1, std::vector<long double>(s + 1, _detail::LOG_ZERO));
		dp[0][0] = _detail::LOG_ONE;

		for (int i = 1; i <= k; ++i) {
			std::vector<long double> pref = dp[i - 1];
			for (int j = 1; j <= s; ++j)
				pref[j] = _detail::add_log_space(pref[j - 1], dp[i - 1][j]);

			for (int j = 0; j <= s; ++j) {
				dp[i][j] = pref[j];
				if (j >= u + 1)
					dp[i][j] =
						_detail::sub_log_space(dp[i][j], pref[j - u - 1]);
			}
		}

		// Recovers parts backwards.
		int left_to_distribute = s;
		for (int i = k; i >= 1; --i) {
			long double log_total = _detail::LOG_ZERO;
			for (int j = 0; j <= u and j <= left_to_distribute; ++j)
				log_total = _detail::add_log_space(
					log_total, dp[i - 1][left_to_distribute - j]);
			tgen::_detail::tgen_ensure_against_bug(
				log_total != _detail::LOG_ZERO,
				"math: total == 0 in gen_partition_fixed_size");

			// Now we choose a number with probability proportional to
			// dp[i-1][.].

			// log(rand() * total) = log(rand()) + log(total).
			long double random =
				_detail::log_space(next<long double>(0, 1)) + log_total;

			long double cur_prob = _detail::LOG_ZERO;
			int chosen = 0;
			for (int j = 0; j <= u and j <= left_to_distribute; ++j) {
				cur_prob = _detail::add_log_space(
					cur_prob, dp[i - 1][left_to_distribute - j]);
				if (random < cur_prob) {
					chosen = j;
					break;
				}
			}

			part[k - i] = chosen;
			left_to_distribute -= chosen;
		}
	}

	for (int &i : part)
		i += part_l;
	return part;
}

}; // namespace math

/**************
 *            *
 *   STRING   *
 *            *
 **************/

/*
 * String generator.
 */

namespace _detail {

/*
 * Regex.
 *
 * Compatible with testlib's regex.
 *
 * Operations:
 * - A single character yields itself ("a", "3").
 * - A list of characters inside square braces yields any a random element
 *   from the list ("[abc123]").
 * - A range of characters is equivalent to listing them ("[a-z1-9A-Z]").
 * - A pattern followed by {n} yields the pattern repeated n times ("a{3}").
 * - A pattern followed by {l,r} yields the pattern repeated between l and r
 *   times, uniformly at random ("a{3,5}").
 * - A list of patterns separated by | yields a random pattern from the
 *   list, uniformly at random ("abc|def|ghi").
 * - Parentheses can be used for grouping ("a((a|b){3})").
 *
 * Examples:
 * 1. str("[1-9][0-9]{1,2}") generates two- or three-digit numbers.
 * 2. str("a[b-d]{2}|e") generates "e" or a random string of length 3, with
 *                       the first character being 'a' and the second and
 *                       third characters being 'b', 'c', or 'd'.
 * 3. str("[1-9][0-9]{%d}", n-1) generates n-digit numbers.
 *
 * Operations defined by {n} and {l,r} are applied from left to right, and
 * the pattern that comes before has its delimiters defined either by () or
 * [] at its end or is taken from the beginning of the pattern (in
 * "a[bc]{2}", "{2}" is applied to "[bc]", and in "[01]abc{3}", the "{3}" is
 * appied to "[01]abc").
 */

// If it has children, it is either a SEQ or an OR group, defined by the
// pattern_ field.
struct regex_node {
	// Considered to be repetition of left_bound != -1, pattern if
	// children_.empty(), otherwise "SEQ" or "OR", defined by the pattern_
	// field.
	std::string
		pattern_; // Either pattern, or "SEQ" or "OR" (if !children_.empty()).
	std::vector<regex_node> children_; // Children, when SEQ or OR.
	int left_bound_, right_bound_; // Left and right bounds of the repetition,
								   // or -1 if not a repetition.
	double
		log_space_num_ways_; // Log space number of ways to match the pattern.
	std::optional<distinct_container<char>>
		distinct_; // Distinct generator for the pattern, for [chars].

	// c or [chars].
	regex_node(const std::string &pattern)
		: pattern_(pattern), left_bound_(-1), right_bound_(-1) {
		if (pattern.size() == 1) {
			log_space_num_ways_ = math::_detail::LOG_ONE;
			return;
		}
		tgen_ensure_against_bug(pattern[0] == '[' and pattern.back() == ']',
								"str: invalid regex: expected character class");
		int size = pattern.size() - 2;
		log_space_num_ways_ = math::_detail::log_space(size);
		distinct_ = distinct_container<char>(pattern.substr(1, size));
	}
	// SEQ or OR.
	regex_node(const std::string &pattern, std::vector<regex_node> &children)
		: pattern_(pattern), left_bound_(-1), right_bound_(-1) {
		if (pattern == "SEQ") {
			// Multiply the number of ways.
			log_space_num_ways_ = math::_detail::LOG_ONE;
			for (const auto &child : children)
				log_space_num_ways_ += child.log_space_num_ways_;
		} else if (pattern == "OR") {
			// Add the number of ways.
			log_space_num_ways_ = math::_detail::LOG_ZERO;
			for (const auto &child : children)
				log_space_num_ways_ = math::_detail::add_log_space(
					log_space_num_ways_, child.log_space_num_ways_);
		} else
			tgen_ensure_against_bug("str: invalid regex: expected SEQ or OR");

		children_ = std::move(children);
		children.clear();
	}
	// REP.
	regex_node(int left_bound, int right_bound, regex_node &child)
		: pattern_("REP"), left_bound_(left_bound), right_bound_(right_bound) {
		log_space_num_ways_ = math::_detail::LOG_ZERO;
		for (int i = left_bound; i <= right_bound; i++)
			log_space_num_ways_ = math::_detail::add_log_space(
				log_space_num_ways_, i * child.log_space_num_ways_);

		children_.push_back(std::move(child));
	}
};

// State of the regex parser.
struct regex_state {
	std::vector<regex_node> cur;	  // Current sequence of nodes.
	std::vector<regex_node> branches; // Branches of the current OR group.
};

// Creates a SEQ node from the current state.
inline regex_node make_regex_seq(regex_state &st) {
	return regex_node("SEQ", st.cur);
}

// Finishes current state.
inline regex_node finish_regex_state(regex_state &st) {
	// SEQ.
	if (st.branches.empty())
		return make_regex_seq(st);

	// OR.
	st.branches.push_back(make_regex_seq(st));
	return regex_node("OR", st.branches);
}

// Parses a regex pattern into a tree, computing the number of ways to match the
// pattern.
inline regex_node parse_regex(std::string regex) {
	std::string new_regex;
	for (char c : regex)
		if (c != ' ')
			new_regex += c;
	swap(regex, new_regex);
	regex_state cur;
	std::vector<regex_state> stack;

	for (size_t i = 0; i < regex.size(); i++) {
		char c = regex[i];

		if (c == '(') {
			// Pushes the current state to the stack.
			stack.push_back(std::move(cur));
			cur = regex_state();
		} else if (c == ')') {
			// Finishes the current state, and adds it to the parent.
			regex_node node = finish_regex_state(cur);

			tgen_ensure(!stack.empty(), "str: invalid regex: unmatched `)`");
			cur = std::move(stack.back());
			stack.pop_back();

			cur.cur.push_back(std::move(node));
		} else if (c == '|') {
			// Starts a new OR group.
			regex_node node = make_regex_seq(cur);
			cur.branches.push_back(std::move(node));
		} else if (c == '[') {
			// Parses a character class.
			std::string chars;

			for (++i; i < regex.size() and regex[i] != ']'; ++i) {
				if (i + 2 < regex.size() and regex[i + 1] == '-') {
					char a = regex[i], b = regex[i + 2];
					if (a > b)
						std::swap(a, b);
					for (char x = a; x <= b; x++)
						chars += x;
					i += 2;
				} else
					chars += regex[i];
			}

			tgen_ensure(i < regex.size() and regex[i] == ']',
						"str: invalid regex: unmatched `[`");
			cur.cur.emplace_back("[" + chars + "]");
		} else if (c == '{') {
			// Parses a repetition.
			++i;
			int l = -1, r = -1;

			while (i < regex.size() and
				   isdigit(static_cast<unsigned char>(regex[i]))) {
				if (l == -1)
					l = 0;
				tgen_ensure(l <= static_cast<int>(1e8),
							"str: invalid regex: number too large inside `{}`");
				l = 10 * l + (regex[i] - '0');
				++i;
			}

			if (i < regex.size() and regex[i] == ',') {
				++i;
				while (i < regex.size() and
					   isdigit(static_cast<unsigned char>(regex[i]))) {
					if (r == -1)
						r = 0;
					tgen_ensure(
						r <= static_cast<int>(1e8),
						"str: invalid regex: number too large inside `{}`");
					r = 10 * r + (regex[i] - '0');
					++i;
				}
			} else
				r = l;

			tgen_ensure(i < regex.size() and regex[i] == '}',
						"str: invalid regex: unmatched `{`");
			tgen_ensure(l != -1 and r != -1,
						"str: invalid regex: missing number inside `{}`");
			tgen_ensure(l <= r, "invalid regex: invalid range inside `{}`");

			// Creates a REP node from the previous node.
			tgen_ensure(!cur.cur.empty(),
						"str: invalid regex: expected expression before `{}`");

			regex_node rep(l, r, cur.cur.back());
			cur.cur.pop_back();
			cur.cur.push_back(std::move(rep));
		} else {
			// Creates a char node.
			cur.cur.emplace_back(std::string(1, c));
		}
	}

	tgen_ensure(stack.empty(), "str: invalid regex: unmatched `(`");
	return finish_regex_state(cur);
}

// Generates a uniformly random string that matches the given regex.
inline void gen_regex(const regex_node &node, std::string &str) {
	// For [chars], generate a random character from the list.
	if (node.pattern_[0] == '[') {
		str += node.pattern_[1 + next<int>(0, node.pattern_.size() - 3)];
		return;
	}

	// For REP, generate a random number of times to repeat the pattern.
	if (node.left_bound_ != -1) {
		// Generates a random value W from 0 to num_ways.
		// log(W) = log(random(0, 1) * num_ways)
		//        = log(random(0, 1)) + log(num_ways).
		double log_rand = math::_detail::log_space(next<double>(0, 1)) +
						  node.log_space_num_ways_;
		double cur_prob = math::_detail::LOG_ZERO;
		double child_num_ways = node.children_[0].log_space_num_ways_;

		for (int i = node.left_bound_; i <= node.right_bound_; ++i) {
			cur_prob =
				math::_detail::add_log_space(cur_prob, i * child_num_ways);
			if (log_rand <= cur_prob) {
				for (int j = 0; j < i; ++j)
					gen_regex(node.children_[0], str);
				return;
			}
		}

		tgen_ensure_against_bug(false,
								"str: log_rand > cur_prob in REP gen_regex");
	}

	// For SEQ, generate all children.
	if (!node.children_.empty() and node.pattern_ == "SEQ") {
		for (const regex_node &child : node.children_)
			gen_regex(child, str);
		return;
	}

	// For OR, generate a random child.
	if (!node.children_.empty() and node.pattern_ == "OR") {
		// Generates a random value W from 0 to num_ways.
		// log(W) = log(random(0, 1) * num_ways)
		//        = log(random(0, 1)) + log(num_ways).
		double log_rand = math::_detail::log_space(next<double>(0, 1)) +
						  node.log_space_num_ways_;
		double cur_prob = math::_detail::LOG_ZERO;

		for (const regex_node &child : node.children_) {
			cur_prob = math::_detail::add_log_space(cur_prob,
													child.log_space_num_ways_);
			if (log_rand <= cur_prob) {
				gen_regex(child, str);
				return;
			}
		}

		tgen_ensure_against_bug(false,
								"str: log_rand > cur_prob in OR gen_regex");
	}

	// For char, generate the character.
	_detail::tgen_ensure_against_bug(
		node.pattern_.size() == 1,
		"str: invalid regex: expected single character, but got `" +
			node.pattern_ + "`");
	str += node.pattern_[0];
}

// Generates a uniformly random string that matches the given regex, updating
// the number of remaining strings that match the regex and have not been
// generated yet.
inline void gen_regex_update(regex_node &node, std::string &str) {
	// For [chars], generate a random character from the list.
	if (node.pattern_[0] == '[') {
		str += node.distinct_->gen();
		node.log_space_num_ways_ =
			math::_detail::log_space(node.distinct_->size());
		return;
	}

	// For REP, generate a random number of times to repeat the pattern.
	if (node.left_bound_ != -1) {
		// Generates a random value W from 0 to num_ways.
		// log(W) = log(random(0, 1) * num_ways)
		//        = log(random(0, 1)) + log(num_ways).
		double log_rand = math::_detail::log_space(next<double>(0, 1)) +
						  node.log_space_num_ways_;
		double cur_prob = math::_detail::LOG_ZERO;
		double child_num_ways = node.children_[0].log_space_num_ways_;
		bool generated = false;
		node.log_space_num_ways_ = math::_detail::LOG_ZERO;

		for (int i = node.left_bound_; i <= node.right_bound_; ++i) {
			cur_prob =
				math::_detail::add_log_space(cur_prob, i * child_num_ways);
			if (!generated and log_rand <= cur_prob) {
				generated = true;
				for (int j = 0; j < i; ++j)
					gen_regex_update(node.children_[0], str);
				generated = true;
			}
			node.log_space_num_ways_ = math::_detail::add_log_space(
				node.log_space_num_ways_, i * child_num_ways);
		}

		tgen_ensure_against_bug(
			generated, "str: log_rand > cur_prob in REP gen_regex_update");
		return;
	}

	// For SEQ, generate all children.
	if (!node.children_.empty() and node.pattern_ == "SEQ") {
		node.log_space_num_ways_ = math::_detail::LOG_ONE;
		for (const regex_node &child : node.children_) {
			gen_regex(child, str);
			node.log_space_num_ways_ += child.log_space_num_ways_;
		}
		return;
	}

	// For OR, generate a random child.
	if (!node.children_.empty() and node.pattern_ == "OR") {
		// Generates a random value W from 0 to num_ways.
		// log(W) = log(random(0, 1) * num_ways)
		//        = log(random(0, 1)) + log(num_ways).
		double log_rand = math::_detail::log_space(next<double>(0, 1)) +
						  node.log_space_num_ways_;
		double cur_prob = math::_detail::LOG_ZERO;
		bool generated = false;
		node.log_space_num_ways_ = math::_detail::LOG_ZERO;

		for (const regex_node &child : node.children_) {
			cur_prob = math::_detail::add_log_space(cur_prob,
													child.log_space_num_ways_);
			if (!generated and log_rand <= cur_prob) {
				gen_regex(child, str);
				generated = true;
			}
			node.log_space_num_ways_ = math::_detail::add_log_space(
				node.log_space_num_ways_, child.log_space_num_ways_);
		}

		tgen_ensure_against_bug(
			generated, "str: log_rand > cur_prob in OR gen_regex_update");
		return;
	}

	// // For char, generate the character.
	// _detail::tgen_ensure_against_bug(
	// 	node.pattern_.size() == 1,
	// 	"str: invalid regex: expected single character, but got `" +
	// 		node.pattern_ + "`");
	// str += node.pattern_[0];
}

// Formats a regex string with given arguments.
template <typename... Args>
std::string regex_format(const std::string &s, Args &&...args) {
	if constexpr (sizeof...(Args) == 0) {
		return s;
	} else {
		int size = std::snprintf(nullptr, 0, s.c_str(), args...) + 1;
		std::string buf(size, '\0');
		std::snprintf(buf.data(), size, s.c_str(), args...);
		buf.pop_back(); // remove '\0'
		return buf;
	}
}

} // namespace _detail

struct str : _detail::gen_base<str> {
	std::optional<sequence<char>> seq_; // Sequence of characters.
	std::optional<_detail::regex_node>
		root_; // Root node of the regex tree for the whole string.

	// Creates generator for strings of size 'size', with random characters in
	// [value_l, value_r].
	str(int size, char value_l = 'a', char value_r = 'z')
		: seq_(sequence<char>(size, value_l, value_r)) {}

	// Creates generator for strings of size 'size', with random characters in
	// 'chars'.
	str(int size, std::set<char> chars) : seq_(sequence<char>(size, chars)) {}

	// Creates generator for strings that match the given regex.
	template <typename... Args> str(const std::string &regex, Args &&...args) {
		tgen_ensure(regex.size() > 0, "str: regex must be non-empty");

		root_ = _detail::parse_regex(
			_detail::regex_format(regex, std::forward<Args>(args)...));
	}

	// Restricts strings for str[idx] = value.
	str &fix(int idx, char character) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		seq_->fix(idx, character);
		return *this;
	}

	// Restricts strings for str[idx_1] = str[idx_2].
	str &equal(int idx_1, int idx_2) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		seq_->equal(idx_1, idx_2);
		return *this;
	}

	// Restricts strings for str[left..right] to have all equal values.
	str &equal_range(int left, int right) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		seq_->equal_range(left, right);
		return *this;
	}

	// Restricts strings for str[left..right] to be a palindrome.
	str &palindrome(int left, int right) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		tgen_ensure(0 <= left and left <= right and right < seq_->size_,
					"str: range indices bust be valid");
		for (int i = left; i < right - (i - left); ++i)
			equal(i, right - (i - left));
		return *this;
	}

	// Restricts strings for the entire string to be a palindrome.
	str &palindrome() {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		return palindrome(0, seq_->size_ - 1);
	}

	// Restricts strings for str[S] to be distinct, for given subset S of
	// indices.
	str &distinct(std::set<int> indices) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		seq_->distinct(indices);
		return *this;
	}

	// Restricts strings for str[idx_1] != str[idx_2].
	str &different(int idx_1, int idx_2) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		seq_->different(idx_1, idx_2);
		return *this;
	}

	// Restricts strings for all values to be distinct.
	str &distinct() {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		seq_->distinct();
		return *this;
	}

	// str instance.
	// Operations on an instance are not random.
	struct instance : _detail::generator_instance_base {
		using tgen_is_sequential_tag = _detail::is_sequential_tag;
		using tgen_has_subset_defined_tag = _detail::has_subset_defined_tag;

		using value_type = char;
		using std_type = std::string;
		std::string str_;

		instance(const std::string &str) : str_(str) {
			tgen_ensure(!str_.empty(), "str: instance: cannot be empty");
		}

		// Fetches size.
		int size() const { return str_.size(); }

		// Fetches position idx.
		char &operator[](int idx) {
			tgen_ensure(0 <= idx and idx < size(),
						"str: instane: index out of bounds");
			return str_[idx];
		}
		const char &operator[](int idx) const {
			tgen_ensure(0 <= idx and idx < size(),
						"str: instance: index out of bounds");
			return str_[idx];
		}

		// Sorts characters in non-decreasing order.
		// O(n log n).
		instance &sort() {
			std::sort(str_.begin(), str_.end());
			return *this;
		}

		// Reverses string.
		// O(n).
		instance &reverse() {
			std::reverse(str_.begin(), str_.end());
			return *this;
		}

		// Lowercases all characters.
		// O(n).
		instance &lowercase() {
			for (char &c : str_)
				c = std::tolower(c);
			return *this;
		}

		// Uppercases all characters.
		// O(n).
		instance &uppercase() {
			for (char &c : str_)
				c = std::toupper(c);
			return *this;
		}

		// Concatenates two instances.
		// Linear.
		instance operator+(const instance &rhs) {
			return instance(str_ + rhs.str_);
		}

		// Prints to std::ostream, separated by spaces.
		friend std::ostream &operator<<(std::ostream &out,
										const instance &inst) {
			return out << inst.str_;
		}

		// Gets a std::string representing the instance.
		std::string to_std() const { return str_; }
	};

	// Generates str instance.
	// If created from restrictions: O(n log n).
	// If created from regex: expected linear.
	instance gen() const {
		if (root_) {
			// Regex.
			std::string ret_str;
			gen_regex(*root_, ret_str);
			return instance(ret_str);
		} else {
			// Sequence.
			std::vector<char> vec = seq_->gen().to_std();
			return instance(std::string(vec.begin(), vec.end()));
		}
	}

	struct distinct {
		_detail::regex_node root_;

		template <typename... Args>
		distinct(const std::string &regex, Args &&...args) {
			root_ = _detail::parse_regex(
				_detail::regex_format(regex, std::forward<Args>(args)...));
		}

		instance gen() {
			std::string ret_str;
			_detail::gen_regex(root_, ret_str);
			return instance(ret_str);
		}
	};

	// Fetches prefix of length n of the string "abacabadabacabae...".
	static instance abacaba(int n) {
		tgen_ensure(n > 0, "str: size must be positive");
		std::string str = "a";
		char c = 'a';
		while (static_cast<int>(str.size()) < n) {
			int prev_size = str.size();
			str += ++c;
			for (int j = 0; j < prev_size and static_cast<int>(str.size()) < n;
				 ++j)
				str += str[j];
		}
		return instance(str);
	}
};

} // namespace tgen

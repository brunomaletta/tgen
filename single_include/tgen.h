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
#include <bitset>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tgen {

/**************************
 *                        *
 *   GENERAL OPERATIONS   *
 *                        *
 **************************/

namespace detail {

// Type aliases.
using u128 = unsigned __int128;
using i128 = __int128;

/*
 * Error handling.
 */

inline void throw_assertion_error(const std::string &condition,
								  const std::string &msg, const char *file,
								  int line) {
	throw std::runtime_error("tgen: " + msg + " (assertion `" + condition +
							 "` failed at " + file + ":" +
							 std::to_string(line) + ")");
}
inline void throw_assertion_error(const std::string &condition,
								  const char *file, int line) {
	throw std::runtime_error("tgen: assertion `" + condition + "` failed at " +
							 std::string(file) + ":" + std::to_string(line));
}
inline std::runtime_error error(const std::string &msg) {
	return std::runtime_error("tgen: " + msg);
}
inline std::runtime_error contradiction_error(const std::string &type,
											  const std::string &msg = "") {
	// Tried to generate a contradictory type.
	std::string error_msg =
		type + ": invalid " + type + " (contradictory restrictions)";
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
		std::string error_msg;
		if (!msg.empty())
			error_msg = "tgen: " + msg + "\n";
		error_msg += "tgen: THERE IS A BUG IN TGEN; PLEASE CONTACT MAINTAINERS";
		throw std::runtime_error(error_msg);
	}
}

// Ensures condition is true, with a clear error message on failure.
#define tgen_ensure(cond, ...)                                                 \
	if (!(cond))                                                               \
	tgen::detail::throw_assertion_error(#cond, ##__VA_ARGS__, __FILE__,        \
										__LINE__)

// Registering checks.
inline bool registered = false;
inline void ensure_registered() {
	tgen_ensure(registered,
				"tgen was not registered! You should call "
				"tgen::register_gen(argc, argv) before running tgen functions");
}

// Template magic to detect types at compile time.

// Detects containers != std::string.
template <typename T, typename = void> struct is_container : std::false_type {};
template <typename T>
struct is_container<T,
					std::void_t<typename std::remove_reference_t<T>::value_type,
								decltype(std::begin(std::declval<T>())),
								decltype(std::end(std::declval<T>()))>>
	: std::true_type {};
// Exclude all basic_string variants
template <typename Char, typename Traits, typename Alloc>
struct is_container<std::basic_string<Char, Traits, Alloc>> : std::false_type {
};
template <typename Char, typename Traits, typename Alloc>
struct is_container<const std::basic_string<Char, Traits, Alloc>>
	: std::false_type {};
template <typename Char, typename Traits, typename Alloc>
struct is_container<std::basic_string<Char, Traits, Alloc> &>
	: std::false_type {};
template <typename Char, typename Traits, typename Alloc>
struct is_container<const std::basic_string<Char, Traits, Alloc> &>
	: std::false_type {};

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
						 !is_scalar<typename std::remove_cv_t<
							 std::remove_reference_t<T>>::value_type>::value> {
};
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

// Used to return false at compile time only if evaluated.
template <typename> inline constexpr bool dependent_false_v = false;

/*
 * Properties of custom types.
 */

// If type is sequential (list-like).
using is_sequential_tag = void;

// Detects associative containers.
template <typename T, typename = void>
struct is_associative_container : std::false_type {};
template <typename T>
struct is_associative_container<
	T, std::void_t<typename T::key_type, typename T::key_compare>>
	: std::true_type {};

// Detects sequential generator values.
template <typename T, typename = void>
struct is_sequential : std::false_type {};
template <typename T>
struct is_sequential<
	T, std::void_t<typename std::decay_t<T>::tgen_is_sequential_tag>>
	: std::true_type {};

/*
 * Unique rng to use.
 */

// The single rng to be used by the library.
inline std::mt19937 rng;

/*
 * Printing.
 */

// Print view struct for printing either a container or a sequential generator
// element.
template <typename T,
		  bool IsCont = detail::is_container<std::decay_t<T>>::value>
struct print_cols_view;

// Container.
template <typename T> struct print_cols_view<T, true> {
	const T &value;
	decltype(std::begin(std::declval<const T &>())) it;

	print_cols_view(const T &v) : value(v), it(v.begin()) {}

	std::size_t size() const { return value.size(); }
	decltype(auto) get(std::size_t) const { return *it; }
	void advance() { ++it; }
};

// Sequential generator element.
template <typename T> struct print_cols_view<T, false> {
	const T &value;

	print_cols_view(const T &v) : value(v) {}

	std::size_t size() const { return value.size(); }
	decltype(auto) get(std::size_t i) const { return value[i]; }
	void advance() {}
};

/*
 * Distinct generation.
 */

// Rejection cap is multiplier * |seen|; with one value left, falsely reporting
// exhaustion has probability about e^{-84} < 10^{-36}.
constexpr int distinct_attempt_multiplier = 84;

// One rejection-sampling step for distinct generation.
template <typename Seen, typename Fn>
auto try_generate_distinct(Seen &seen, Fn &&next, bool insert = true)
	-> std::optional<std::invoke_result_t<Fn &>> {
	using T = std::invoke_result_t<Fn &>;
	size_t attempts =
		distinct_attempt_multiplier * std::max<size_t>(1, seen.size());
	for (size_t i = 0; i < attempts; ++i) {
		T val = next();
		if (insert) {
			if (seen.insert(val).second)
				return val;
		} else if (seen.count(val) == 0)
			return val;
	}
	return std::nullopt;
}

} // namespace detail

/*
 * Compiler configuration (see set_compiler).
 */

// Kinds of compilers.
enum class compiler_kind { gcc, clang, unknown };

// Compiler identity and version.
struct compiler_value {
	compiler_kind kind_;
	int major_;
	int minor_;

	compiler_value(compiler_kind kind = compiler_kind::unknown, int major = 0,
				   int minor = 0)
		: kind_(kind), major_(major), minor_(minor) {}
};

namespace detail {

// Global C++ version value (0 means unknown).
struct cpp_value {
	int version_;

	cpp_value(std::optional<int> version = std::nullopt)
		: version_(version ? *version : 0) {
		if (version) {
			tgen_ensure(*version == 17 or *version == 20 or *version == 23,
						"unsupported C++ version (use 17, 20, 23)");
		}
	}
};

inline cpp_value cpp;
inline compiler_value compiler;

} // namespace detail

/*
 * Base classes.
 */

// Needed for return type of some functions.
template <typename T> struct list;

// Generates distinct values of a function.
template <typename Func, typename... Args> struct distinct {
	Func func_;
	std::tuple<Args...> args_;
	using T = std::invoke_result_t<Func &, Args &...>;
	std::set<T> seen_;

	distinct(Func func, Args... args)
		: func_(std::move(func)), args_(std::move(args)...) {}

	// Generates a distinct value (i.e., one not returned before).
	//
	// Assume gen() produces a uniformly random value in O(T) time.
	// Since duplicates are rejected, the expected number of trials over
	// k successful generations is:
	//
	//   sum_{i=1}^k k / i = O(k log k)
	//
	// (coupon collector argument).
	//
	// Each trial additionally performs O(log k) work to check/store
	// previously generated values, yielding a total time of
	// O((T + log k) * k log k).
	//
	// Thus, the amortized expected time per call is
	// O(T * log k + log^2 k).
	//
	// With extremely small probability (< 1e-18), the algorithm may
	// incorrectly report that no more distinct values exist.
	auto gen() {
		auto val = generate_distinct(true);
		if (val)
			return *val;

		throw detail::error("distinct: no more distinct values");
	}
	template <typename U> auto gen(std::initializer_list<U> il) {
		return gen(std::vector<U>(il));
	}

	// Generates a list of distinct values.
	auto gen_list(int size) {
		std::vector<T> res;
		for (int i = 0; i < size; ++i)
			res.push_back(gen());

		return typename list<T>::value(res);
	}

	// Checks if there are no more distinct values.
	// With extremely small probability (< 1e-18), the algorithm may
	// incorrectly report that there are no more distinct values.
	bool empty() { return generate_distinct(false) == std::nullopt; }

	// Generates all distinct values.
	auto gen_all() {
		std::vector<T> res;
		while (true) {
			auto val = generate_distinct(true);
			if (val)
				res.push_back(*val);
			else
				break;
		}
		return typename list<T>::value(res);
	}

	// Nice error for `out << distinct`.
	friend std::ostream &operator<<(std::ostream &out, const distinct &) {
		static_assert(
			detail::dependent_false_v<distinct>,
			"distinct: cannot print a distinct generator. Maybe you forgot to "
			"call `gen()`?");
		return out;
	}

  private:
	// Generates distinct value and inserts it if `insert` is true.
	// Returns the value if found, otherwise returns std::nullopt.
	auto generate_distinct(bool insert) {
		return detail::try_generate_distinct(
			seen_, [&] { return std::apply(func_, args_); }, insert);
	}
};
template <typename Func, typename... Args>
distinct(Func, Args...) -> distinct<Func, Args...>;

// Base struct for generators.
template <typename Gen> struct gen_base {
	const Gen &self() const { return *static_cast<const Gen *>(this); }

	template <typename... Args> auto gen_list(int size, Args &&...args) const {
		std::vector<typename Gen::value> res;

		for (int i = 0; i < size; ++i)
			res.push_back(static_cast<const Gen *>(this)->gen(
				std::forward<Args>(args)...));

		return typename list<typename Gen::value>::value(res);
	}

	// Calls the generator until predicate is true.
	template <typename Pred, typename... Args>
	auto gen_until(Pred predicate, int max_tries, Args &&...args) const {
		for (int i = 0; i < max_tries; ++i) {
			typename Gen::value val = static_cast<const Gen *>(this)->gen(
				std::forward<Args>(args)...);

			if (predicate(val))
				return val;
		}

		throw detail::error("could not generate value matching predicate");
	}
	template <typename Pred, typename T, typename... Args>
	auto gen_until(Pred predicate, int max_tries, std::initializer_list<T> il,
				   Args &&...args) const {
		return gen_until(predicate, max_tries, std::vector<T>(il),
						 std::forward<Args>(args)...);
	}

	// Distinct for generator.
	template <typename... Args> auto distinct(Args &&...args) const {
		return tgen::distinct(
			[self = self()](auto &&...inner_args) mutable -> decltype(auto) {
				return self.gen(
					std::forward<decltype(inner_args)>(inner_args)...);
			},
			std::forward<Args>(args)...);
	}
	template <typename T, typename... Args>
	auto distinct(std::initializer_list<T> il, Args &&...args) const {
		return distinct(std::vector<T>(il), std::forward<Args>(args)...);
	}

	// Nice error for `out << generator`.
	friend std::ostream &operator<<(std::ostream &out, const gen_base &) {
		static_assert(detail::dependent_false_v<gen_base>,
					  "gen_base: cannot print a generator. Maybe you forgot to "
					  "call `gen()`?");
		return out;
	}
};

// Base class for generator values.
template <typename Val> struct gen_value_base {
	const Val &self() const { return *static_cast<const Val *>(this); }

	bool operator<(const Val &rhs) const {
		return self().to_std() < rhs.to_std();
	}
};

namespace detail {

// Detects generator values.
template <typename T>
struct is_generator_value
	: std::is_base_of<gen_value_base<std::decay_t<T>>, std::decay_t<T>> {};

} // namespace detail

/*
 * Easier printing.
 */

// Struct to print standard types to std::ostream;
struct print {
	std::string s_;

	template <typename T> print(const T &val, char sep = ' ') {
		std::ostringstream oss;
		write(oss, val, sep);
		s_ = oss.str();
	}
	template <typename T>
	print(const std::initializer_list<T> &il, char sep = ' ') {
		std::ostringstream oss;
		write(oss, std::vector<T>(il), sep);
		s_ = oss.str();
	}
	template <typename T>
	print(const std::initializer_list<std::initializer_list<T>> &il,
		  char sep = ' ') {
		std::ostringstream oss;
		std::vector<std::vector<T>> mat;
		for (const auto &i : il)
			mat.push_back(i);
		write(oss, mat, sep);
		s_ = oss.str();
	}

	template <typename T> void write(std::ostream &os, const T &val, char sep) {
		if constexpr (detail::is_pair<T>::value) {
			if constexpr (detail::is_pair_multiline<T>::value) {
				write(os, val.first, sep);
				os << '\n';
				write(os, val.second, sep);
			} else {
				// Use space for inner separator.
				write(os, val.first, ' ');
				os << sep;
				write(os, val.second, ' ');
			}
		} else if constexpr (detail::is_tuple<T>::value)
			write_tuple(os, val, sep);
		else if constexpr (detail::is_container<T>::value)
			write_container(os, val, sep);
		else if constexpr (std::is_same_v<T, detail::i128> or
						   std::is_same_v<T, detail::u128>)
			write_128_number(os, val);
		else
			os << val;
	}

	// Writes 128 bit number.
	template <typename T> void write_128_number(std::ostream &os, T num) {
		static const long long BASE = 1e18;

		if (num < 0) {
			os << '-';
			num = -num;
		}

		if (num >= BASE) {
			write_128_number(os, num / BASE);
			os << std::setw(18) << std::setfill('0')
			   << static_cast<long long>(num % BASE);
		} else
			os << static_cast<long long>(num);
	}
	// Writes container, checking separator.
	template <typename C>
	void write_container(std::ostream &os, const C &container, char sep) {
		bool first = true;

		for (const auto &e : container) {
			if (!first)
				os << (detail::is_container_multiline<C>::value ? '\n' : sep);
			first = false;
			write(os, e, detail::is_container_multiline<C>::value ? sep : ' ');
		}
	}

	// Writes tuple, checking separator.
	template <typename Tuple, size_t... I>
	void write_tuple_impl(std::ostream &os, const Tuple &tp, char sep,
						  std::index_sequence<I...>) {
		bool first = true;
		((os << (first ? (first = false, "")
					   : (detail::is_tuple_multiline<Tuple>::value
							  ? "\n"
							  : std::string(1, sep))),
		  write(os, std::get<I>(tp),
				detail::is_tuple_multiline<Tuple>::value ? sep : ' ')),
		 ...);
	}
	template <typename T>
	void write_tuple(std::ostream &os, const T &tp, char sep) {
		write_tuple_impl(os, tp, sep,
						 std::make_index_sequence<std::tuple_size<T>::value>{});
	}

	friend std::ostream &operator<<(std::ostream &out, const print &pr) {
		return out << pr.s_;
	}
};

// Prints in a new line.
struct println : print {
	template <typename T>
	println(const T &val, char sep = ' ') : print(val, sep) {}
	template <typename T>
	println(const std::initializer_list<T> &il, char sep = ' ')
		: print(il, sep) {}
	template <typename T>
	println(const std::initializer_list<std::initializer_list<T>> &il,
			char sep = ' ')
		: print(il, sep) {}

	friend std::ostream &operator<<(std::ostream &out, const println &pr) {
		return out << pr.s_ << '\n';
	}
};

// Prints container / sequential generator value on its own column.
// Example:
//   A = {1, 2, 3}, B = {4, 2, 5}
//   print_each(A, B) will print:
//  "1 4
//   2 2
//   3 5
//",
//  that is, it prints the end of the line for all lines.
template <typename... Args> struct print_cols {
	std::string s_;

	print_cols(const Args &...args) {
		static_assert(
			((detail::is_container<std::decay_t<Args>>::value or
			  detail::is_sequential<std::decay_t<Args>>::value) and
			 ...),
			"print_cols: arguments must be containers or sequential generator "
			"values");
		std::ostringstream oss;
		write(oss, args...);
		s_ = oss.str();
	}

	void write(std::ostream &os, const Args &...args) {
		auto views = std::apply(
			[](const Args &...inner_args) {
				return std::make_tuple(
					detail::print_cols_view<decltype(inner_args)>{
						inner_args}...);
			},
			std::forward_as_tuple(args...));

		const std::size_t n = std::get<0>(views).size();

		auto check = [&](const auto &v) {
			tgen_ensure(v.size() == n, "print_cols: sizes should be the same");
		};
		std::apply([&](const auto &...v) { (check(v), ...); }, views);

		for (std::size_t i = 0; i < n; ++i) {
			bool first = true;

			std::apply(
				[&](const auto &...v) {
					((os << (first ? "" : " ") << print(v.get(i)),
					  first = false),
					 ...);
				},
				views);

			os << '\n';

			std::apply([](auto &...v) { (v.advance(), ...); }, views);
		}
	}

	friend std::ostream &operator<<(std::ostream &out, const print_cols &pr) {
		return out << pr.s_;
	}
};

/*
 * Global random operations.
 */

namespace detail {

// libstdc++ accepts std::uniform_int_distribution with narrow integral types
// (char/signed char/unsigned char/short/bool), but libc++ rejects them with a
// hard static_assert ("IntType must be a supported integer type"). Promote such
// types to a width the standard guarantees, preserving signedness, so the same
// `next<T>` works across both standard libraries (e.g. Apple clang / libc++).
template <typename T>
using uniform_int_t = std::conditional_t<
	(sizeof(T) >= sizeof(short)), T,
	std::conditional_t<std::is_signed_v<T>, int, unsigned int>>;

} // namespace detail

// Returns a uniformly random number in [0, right)
// O(1).
template <typename T> T next(T right) {
	detail::ensure_registered();
	if constexpr (std::is_integral_v<T>) {
		tgen_ensure(right >= 1, "value for `next` must be valid");
		return static_cast<T>(
			std::uniform_int_distribution<detail::uniform_int_t<T>>(
				0,
				static_cast<detail::uniform_int_t<T>>(right) - 1)(detail::rng));
	} else if constexpr (std::is_floating_point_v<T>) {
		tgen_ensure(right >= 0, "value for `next` must be valid");
		return std::uniform_real_distribution<T>(0, right)(detail::rng);
	} else
		throw detail::error("invalid type for next (" +
							std::string(typeid(T).name()) + ")");
}

// Returns a uniformly random number in [left, right].
// For floating-point types, uses uniform_real_distribution ([left, right) in
// C++), equivalent to [left, right] because the right endpoint has probability
// zero.
// O(1).
template <typename T> T next(T left, T right) {
	detail::ensure_registered();
	tgen_ensure(left <= right, "range for `next` must be valid");
	if constexpr (std::is_integral_v<T>)
		return static_cast<T>(
			std::uniform_int_distribution<detail::uniform_int_t<T>>(
				static_cast<detail::uniform_int_t<T>>(left),
				static_cast<detail::uniform_int_t<T>>(right))(detail::rng));
	else if constexpr (std::is_floating_point_v<T>)
		return std::uniform_real_distribution<T>(left, right)(detail::rng);
	else
		throw detail::error("invalid type for next (" +
							std::string(typeid(T).name()) + ")");
}

// Skewed next.
//
// Returns a random number in [0, right) with a bias controlled by `w`.
// - w = 0:
//     Uniform distribution.
// - w > 0:
//     Returns the maximum of (w + 1) independent uniform samples.
//     Biases the distribution toward larger values.
//     The resulting density is proportional to:
//         f(x) = x^w
//     In particular:
//         w = 1 -> linear bias
//         w = 2 -> quadratic bias
//         w = 3 -> cubic bias
// - w < 0:
//     Returns the minimum of (-w + 1) independent uniform samples.
//     Symmetric to the w > 0 case.
// The continuous version corresponds to Beta distributions:
//     w > 0 -> Beta(w + 1, 1)
//     w < 0 -> Beta(1, -w + 1)
// For |w| > 5, the distribution is approximate.
// O(1).
template <typename T> T wnext(T right, int w) {
	// For small |w|, use the naive approach.
	if (abs(w) <= 5) {
		T val = next<T>(right);
		for (int i = 0; i < w; ++i)
			val = std::max(val, next<T>(right));
		for (int i = 0; i < -w; ++i)
			val = std::min(val, next<T>(right));
		return val;
	}

	// O(1) way.
	double x, r = next<double>(0, 1);

	if (w >= 0) {
		x = std::pow(r, 1.0 / (w + 1));
	} else {
		x = 1.0 - std::pow(r, 1.0 / (-w + 1));
	}

	return T(x * right);
}

// Returns a random number in [left, right] with a bias controlled by `w`.
// O(1).
template <typename T> T wnext(T left, T right, int w) {
	// For small |w|, use the naive approach.
	if (abs(w) <= 5) {
		T val = next<T>(left, right);
		for (int i = 0; i < w; ++i)
			val = std::max(val, next<T>(left, right));
		for (int i = 0; i < -w; ++i)
			val = std::min(val, next<T>(left, right));
		return val;
	}

	// O(1) way.
	double x, r = next<double>(0, 1);

	if (w >= 0) {
		x = std::pow(r, 1.0 / (w + 1));
	} else {
		x = 1.0 - std::pow(r, 1.0 / (-w + 1));
	}

	return left + T(x * (right - left));
}

namespace detail {

// Uniformly random 128 bit number in [0, total).
// O(1) expected.
inline u128 next128(u128 total) {
	tgen_ensure(total > 0, "next128: total must be positive");

	// Largest multiple of total less than 2^128.
	u128 limit = (u128(-1) / total) * total;

	while (true) {
		// Generate uniform 128-bit random number.
		u128 r = (u128(next<uint64_t>(0, std::numeric_limits<uint64_t>::max()))
				  << 64) |
				 next<uint64_t>(0, std::numeric_limits<uint64_t>::max());

		if (r < limit)
			return r % total;
	}
}

} // namespace detail

// Weighted sampler.
//
// Generates indices with probability proportional to `distribution`, using
// alias method.
//
// Internally, integral weights are accumulated in unsigned __int128 (exact);
// floating-point weights are accumulated in double.
// <O(n), O(1)>.
template <typename T> struct weighted_sampler {
	static_assert(std::is_arithmetic_v<T>,
				  "weighted_sampler requires an arithmetic weight type");

	// Internal storage type: `u128` for integral inputs (exact arithmetic),
	// `double` for floating-point inputs.
	using storage_t =
		std::conditional_t<std::is_integral_v<T>, detail::u128, double>;

	int n_;
	std::vector<storage_t> weight_;
	std::vector<int> alias_;
	storage_t total_;

	// Creates an alias method for generating indices with probability
	// proportional to the distribution.
	// O(n).
	weighted_sampler(const std::vector<T> &distribution)
		: n_(distribution.size()), alias_(n_) {
		tgen_ensure(distribution.size() > 0,
					"weighted_sampler: distribution must be non-empty");
		for (const auto &w : distribution)
			tgen_ensure(w >= 0,
						"weighted_sampler: distribution must be non-negative");

		total_ = std::accumulate(distribution.begin(), distribution.end(),
								 storage_t(0));

		std::queue<int> big, small;
		for (int i = 0; i < n_; ++i) {
			weight_.push_back(storage_t(n_) * storage_t(distribution[i]));
			if (weight_[i] < total_)
				small.push(i);
			else
				big.push(i);
		}

		while (!small.empty() and !big.empty()) {
			int s = small.front();
			small.pop();
			int b = big.front();
			big.pop();

			alias_[s] = b;

			weight_[b] -= total_ - weight_[s];
			if (weight_[b] < total_)
				small.push(b);
			else
				big.push(b);
		}

		detail::tgen_ensure_against_bug(
			small.empty(), "weighted_sampler: small must be empty");

		// The remaining elements should have weight equal to total and be
		// assigned to themselves.
		while (!big.empty()) {
			int b = big.front();
			big.pop();
			if constexpr (std::is_integral_v<T>) {
				detail::tgen_ensure_against_bug(
					weight_[b] == total_,
					"weighted_sampler: weight of big element must be total");
			}
			alias_[b] = b;
		}
	}
	weighted_sampler(const std::initializer_list<T> &distribution)
		: weighted_sampler(std::vector<T>(distribution)) {}

	// Uniformly random value in [0, total). Overloaded so next() can dispatch
	// at compile time to the right primitive for the chosen `storage_t`.
	static detail::u128 sample_below(detail::u128 total) {
		return detail::next128(total);
	}
	static double sample_below(double total) {
		return tgen::next<double>(0, total);
	}

	// Generates a random index with probability proportional to the
	// distribution.
	// O(1).
	size_t next() const {
		int i = tgen::next<int>(0, n_ - 1);
		return sample_below(total_) < weight_[i] ? i : alias_[i];
	}
};
template <typename T>
weighted_sampler(const std::vector<T> &) -> weighted_sampler<T>;
template <typename T>
weighted_sampler(const std::initializer_list<T> &) -> weighted_sampler<T>;

// Returns i with probability proportional to distribution[i].
// O(|distribution|).
template <typename T>
size_t next_by_distribution(const std::vector<T> &distribution) {
	return weighted_sampler(distribution).next();
}
template <typename T>
size_t next_by_distribution(const std::initializer_list<T> &distribution) {
	return next_by_distribution(std::vector<T>(distribution));
}

// Returns a vector of k indices with probability proportional to
// `distribution`. Uses alias method.
// O(k + |distribution|).
template <typename T>
std::vector<int> many_by_distribution(int k,
									  const std::vector<T> &distribution) {
	tgen_ensure(distribution.size() > 0, "distribution must be non-empty");
	tgen_ensure(k >= 0, "number of elements to choose must be non-negative");

	weighted_sampler am(distribution);
	std::vector<int> res;
	for (int i = 0; i < k; ++i)
		res.push_back(am.next());
	return res;
}
template <typename T>
std::vector<int>
many_by_distribution(int k, const std::initializer_list<T> &distribution) {
	return many_by_distribution(k, std::vector<T>(distribution));
}

// Shuffles [first, last) inplace uniformly, for RandomAccessIterator.
// O(|container|).
template <typename It> void shuffle(It first, It last) {
	if (first == last)
		return;

	for (It i = first + 1; i != last; ++i)
		std::iter_swap(i, first + next(0, static_cast<int>(i - first)));
}

// Shuffles container uniformly.
// O(|container|).
template <typename C> [[nodiscard]] auto shuffled(const C &container) {
	if constexpr (detail::is_associative_container<C>::value) {
		std::vector<typename C::value_type> vec(container.begin(),
												container.end());
		shuffle(vec.begin(), vec.end());
		return vec;
	} else {
		auto new_container = container;
		shuffle(new_container.begin(), new_container.end());
		return new_container;
	}
}
template <typename T>
[[nodiscard]] std::vector<T> shuffled(const std::initializer_list<T> &il) {
	return shuffled(std::vector<T>(il));
}

// Returns a random element from [first, last) uniformly.
// O(1) for random_access_iterator, O(|last - first|) otherwise.
template <typename It> typename It::value_type pick(It first, It last) {
	int size = std::distance(first, last);
	tgen_ensure(size > 0, "cannot pick from empty range");
	It it = first;
	std::advance(it, next(0, size - 1));
	return *it;
}

// Returns a random element from container uniformly.
// O(1) for random_access_iterator, O(|container|) otherwise.
template <typename C> typename C::value_type pick(const C &container) {
	return pick(container.begin(), container.end());
}
template <typename T> T pick(const std::initializer_list<T> &il) {
	return pick(std::vector<T>(il));
}

// Returns container[i] with probability proportional to distribution[i].
// O(1) for random_access_iterator, O(|container|) otherwise.
template <typename C, typename T>
typename C::value_type pick_by_distribution(const C &container,
											std::vector<T> distribution) {
	tgen_ensure(container.size() == distribution.size(),
				"container and distribution must have the same size");
	auto it = container.begin();
	std::advance(it, next_by_distribution(distribution));
	return *it;
}
template <typename C, typename T>
typename C::value_type
pick_by_distribution(const C &container,
					 const std::initializer_list<T> &distribution) {
	return pick_by_distribution(container, std::vector<T>(distribution));
}
template <typename T, typename U>
T pick_by_distribution(const std::initializer_list<T> &il,
					   const std::vector<U> &distribution) {
	return pick_by_distribution(std::vector<T>(il), distribution);
}
template <typename T, typename U>
T pick_by_distribution(const std::initializer_list<T> &il,
					   const std::initializer_list<U> &distribution) {
	return pick_by_distribution(std::vector<T>(il),
								std::vector<U>(distribution));
}

// Chooses k values uniformly from container, as in a subsequence of size k.
// Returns a copy. O(|container|).
template <typename C> C choose(const C &container, int k) {
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
std::vector<T> choose(const std::initializer_list<T> &il, int k) {
	return choose(std::vector<T>(il), k);
}

// Number distinct generator for integral types.
// Optimized for performance (unordered_map virtual list; gen_list uses array
// pool, complement, or sparse sampling).
template <typename T> struct distinct_range {
	T left_, right_;
	T num_available_;
	std::unordered_map<T, T> virtual_list_;

	// When the range fits in memory, sample via array Fisher–Yates.
	static constexpr size_t array_pool_max = size_t{1} << 23;

	// Generator of distinct values in [left, right].
	distinct_range(T left, T right)
		: left_(left), right_(right), num_available_(right - left + 1) {}

	// Returns the number of distinct values left to generate.
	T size() const { return num_available_; }

	// Generates a random value in [left_, right_] that has not been generated
	// yet.
	// O(log n).
	T gen() {
		// One iteration of Fisher–Yates.
		tgen_ensure(size() > 0, "distinct_range: no more values to generate");

		T i = next<T>(0, size() - 1);
		T j = size() - 1;

		auto vi_it = virtual_list_.find(i);
		T vi = vi_it == virtual_list_.end() ? i : vi_it->second;
		auto vj_it = virtual_list_.find(j);
		T vj = vj_it == virtual_list_.end() ? j : vj_it->second;
		virtual_list_[i] = vj;

		--num_available_;

		return vi + left_;
	}

	// Generates a list of distinct values.
	// Optimized for performance (array pool, complement, or sparse sampling).
	// O(size) when the range fits in memory; O(size log range) otherwise.
	auto gen_list(int count) {
		tgen_ensure(count >= 0, "distinct_range: size must be nonnegative");
		tgen_ensure(count <= num_available_,
					"distinct_range: no more values to generate");

		size_t range_size = right_ - left_ + 1;
		size_t sample_count = count;

		std::vector<T> res;
		if (sample_count > 0) {
			if (range_size <= array_pool_max)
				res = sample_from_pool(sample_count, range_size);
			else if (sample_count * 2 > range_size)
				res = sample_complement(sample_count, range_size);
			else
				res = sample_sparse(sample_count);
		}

		num_available_ -= count;
		virtual_list_.clear();
		return typename list<T>::value(res);
	}

	// Generates all distinct values.
	// O(n) when the range fits in memory; O(n log n) otherwise.
	auto gen_all() { return gen_list(size()); }

  private:
	// Samples count distinct values via array Fisher–Yates on [left_, right_].
	// O(range_size) time and memory.
	std::vector<T> sample_from_pool(size_t count, size_t range_size) {
		std::vector<T> pool(range_size);
		std::iota(pool.begin(), pool.end(), left_);
		for (size_t i = 0; i < count; ++i) {
			size_t j = next<size_t>(i, range_size - 1);
			std::swap(pool[i], pool[j]);
		}
		pool.resize(count);
		return pool;
	}

	// Samples count distinct values by excluding range_size - count values.
	// O(range_size + (range_size - count) log(range_size)).
	std::vector<T> sample_complement(size_t count, size_t range_size) {
		size_t exclude_count = range_size - count;
		std::unordered_set<T> excluded;
		excluded.reserve(exclude_count * 2);

		if (exclude_count <= array_pool_max) {
			for (T value : sample_from_pool(exclude_count, range_size))
				excluded.insert(value);
		} else {
			for (T value : sample_sparse(exclude_count))
				excluded.insert(value);
		}

		std::vector<T> res;
		res.reserve(count);
		for (T value = left_; value <= right_; ++value) {
			if (!excluded.count(value))
				res.push_back(value);
		}
		detail::tgen_ensure_against_bug(
			res.size() == count, "distinct_range: complement sampling failed");
		return res;
	}

	// Samples count distinct values via sparse-map Fisher–Yates.
	// O(count log(range_size)).
	std::vector<T> sample_sparse(size_t count) {
		std::unordered_map<T, T> local_virtual;
		local_virtual.reserve(count * 2);
		T remaining = range_span();
		std::vector<T> res;
		res.reserve(count);
		for (size_t step = 0; step < count; ++step) {
			T i = next<T>(0, remaining - 1);
			T j = remaining - 1;

			auto vi_it = local_virtual.find(i);
			T vi = vi_it == local_virtual.end() ? i : vi_it->second;
			auto vj_it = local_virtual.find(j);
			T vj = vj_it == local_virtual.end() ? j : vj_it->second;
			local_virtual[i] = vj;

			res.push_back(vi + left_);
			--remaining;
		}
		return res;
	}

	// Returns right_ - left_ + 1.
	// O(1).
	T range_span() { return right_ - left_ + 1; }
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
		: distinct_container(std::vector<T>(il)) {}

	// Returns the number of distinct elements left to generate.
	size_t size() const { return idx_.size(); }

	// Generates a random element from container uniformly.
	// O(log n).
	T gen() { return list_[idx_.gen()]; }

	// Generates a list of distinct values.
	// O(size * log(n)).
	auto gen_list(int size) {
		std::vector<T> res;
		for (int i = 0; i < size; ++i)
			res.push_back(gen());
		return typename list<T>::value(res);
	}

	// Generates all distinct values.
	// O(n log(n))
	auto gen_all() {
		std::vector<T> res;
		while (size() > 0)
			res.push_back(gen());
		return typename list<T>::value(res);
	}
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
 * Named options are given in one of the following formats:
 * 1) -keyname=value or --keyname=value (ex. -n=10   , --test-count=20)
 * 2) -keyname value or --keyname value (ex. -n 10   , --test-count 20)
 *
 * Positional options are numbered from 0 sequentially.
 * For example, for "10 -n=20 str" positional option 1 is the string "str".
 */

/*
 * C++ version selection.
 */

// Sets C++ version.
inline void set_cpp_version(int version) {
	detail::cpp = detail::cpp_value(version);
}

/*
 * Compiler selection.
 */

// GCC compiler type.
inline compiler_value gcc(int major = 0, int minor = 0) {
	return {compiler_kind::gcc, major, minor};
}

// Clang compiler type.
inline compiler_value clang(int major = 0, int minor = 0) {
	return {compiler_kind::clang, major, minor};
}

// Sets compiler.
inline void set_compiler(compiler_value compiler) {
	detail::compiler.kind_ = compiler.kind_;
	detail::compiler.major_ = compiler.major_;
	detail::compiler.minor_ = compiler.minor_;
}

namespace detail {

// Processes special opt flags.
// Returns true if the key is a special opt flag.
inline bool process_special_opt_flags(std::string &key) {
	// Checks for gen::CPP=17|20|23
	if (key.find("tgen::CPP:") == 0) {
		int prefix_len = std::string("tgen::CPP:").size();
		tgen_ensure(static_cast<int>(key.size()) == prefix_len + 2 and
						std::isdigit(key[prefix_len]) and
						std::isdigit(key[prefix_len + 1]),
					"invalid CPP format");
		int version = std::stoi(key.substr(prefix_len, 2));
		set_cpp_version(version);
		return true;
	}

	// Checks for tgen::(GCC|CLANG) or
	// tgen::(GCC|CLANG):(version|version.minor).
	compiler_kind kind;
	size_t prefix_len = 0;

	if (key.find("tgen::GCC") == 0) {
		kind = compiler_kind::gcc;
		prefix_len = std::string("tgen::GCC").size();
	} else if (key.find("tgen::CLANG") == 0) {
		kind = compiler_kind::clang;
		prefix_len = std::string("tgen::CLANG").size();
	} else {
		return false;
	}

	if (key.size() == prefix_len) {
		set_compiler(compiler_value(kind, 0, 0));
		return true;
	}

	tgen_ensure(key[prefix_len] == ':', "invalid compiler format");
	++prefix_len; // for ':'.

	std::string inside = key.substr(prefix_len, key.size() - prefix_len);
	int major = 0, minor = 0;

	size_t dot = inside.find('.');
	if (dot == std::string::npos) {
		tgen_ensure(!inside.empty() and
						std::all_of(inside.begin(), inside.end(), ::isdigit),
					"invalid compiler version");
		major = std::stoi(inside);
	} else {
		std::string maj = inside.substr(0, dot);
		std::string min = inside.substr(dot + 1);

		tgen_ensure(!maj.empty() and
						std::all_of(maj.begin(), maj.end(), ::isdigit) and
						maj.size() <= 3,
					"invalid compiler major version");
		tgen_ensure(!min.empty() and
						std::all_of(min.begin(), min.end(), ::isdigit) and
						min.size() <= 3,
					"invalid compiler minor version");

		major = std::stoi(maj);
		minor = std::stoi(min);
	}

	set_compiler(compiler_value(kind, major, minor));

	return true;
}

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
			return value; // Default: std::string.
	} catch (...) {
	}

	throw error("invalid value `" + value + "` for type " + typeid(T).name());
}

inline void parse_opts(int argc, char **argv) {
	// Parses the opts into `pos_opts` vector and `named_opts`
	// map. Starting from 1 to ignore the name of the executable.
	for (int i = 1; i < argc; ++i) {
		std::string key(argv[i]);

		if (process_special_opt_flags(key))
			continue;

		if (key[0] == '-') {
			tgen_ensure(key.size() > 1,
						"invalid opt (" + std::string(argv[i]) + ")");
			if ('0' <= key[1] and key[1] <= '9') {
				// This case is a positional negative number argument.
				pos_opts.push_back(key);
				continue;
			}

			// Pops first char '-'.
			key = key.substr(1);
		} else {
			// This case is a positional argument that does not start with '-'.
			pos_opts.push_back(key);
			continue;
		}

		// Pops a possible second char '-'.
		if (key[0] == '-') {
			tgen_ensure(key.size() > 1,
						"invalid opt (" + std::string(argv[i]) + ")");

			// Pops first char '-'.
			key = key.substr(1);
		}

		// Assumes that, if it starts with '-' and second char is not a digit,
		// then it is a <key, value> pair.
		// 1 or 2 chars '-' have already been popped.

		std::size_t eq = key.find('=');
		if (eq != std::string::npos) {
			// This is the '--key=value' case.
			std::string value = key.substr(eq + 1);
			key = key.substr(0, eq);
			tgen_ensure(!key.empty() and !value.empty(),
						"expected non-empty key/value in opt (" +
							std::string(argv[i]) + ")");
			tgen_ensure(named_opts.count(key) == 0,
						"cannot have repeated keys");
			named_opts[key] = value;
		} else {
			// This is the '--key value' case.
			tgen_ensure(named_opts.count(key) == 0,
						"cannot have repeated keys");
			tgen_ensure(argv[i + 1], "value cannot be empty");
			named_opts[key] = std::string(argv[i + 1]);
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
	rng.seed(seq);
}

} // namespace detail

// Returns true if there is an opt at a given index.
inline bool has_opt(std::size_t index) {
	detail::ensure_registered();
	return index < detail::pos_opts.size();
}

// Returns true if there is an opt with a given key.
inline bool has_opt(const std::string &key) {
	detail::ensure_registered();
	return detail::named_opts.count(key) != 0;
}
template <typename K>
std::enable_if_t<std::is_same_v<K, char>, bool> has_opt(K key) {
	return has_opt(std::string(1, key));
}

// Returns the parsed opt by a given index. If no opts with the given index are
// found, returns the given default_value.
template <typename T>
T opt(size_t index, std::optional<T> default_value = std::nullopt) {
	detail::ensure_registered();
	if (!has_opt(index)) {
		if (default_value)
			return *default_value;
		throw detail::error("cannot find opt at index " +
							std::to_string(index));
	}
	return detail::get_opt<T>(detail::pos_opts[index]);
}

// Returns the parsed opt by a given key. If no opts with the given key are
// found, returns the given default_value.
template <typename T>
T opt(const std::string &key, std::optional<T> default_value = std::nullopt) {
	detail::ensure_registered();
	if (!has_opt(key)) {
		if (default_value)
			return *default_value;
		throw detail::error("cannot find opt with key " + key);
	}
	return detail::get_opt<T>(detail::named_opts[key]);
}
template <typename T, typename K>
std::enable_if_t<std::is_same_v<K, char>, T>
opt(K key, std::optional<T> default_value = std::nullopt) {
	return opt<T>(std::string(1, key), default_value);
}

// Registers generator by initializing rng and parsing opts.
inline void register_gen(int argc, char **argv) {
	detail::set_seed(argc, argv);

	detail::pos_opts.clear();
	detail::named_opts.clear();
	detail::parse_opts(argc, argv);

	detail::registered = true;
}

// Registers generator by initializing rng with a given seed.
inline void register_gen(std::optional<long long> seed = std::nullopt) {
	if (seed)
		detail::rng.seed(*seed);
	else
		detail::rng.seed();

	detail::pos_opts.clear();
	detail::named_opts.clear();

	detail::registered = true;
}

/************
 *          *
 *   LIST   *
 *          *
 ************/

/*
 * List generator.
 *
 * List of integral types.
 */

template <typename T> struct list : gen_base<list<T>> {
	int size_;			  // Size of list.
	T value_l_, value_r_; // Range of defined values.
	std::set<T> values_;  // Set of values. If empty, use range; if not,
						  // represents the possible values, and the range
						  // represents the index in this set.
	std::map<T, int>
		value_idx_in_set_; // Index of every value in the set above.
	mutable std::vector<std::pair<T, T>>
		val_range_; // Range of values of each index.
	mutable std::vector<std::vector<int>> neigh_; // Adjacency list of equality.
	std::vector<std::set<int>>
		diff_restrictions_; // All different restrictions.
	bool index_constraints_{
		false}; // True after fix/equal narrows per-index generation.
	mutable bool uses_full_range_{
		false}; // If true, every index uses [value_l_, value_r_] lazily.

	// Creates generator for lists of size 'size', with random T in [value_left,
	// value_right].
	list(int size, T value_left, T value_right)
		: size_(size), value_l_(value_left), value_r_(value_right),
		  uses_full_range_(true) {
		tgen_ensure(size_ > 0, "list: size must be positive");
		tgen_ensure(value_l_ <= value_r_, "list: value range must be valid");
	}

	// Creates list with value set.
	list(int size, std::set<T> values)
		: size_(size), values_(values), index_constraints_(true) {
		tgen_ensure(size_ > 0, "list: size must be positive");
		tgen_ensure(!values.empty(), "list: value set must be non-empty");
		value_l_ = 0, value_r_ = values.size() - 1;
		val_range_.assign(size_, {value_l_, value_r_});
		int idx = 0;
		for (T val : values_)
			value_idx_in_set_[val] = idx++;
	}

	// Restricts lists for list[idx] = val.
	list &fix(int idx, T val) {
		tgen_ensure(0 <= idx and idx < size_, "list: index must be valid");
		ensure_val_range_materialized();
		if (values_.size() == 0) {
			auto &[left, right] = val_range_[idx];
			if (left == right and value_l_ != value_r_) {
				tgen_ensure(left == val,
							"list: must not set to two different values");
			} else {
				tgen_ensure(left <= val and val <= right,
							"list: value must be in the defined range");
			}
			left = right = val;
		} else {
			tgen_ensure(values_.count(val),
						"list: value must be in the set of values");
			auto &[left, right] = val_range_[idx];
			int new_val = value_idx_in_set_[val];
			tgen_ensure(left <= new_val and new_val <= right,
						"list: must not set to two different values");
			left = right = new_val;
		}
		index_constraints_ = true;
		return *this;
	}

	// Restricts lists for list[idx_1] = list[idx_2].
	list &equal(int idx_1, int idx_2) {
		tgen_ensure(0 <= std::min(idx_1, idx_2) and
						std::max(idx_1, idx_2) < size_,
					"list: indices must be valid");
		if (idx_1 == idx_2)
			return *this;

		ensure_val_range_materialized();
		ensure_neigh_allocated();
		index_constraints_ = true;
		neigh_[idx_1].push_back(idx_2);
		neigh_[idx_2].push_back(idx_1);
		return *this;
	}

	// Restricts lists for list[S] to be equal, for given subset S of indices.
	list &equal(std::set<int> indices) {
		if (!indices.empty()) {
			std::set<int>::iterator beg = indices.begin();
			for (auto it = std::next(beg); it != indices.end(); ++it)
				equal(*beg, *it);
		}
		return *this;
	}

	// Restricts lists for list[left..right] to have all equal values.
	list &equal_range(int left, int right) {
		tgen_ensure(0 <= left and left <= right and right < size_,
					"list: range indices must be valid");
		for (int i = left; i < right; ++i)
			equal(i, i + 1);
		return *this;
	}

	// Restricts lists for all equal elements.
	list &all_equal() { return equal_range(0, size_ - 1); }

	// Restricts lists for list[S] to be different (distinct), for given subset
	// S of indices. You cannot add two of these restrictions on sets that
	// intersect.
	list &different(std::set<int> indices) {
		if (!indices.empty())
			diff_restrictions_.push_back(indices);
		return *this;
	}

	// Restricts lists for list[idx_1] != list[idx_2].
	list &different(int idx_1, int idx_2) {
		std::set<int> indices = {idx_1, idx_2};
		return different(indices);
	}

	// Restricts lists for list[left..right] to have all different values.
	list &different_range(int left, int right) {
		tgen_ensure(0 <= left and left <= right and right < size_,
					"list: range indices must be valid");
		std::vector<int> indices(right - left + 1);
		std::iota(indices.begin(), indices.end(), left);
		return different(std::set<int>(indices.begin(), indices.end()));
	}

	// Restricts lists for all different elements.
	list &all_different() {
		std::vector<int> indices(size_);
		std::iota(indices.begin(), indices.end(), 0);
		return different(std::set<int>(indices.begin(), indices.end()));
	}

	// List value.
	// Operations on a value are not random.
	struct value : gen_value_base<value> {
		using tgen_is_sequential_tag = detail::is_sequential_tag;

		using value_type = T;			 // Value type, for templates.
		using std_type = std::vector<T>; // std type for value.

		std::vector<T> vec_; // list.
		char sep_;			 // Separator for printing.

		value(const std::vector<T> &vec) : vec_(vec), sep_(' ') {}
		value(const std::initializer_list<T> &il) : value(std::vector<T>(il)) {}

		// Fetches size.
		int size() const { return vec_.size(); }

		// Fetches position idx.
		T &operator[](int idx) {
			tgen_ensure(0 <= idx and idx < size(),
						"list: value: index out of bounds");
			return vec_[idx];
		}
		const T &operator[](int idx) const {
			tgen_ensure(0 <= idx and idx < size(),
						"list: value: index out of bounds");
			return vec_[idx];
		}

		// Sorts values in non-decreasing order.
		// O(n log n).
		value &sort() {
			std::sort(vec_.begin(), vec_.end());
			return *this;
		}

		// Reverses list.
		// O(n).
		value &reverse() {
			std::reverse(vec_.begin(), vec_.end());
			return *this;
		}

		// Sets the separator for the list, for printing.
		// O(1).
		value &separator(char sep) {
			sep_ = sep;
			return *this;
		}

		// Concatenates two values.
		// Linear.
		value operator+(const value &rhs) const {
			std::vector<T> new_vec = vec_;
			for (int i = 0; i < rhs.size(); ++i)
				new_vec.push_back(rhs[i]);
			return value(new_vec);
		}

		// Shuffles list uniformly.
		// O(n).
		value &shuffle() {
			for (int i = 0; i < size(); ++i)
				std::swap(vec_[i], vec_[next(0, size() - 1)]);
			return *this;
		}

		// Returns a random element uniformly.
		// O(1).
		T pick() const { return vec_[next<int>(0, size() - 1)]; }

		// Returns vec_[i] with probability proportional to distribution[i].
		// O(1).
		template <typename Dist>
		T pick_by_distribution(const std::vector<Dist> &distribution) const {
			tgen_ensure(static_cast<size_t>(size()) == distribution.size(),
						"value and distribution must have the same size");
			return vec_[next_by_distribution(distribution)];
		}
		template <typename Dist>
		T pick_by_distribution(
			const std::initializer_list<Dist> &distribution) const {
			return pick_by_distribution(std::vector<Dist>(distribution));
		}

		// Chooses k values uniformly, as in a subsequence of size k.
		// O(n).
		value choose(int k) const {
			tgen_ensure(0 < k and k <= size(),
						"number of elements to choose must be valid");
			std::vector<T> new_vec;
			int need = k;
			for (int i = 0; need > 0; ++i) {
				int left = size() - i;
				if (next(1, left) <= need) {
					new_vec.push_back(vec_[i]);
					need--;
				}
			}
			return value(new_vec);
		}

		// Prints to std::ostream, separated by sep_.
		friend std::ostream &operator<<(std::ostream &out, const value &val) {
			for (int i = 0; i < val.size(); ++i) {
				if (i > 0)
					out << val.sep_;
				out << val[i];
			}
			return out;
		}

		// Gets a std::vector representing the value.
		auto to_std() const {
			if constexpr (!detail::is_generator_value<T>::value) {
				return vec_;
			} else {
				std::vector<typename T::std_type> vec;
				for (const auto &i : vec_)
					vec.push_back(i.to_std());
				return vec;
			}
		}
	};

	// Generates list value.
	// Optimized for performance (unconstrained and all-different fast paths).
	// O(n log n).
	value gen() const {
		if (diff_restrictions_.empty()) {
			if (auto unconstrained = try_gen_unconstrained())
				return *unconstrained;
		}
		if (auto all_different = try_gen_all_different())
			return *all_different;

		ensure_neigh_allocated();
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
						auto [l, r] = val_range_at(cur_idx);
						if (l == r) {
							if (!value_defined) {
								// We found the value.
								value_defined = true;
								new_value = l;
							} else if (new_value != l) {
								// We found a contradiction
								throw detail::contradiction_error(
									"list",
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

		// Initial parsing of different restrictions.
		std::vector<std::set<int>> diff_containing_comp_idx(comp_count);
		{
			int dist_id = 0;
			for (const std::set<int> &diff : diff_restrictions_) {
				// Checks if there are too many different values.
				if (static_cast<uint64_t>(diff.size() - 1) +
						static_cast<uint64_t>(value_l_) >
					static_cast<uint64_t>(value_r_))
					throw detail::contradiction_error(
						"list", "tried to generate " +
									std::to_string(diff.size()) +
									" different values, but the maximum is " +
									std::to_string(value_r_ - value_l_ + 1));

				// Checks if two values in same component are marked as
				// different.
				std::set<int> comp_ids;
				for (int idx : diff) {
					if (comp_ids.count(comp_id[idx]))
						throw detail::contradiction_error(
							"list", "tried to set two indices as equal and "
									"different");
					comp_ids.insert(comp_id[idx]);

					diff_containing_comp_idx[comp_id[idx]].insert(dist_id);
				}
				++dist_id;
			}
		}

		// If some value is in >= 3 sets, then there is a cycle.
		for (auto &diff_containing : diff_containing_comp_idx)
			if (diff_containing.size() >= 3)
				throw detail::complex_restrictions_error(
					"list",
					"one index cannot be in >= 3 'different' restrictions");

		std::vector<bool> vis_diff(diff_restrictions_.size(), false);
		std::vector<bool> initially_defined_comp_idx(comp_count, false);

		// Fills the value in a tree defined by "different" restrictions.
		auto define_tree = [&](int diff_id) {
			// The set `diff_restrictions_[diff_id]` can have some
			// values that are defined.

			// Generates set of already defined values.
			std::set<T> defined_values;
			for (int idx : diff_restrictions_[diff_id])
				if (defined_idx[idx]) {
					// Checks if two values in `diff_restrictions_[dist_id]`
					// have been set to the same value
					if (defined_values.count(vec[idx]))
						throw detail::contradiction_error(
							"list",
							"tried to set two indices as equal and different");

					defined_values.insert(vec[idx]);
				}

			// Generates values in this root "different" restriction.
			{
				int new_value_count = diff_restrictions_[diff_id].size() -
									  static_cast<int>(defined_values.size());
				std::vector<T> generated_values =
					generate_distinct_values(new_value_count, defined_values);
				auto val_it = generated_values.begin();
				for (int idx : diff_restrictions_[diff_id])
					if (defined_idx[idx]) {
						// The root can cover these components, but there should
						// not be any other defined in this tree.
						initially_defined_comp_idx[comp_id[idx]] = false;
					} else {
						define_comp(comp_id[idx], *val_it);
						++val_it;
					}
			}

			// BFS on the tree of "different" restrictions.
			std::queue<std::pair<int, int>> q; // {id, parent id}
			q.emplace(diff_id, -1);
			vis_diff[diff_id] = true;
			while (!q.empty()) {
				auto [cur_diff, parent] = q.front();
				q.pop();

				std::set<int> neigh_diff;
				for (int idx : diff_restrictions_[cur_diff])
					for (int nxt_diff :
						 diff_containing_comp_idx[comp_id[idx]]) {
						if (nxt_diff == cur_diff or nxt_diff == parent)
							continue;

						// Cycle found.
						if (vis_diff[nxt_diff])
							throw detail::complex_restrictions_error(
								"list",
								"cycle found in 'different' restrictions");

						neigh_diff.insert(nxt_diff);
					}

				for (int nxt_diff : neigh_diff) {
					vis_diff[nxt_diff] = true;
					q.emplace(nxt_diff, cur_diff);

					// Generates this "different" restriction.
					std::set<T> nxt_defined_values;
					for (int idx2 : diff_restrictions_[nxt_diff])
						if (defined_idx[idx2]) {
							// There cannot be any more defined. This case is
							// when there are values not covered by a single
							// "different" restriction in the tree.
							if (initially_defined_comp_idx[comp_id[idx2]])
								throw detail::complex_restrictions_error(
									"list");

							nxt_defined_values.insert(vec[idx2]);
						}
					int new_value_count =
						diff_restrictions_[nxt_diff].size() -
						static_cast<int>(nxt_defined_values.size());
					std::vector<T> generated_values = generate_distinct_values(
						new_value_count, nxt_defined_values);
					auto val_it = generated_values.begin();
					for (int idx2 : diff_restrictions_[nxt_diff])
						if (!defined_idx[idx2]) {
							define_comp(comp_id[idx2], *val_it);
							++val_it;
						}
				}
			}
		};

		// Loops through "different" restrictions, sorts "different"
		// restrictions by number of defined components (non-increasing). This
		// guarantees that if there is a valid root (that covers all 'defined'),
		// we find it.
		{
			std::vector<std::pair<int, int>> defined_cnt_and_diff_idx;
			int dist_id = 0;
			for (const std::set<int> &diff : diff_restrictions_) {
				int defined_cnt = 0;
				for (int idx : diff)
					if (defined_idx[idx]) {
						++defined_cnt;
						initially_defined_comp_idx[comp_id[idx]] = true;
					}
				defined_cnt_and_diff_idx.emplace_back(defined_cnt, dist_id);
				++dist_id;
			}

			std::sort(defined_cnt_and_diff_idx.rbegin(),
					  defined_cnt_and_diff_idx.rend());
			for (auto [defined_cnt, diff_idx] : defined_cnt_and_diff_idx)
				if (!vis_diff[diff_idx])
					define_tree(diff_idx);
		}

		// Loops through "different" restrictions do define the rest.
		for (std::size_t dist_id = 0; dist_id < diff_restrictions_.size();
			 ++dist_id)
			if (!vis_diff[dist_id])
				define_tree(dist_id);

		// Define final values. These values all should be random in [l, r], and
		// the "different" restrictions have already been processed. However,
		// there can be still equality restrictions, so we define entire
		// components.
		for (int idx = 0; idx < size_; ++idx)
			if (!defined_idx[idx])
				define_comp(comp_id[idx], next<T>(value_l_, value_r_));

		if (!values_.empty()) {
			// Needs to fetch the values from the value set.
			std::vector<T> value_vec(values_.begin(), values_.end());
			for (T &val : vec)
				val = value_vec[val];
		}

		return value(vec);
	}

  private:
	// Materializes neigh_ after the first equality restriction.
	void ensure_neigh_allocated() const {
		if (neigh_.size() == static_cast<size_t>(size_))
			return;
		neigh_.assign(size_, {});
	}

	// Materializes val_range_ after the first per-index restriction.
	void ensure_val_range_materialized() const {
		if (!uses_full_range_)
			return;
		val_range_.assign(size_, {value_l_, value_r_});
		uses_full_range_ = false;
	}

	// Returns the allowed value range at index idx.
	std::pair<T, T> val_range_at(int idx) const {
		if (uses_full_range_)
			return {value_l_, value_r_};
		return val_range_[idx];
	}

	// Generates a uniformly random list of k distinct values in `[value_l,
	// value_r]`, such that no value is in `forbidden_values`.
	std::vector<T>
	generate_distinct_values(int k, const std::set<T> &forbidden_values) const {
		for (auto forbidden : forbidden_values)
			tgen_ensure(value_l_ <= forbidden and forbidden <= value_r_);
		const T num_available =
			(value_r_ - value_l_ + 1) - forbidden_values.size();
		if (num_available < k)
			throw detail::complex_restrictions_error(
				"list", "not enough distinct values");
		if (forbidden_values.empty())
			return distinct_range<T>(value_l_, value_r_).gen_list(k).to_std();

		std::map<T, T> virtual_list;
		std::vector<T> gen_list;
		for (int i = 0; i < k; ++i) {
			T j = next<T>(i, num_available - 1);
			T vj = virtual_list.count(j) ? virtual_list[j] : j;
			T vi = virtual_list.count(i) ? virtual_list[i] : i;

			virtual_list[j] = vi, virtual_list[i] = vj;

			gen_list.push_back(virtual_list[i]);
		}

		for (T &val : gen_list)
			val += value_l_;

		std::vector<std::pair<T, int>> values_sorted;
		for (std::size_t i = 0; i < gen_list.size(); ++i)
			values_sorted.emplace_back(gen_list[i], i);
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

	// If this generator has no constraints beyond [value_l_, value_r_],
	// returns independent uniform samples; otherwise returns std::nullopt.
	// O(n).
	std::optional<value> try_gen_unconstrained() const {
		if (!values_.empty() or index_constraints_)
			return std::nullopt;

		std::vector<T> vec(size_);
		for (int i = 0; i < size_; ++i)
			vec[i] = next<T>(value_l_, value_r_);
		return value(vec);
	}

	// If this generator is exactly all-distinct in [value_l_, value_r_],
	// returns a uniformly random list; otherwise returns std::nullopt.
	// Optimized for performance (distinct_range fast path).
	// O(n log n).
	std::optional<value> try_gen_all_different() const {
		if (!values_.empty() or diff_restrictions_.size() != 1)
			return std::nullopt;

		const std::set<int> &diff = diff_restrictions_[0];
		if (static_cast<int>(diff.size()) != size_ or *diff.begin() != 0 or
			*diff.rbegin() != size_ - 1)
			return std::nullopt;

		if (!neigh_.empty()) {
			for (const auto &adj : neigh_) {
				if (!adj.empty())
					return std::nullopt;
			}
		}

		if (index_constraints_)
			return std::nullopt;

		if (static_cast<long long>(size_) >
			static_cast<long long>(value_r_) - value_l_ + 1)
			throw detail::contradiction_error(
				"list", "tried to generate " + std::to_string(size_) +
							" different values, but the maximum is " +
							std::to_string(value_r_ - value_l_ + 1));

		return distinct_range<T>(value_l_, value_r_).gen_list(size_);
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

struct permutation : gen_base<permutation> {
	int size_;									  // Size of permutation.
	std::vector<std::pair<int, int>> defs_;		  // {idx, value}.
	std::optional<std::vector<int>> cycle_sizes_; // Cycle sizes.

	// Creates generator for permutation of size 'size'.
	permutation(int size) : size_(size) {
		tgen_ensure(size_ > 0, "permutation: size must be positive");
	}

	// Restricts permutations for permutation[idx] = val.
	permutation &fix(int idx, int val) {
		tgen_ensure(0 <= idx and idx < size_,
					"permutation: index must be valid");
		defs_.emplace_back(idx, val);
		return *this;
	}

	// Restricts permutations for permutation to have cycle sizes.
	permutation &cycles(const std::vector<int> &cycle_sizes) {
		tgen_ensure(
			size_ == std::accumulate(cycle_sizes.begin(), cycle_sizes.end(), 0),
			"permutation: cycle sizes must add up to size of permutation");
		cycle_sizes_ = cycle_sizes;
		return *this;
	}
	permutation &cycles(const std::initializer_list<int> &cycle_sizes) {
		return cycles(std::vector<int>(cycle_sizes));
	}

	// Permutation value.
	// Operations on a value are not random.
	struct value : gen_value_base<value> {
		using tgen_is_sequential_tag = detail::is_sequential_tag;

		using std_type = std::vector<int>; // std type for value.
		std::vector<int> vec_;			   // Permutation.
		char sep_;						   // Separator for printing.
		bool add_1_;					   // If should add 1, for printing.

		value(const std::vector<int> &vec)
			: vec_(vec), sep_(' '), add_1_(false) {
			tgen_ensure(!vec_.empty(), "permutation: value: cannot be empty");
			std::vector<bool> vis(vec_.size(), false);
			for (int i = 0; i < size(); ++i) {
				tgen_ensure(0 <= vec_[i] and
								vec_[i] < static_cast<int>(vec_.size()),
							"permutation: value: values must be from `0` to "
							"`size-1`");
				tgen_ensure(!vis[vec_[i]],
							"permutation: value: cannot have repeated values");
				vis[vec_[i]] = true;
			}
		}
		value(const std::initializer_list<int> &il)
			: value(std::vector<int>(il)) {}

		// Fetches size.
		int size() const { return vec_.size(); }

		// Fetches position idx.
		const int &operator[](int idx) const {
			tgen_ensure(0 <= idx and idx < size(),
						"permutation: value: index out of bounds");
			return vec_[idx];
		}

		// Returns parity of the permutation (+1 if even, -1 if odd).
		// O(n).
		int parity() const {
			std::vector<bool> vis(size(), false);
			int cycles = 0;

			for (int i = 0; i < size(); ++i)
				if (!vis[i]) {
					++cycles;
					for (int j = i; !vis[j]; j = vec_[j])
						vis[j] = true;
				}
			// Even iff (n - cycles) is even.
			return ((size() - cycles) % 2 == 0) ? +1 : -1;
		}

		// Sorts values in increasing order.
		// O(n).
		value &sort() {
			for (int i = 0; i < size(); ++i)
				vec_[i] = i;
			return *this;
		}

		// Reverses permutation.
		// O(n).
		value &reverse() {
			std::reverse(vec_.begin(), vec_.end());
			return *this;
		}

		// Inverse of the permutation.
		// O(n).
		value &inverse() {
			std::vector<int> inv(size());
			for (int i = 0; i < size(); ++i)
				inv[vec_[i]] = i;
			swap(vec_, inv);
			return *this;
		}

		// Sets the separator, for printing.
		// O(1).
		value &separator(char sep) {
			sep_ = sep;
			return *this;
		}

		// Sets that should print values 1-based.
		// O(1).
		value &add_1() {
			add_1_ = true;
			return *this;
		}

		// Shuffles permutation uniformly.
		// O(n).
		value &shuffle() {
			for (int i = 0; i < size(); ++i)
				std::swap(vec_[i], vec_[next(0, size() - 1)]);
			return *this;
		}

		// Returns a random element uniformly.
		// O(1).
		int pick() const { return vec_[next<int>(0, size() - 1)]; }

		// Returns vec_[i] with probability proportional to distribution[i].
		// O(1).
		template <typename Dist>
		int pick_by_distribution(const std::vector<Dist> &distribution) const {
			tgen_ensure(static_cast<size_t>(size()) == distribution.size(),
						"value and distribution must have the same size");
			return vec_[next_by_distribution(distribution)];
		}
		template <typename Dist>
		int pick_by_distribution(
			const std::initializer_list<Dist> &distribution) const {
			return pick_by_distribution(std::vector<Dist>(distribution));
		}

		// Prints to std::ostream, separated by sep_.
		friend std::ostream &operator<<(std::ostream &out, const value &val) {
			for (int i = 0; i < val.size(); ++i) {
				if (i > 0)
					out << val.sep_;
				out << val[i] + val.add_1_;
			}
			return out;
		}

		// Gets a std::vector representing the value.
		std::vector<int> to_std() const { return std_type(vec_); }
	};

	// Generates permutation value.
	// O(n).
	value gen() const {
		if (!cycle_sizes_) {
			// Cycle sizes not specified.
			std::vector<int> idx_to_val(size_, -1), val_to_idx(size_, -1);
			for (auto [idx, val] : defs_) {
				tgen_ensure(
					0 <= val and val < size_,
					"permutation: value in permutation must be in [0, " +
						std::to_string(size_) + ")");

				if (idx_to_val[idx] != -1) {
					tgen_ensure(idx_to_val[idx] == val,
								"permutation: cannot set an index to two "
								"different values");
				} else
					idx_to_val[idx] = val;

				if (val_to_idx[val] != -1) {
					tgen_ensure(val_to_idx[val] == idx,
								"permutation: cannot set two indices to the "
								"same value");
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

		// Creates cycles.
		std::vector<int> order(size_);
		std::iota(order.begin(), order.end(), 0);
		shuffle(order.begin(), order.end());
		int idx = 0;
		std::vector<std::vector<int>> cycles;
		for (int cycle_size : *cycle_sizes_) {
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

		return value(perm);
	}
};

/************
 *          *
 *   MATH   *
 *          *
 ************/

namespace math {

namespace detail {

using namespace tgen::detail;

inline int popcount(uint64_t x) { return __builtin_popcountll(x); }

inline int ctzll(uint64_t x) {
	// Mystery code found on the internet.
	// Uses de Bruijn sequence.
	static const unsigned char index64[64] = {
		0,	1,	2,	53, 3,	7,	54, 27, 4,	38, 41, 8,	34, 55, 48, 28,
		62, 5,	39, 46, 44, 42, 22, 9,	24, 35, 59, 56, 49, 18, 29, 11,
		63, 52, 6,	26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
		51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12};
	return index64[((x & -x) * 0x022FDD63CC95386D) >> 58];
}

inline uint64_t mul_mod(uint64_t a, uint64_t b, uint64_t m) {
	return static_cast<u128>(a) * b % m;
}

// O(log n).
// 0 <= x < m.
inline uint64_t expo_mod(uint64_t x, uint64_t y, uint64_t m) {
	if (!y)
		return 1;
	uint64_t ans = expo_mod(mul_mod(x, x, m), y / 2, m);
	return y % 2 ? mul_mod(x, ans, m) : ans;
}

} // namespace detail

// O(log^2 n).
inline bool is_prime(uint64_t n) {
	if (n < 2)
		return false;
	if (n == 2 or n == 3)
		return true;
	if (n % 2 == 0)
		return false;

	uint64_t r = detail::ctzll(n - 1), d = n >> r;
	// These bases are guaranteed to work for n <= 2^64.
	for (int a : {2, 325, 9375, 28178, 450775, 9780504, 1795265022}) {
		uint64_t x = detail::expo_mod(a, d, n);
		if (x == 1 or x == n - 1 or a % n == 0)
			continue;

		for (uint64_t j = 0; j < r - 1; ++j) {
			x = detail::mul_mod(x, x, n);
			if (x == n - 1)
				break;
		}
		if (x != n - 1)
			return false;
	}
	return true;
}

namespace detail {

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
		x = f(x), y = f(f(y)), ++t;
	}
	return std::gcd(prd, n);
}

inline std::vector<uint64_t> factor(uint64_t n) {
	if (n == 1)
		return {};
	if (is_prime(n))
		return {n};
	uint64_t d = pollard_rho(n);
	std::vector<uint64_t> l = factor(d), r = factor(n / d);
	l.insert(l.end(), r.begin(), r.end());
	return l;
}

// Error handling.
template <typename T>
std::runtime_error there_is_no_in_range_error(const std::string &type, T l,
											  T r) {
	return error("math: there is no " + type + " in range [" +
				 std::to_string(l) + ", " + std::to_string(r) + "]");
}
template <typename T>
std::runtime_error there_is_no_from_error(const std::string &type, T r) {
	return error("math: there is no " + type + " from " + std::to_string(r));
}
template <typename T>
std::runtime_error there_is_no_upto_error(const std::string &type, T r) {
	return error("math: there is no " + type + " up to " + std::to_string(r));
}

// O(log mod).
// 0 < a < mod.
// gcd(a, mod) = 1.
inline i128 modular_inverse_128(i128 a, i128 mod) {
	tgen_ensure(0 < a and a < mod,
				"math: modular inverse requires 0 < value < mod");

	i128 t = 0, new_t = 1;
	i128 r = mod, new_r = a;

	while (new_r != 0) {
		i128 q = r / new_r;

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
	if (a == 0 or b == 0)
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
		// Necessary for correctness.
		if (!exp)
			break;

		if (!mul_leq(base, base, limit))
			return std::nullopt;
		base *= base;
	}
	return result;
}

// O(log n log k).
// 0 < k.
inline uint64_t kth_root_floor(uint64_t n, uint64_t k) {
	tgen_ensure_against_bug(k > 0, "math: value must be valid");
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
inline i128 gcd128(i128 a, i128 b) {
	if (a < 0)
		a = -a;
	if (b < 0)
		b = -b;
	while (b != 0) {
		i128 t = a % b;
		a = b;
		b = t;
	}
	return a;
}

// min(2^64, a*b).
// O(log a).
// a, b >= 0.
inline i128 mul_saturate(i128 a, i128 b) {
	tgen_ensure(a >= 0 and b >= 0);
	static const i128 LIMIT = static_cast<i128>(1) << 64;
	if (a == 0 or b == 0)
		return 0;
	if (a > LIMIT / b)
		return LIMIT;
	return a * b;
}

struct crt {
	using T = i128;
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

		k = static_cast<u128>(k) * inv % m2;

		T lcm = mul_saturate(m, m2);

		T res = (a + static_cast<T>((static_cast<u128>(k) * m) % lcm)) % lcm;
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

} // namespace detail

// Sorted.
// O(n^(1/4) log n) expected.
// 0 < n.
inline std::vector<uint64_t> factor(uint64_t n) {
	tgen_ensure(n > 0, "math: number to factor must be positive");
	auto factors = detail::factor(n);
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
	return detail::modular_inverse_128(a, mod);
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
// O(log(right)) approximately.
inline std::pair<uint64_t, uint64_t> prime_gap_upto(uint64_t right) {
	if (right < 4)
		throw detail::there_is_no_upto_error("prime gap", right);

	const auto &[P, G] = prime_gaps();
	for (int i = P.size() - 1;; --i) {
		if (P[i] >= right)
			continue;

		uint64_t real_right = std::min(right, P[i] + G[i] - 1);
		uint64_t prev = i > 0 ? G[i - 1] : 0;
		uint64_t curr = real_right - P[i];

		if (curr >= prev)
			return {P[i] + 1, real_right};
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

// O(log(right)) approximately.
inline uint64_t highly_composite_upto(uint64_t right) {
	for (int i = highly_composites().size() - 1; i >= 0; --i)
		if (highly_composites()[i] <= right)
			return highly_composites()[i];

	throw detail::there_is_no_upto_error("highly composite number", right);
}

// O(log^3 (right)) expected.
// Generates a random prime in [left, right].
inline uint64_t gen_prime(uint64_t left, uint64_t right) {
	if (right < left or right < 2)
		throw detail::there_is_no_in_range_error("prime", left, right);
	left = std::max<uint64_t>(left, 2);
	auto [l_gap, r_gap] = prime_gap_upto(right);
	if (right - left + 1 <= r_gap - l_gap + 1) {
		// There might be no primes in the range.
		std::vector<uint64_t> vals(right - left + 1);
		iota(vals.begin(), vals.end(), left);
		shuffle(vals.begin(), vals.end());
		for (uint64_t i : vals)
			if (is_prime(i))
				return i;
		throw detail::there_is_no_in_range_error("prime", left, right);
	}

	uint64_t n;
	do {
		n = next(left, right);
	} while (!is_prime(n));
	return n;
}

// O(log^3 (left)) expected.
// left <= 2^64 - 59.
inline uint64_t prime_from(uint64_t left) {
	tgen_ensure(left <= std::numeric_limits<uint64_t>::max() - 58,
				"math: invalid bound");
	for (uint64_t i = std::max<uint64_t>(2, left);; ++i)
		if (is_prime(i))
			return i;
}

// O(log^3 (right)) expected.
inline uint64_t prime_upto(uint64_t right) {
	if (right >= 2)
		for (uint64_t i = right; i >= 2; --i)
			if (is_prime(i))
				return i;
	throw detail::there_is_no_upto_error("prime", right);
}

// O(n^(1/4) log n) expected.
// 0 < n.
inline int num_divisors(uint64_t n) {
	int divisors = 1;
	for (auto [p, e] : factor_by_prime(n))
		divisors *= (e + 1);
	return divisors;
}

// Random number in [left, right] with `divisor_count` divisors.
// O(log(right) log(divisor_count)).
// divisor_count must be prime.
inline uint64_t gen_divisor_count(uint64_t left, uint64_t right,
								  int divisor_count) {
	tgen_ensure(divisor_count > 0 and is_prime(divisor_count),
				"math: divisor count must be prime");
	int root = divisor_count - 1;
	uint64_t p = gen_prime(detail::kth_root_floor(left, root),
						   detail::kth_root_floor(right, root));
	return *detail::expo(p, root, right);
}

// O(|mods| + log (right)).
// |rems| = |mods|.
// rems_i < mods_i.
inline uint64_t gen_congruent(uint64_t left, uint64_t right,
							  std::vector<uint64_t> rems,
							  std::vector<uint64_t> mods) {
	if (left > right)
		throw detail::there_is_no_in_range_error("congruent number", left,
												 right);
	tgen_ensure(rems.size() == mods.size(),
				"math: number of remainders and mods must be the same");
	tgen_ensure(rems.size() > 0, "math: must have at least one congruence");

	detail::crt crt;
	for (int i = 0; i < static_cast<int>(rems.size()); ++i) {
		tgen_ensure(rems[i] < mods[i],
					"math: remainder must be smaller than the mod");
		crt = crt * detail::crt(rems[i], mods[i]);

		if (crt.a == -1)
			throw detail::there_is_no_in_range_error("congruent number", left,
													 right);
		if (crt.m > right) {
			if (!(left <= crt.a and crt.a <= right))
				throw detail::there_is_no_in_range_error("congruent number",
														 left, right);

			for (int j = 0; j < static_cast<int>(rems.size()); ++j)
				if (crt.a % mods[j] != rems[j])
					throw detail::there_is_no_in_range_error("congruent number",
															 left, right);
			return crt.a;
		}
	}

	uint64_t k_min = crt.a >= left ? 0 : ((left - crt.a) + crt.m - 1) / crt.m;
	uint64_t k_max = (right - crt.a) / crt.m;

	if (k_min > k_max)
		throw detail::there_is_no_in_range_error("congruent number", left,
												 right);

	return crt.a + next(k_min, k_max) * crt.m;
}

// O(log (right)).
// rem < mod.
inline uint64_t gen_congruent(uint64_t left, uint64_t right, uint64_t rem,
							  uint64_t mod) {
	return gen_congruent(left, right, std::vector<uint64_t>({rem}),
						 std::vector<uint64_t>({mod}));
}

// First congruent number >= left.
// O(|mods| + log (left)).
// |rems| = |mods|.
// rems_i < mods_i.
inline uint64_t congruent_from(uint64_t left, std::vector<uint64_t> rems,
							   std::vector<uint64_t> mods) {
	tgen_ensure(rems.size() == mods.size(),
				"math: number of remainders and mods must be the same");
	tgen_ensure(rems.size() > 0, "math: must have at least one congruence");

	detail::crt crt;
	for (int i = 0; i < static_cast<int>(rems.size()); ++i) {
		tgen_ensure(rems[i] < mods[i],
					"math: remainder must be smaller than the mod");
		crt = crt * detail::crt(rems[i], mods[i]);

		if (crt.a == -1)
			throw detail::there_is_no_from_error("congruent number", left);
		if (crt.m > std::numeric_limits<uint64_t>::max()) {
			if (crt.a < left)
				throw detail::error(
					"math: congruent number does not exist or is too large");

			for (int j = 0; j < static_cast<int>(rems.size()); ++j)
				if (crt.a % mods[j] != rems[j])
					throw detail::error("math: congruent number does "
										"not exist or is too large");
			return crt.a;
		}
	}

	uint64_t k = 0;
	if (crt.a < left)
		k = ((left - crt.a) + crt.m - 1) / crt.m;
	detail::i128 result = crt.a + k * crt.m;

	if (result > std::numeric_limits<uint64_t>::max())
		throw detail::error("math: congruent number is too large");
	return result;
}

// O(log (left))
// rem < mod.
inline uint64_t congruent_from(uint64_t left, uint64_t rem, uint64_t mod) {
	return congruent_from(left, std::vector<uint64_t>{rem},
						  std::vector<uint64_t>{mod});
}

// Last congruent number <= right.
// O(|mods| + log (right)).
// |rems| = |mods|.
// rems_i < mods_i.
inline uint64_t congruent_upto(uint64_t right, std::vector<uint64_t> rems,
							   std::vector<uint64_t> mods) {
	tgen_ensure(rems.size() == mods.size(),
				"math: number of remainders and mods must be the same");
	tgen_ensure(rems.size() > 0, "math: must have at least one congruence");

	detail::crt crt;
	for (int i = 0; i < static_cast<int>(rems.size()); ++i) {
		tgen_ensure(rems[i] < mods[i],
					"math: remainder must be smaller than the mod");

		crt = crt * detail::crt(rems[i], mods[i]);

		if (crt.a == -1)
			throw detail::there_is_no_upto_error("congruent number", right);
		if (crt.m > right) {
			if (!(crt.a <= right))
				throw detail::there_is_no_upto_error("congruent number", right);

			for (int j = 0; j < static_cast<int>(rems.size()); ++j)
				if (crt.a % mods[j] != rems[j])
					throw detail::there_is_no_upto_error("congruent number",
														 right);
			return crt.a;
		}
	}

	if (crt.a > right)
		throw detail::there_is_no_upto_error("congruent number", right);

	uint64_t k = (right - crt.a) / crt.m;
	detail::i128 result = crt.a + k * crt.m;

	if (result < 0)
		throw detail::there_is_no_upto_error("congruent number", right);
	return result;
}

// O(log r)
// rem < mod.
inline uint64_t congruent_upto(uint64_t right, uint64_t rem, uint64_t mod) {
	return congruent_upto(right, std::vector<uint64_t>{rem},
						  std::vector<uint64_t>{mod});
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

// Partition is ordered (composition), that is, (1, 1, 2) != (1, 2, 1).
// O(n).
// 0 < n.
// 0 < part_left.
inline std::vector<int>
gen_partition(int n, int part_left = 1,
			  std::optional<int> part_right = std::nullopt) {
	if (!part_right.has_value())
		part_right = n;
	part_right = std::min(*part_right, n);
	tgen_ensure(n > 0 and part_left > 0,
				"math: invalid parameters to gen_partition");
	tgen_ensure(part_left <= n and *part_right > 0, "math: no such partition");

	// dp[i] = log(number of ways to add to i).
	std::vector<long double> dp(n + 1, detail::LOG_ZERO);
	dp[0] = detail::LOG_ONE;
	long double window = detail::LOG_ZERO;
	for (int i = 1; i <= n; ++i) {
		if (i >= part_left)
			window = detail::add_log_space(window, dp[i - part_left]);
		if (i >= *part_right + 1)
			window = detail::sub_log_space(window, dp[i - *part_right - 1]);
		dp[i] = window;
	}
	tgen_ensure(dp[n] >= 0, "math: no such partition");

	// Crazy math tricks ahead.
	auto dp_pref = dp;
	for (int i = 1; i <= n; ++i)
		dp_pref[i] = detail::add_log_space(dp_pref[i - 1], dp[i]);

	std::vector<int> part;
	int sum = n;
	while (sum > 0) {
		// Will generate a number such that what remains is in [l, r].
		int l = std::max(0, sum - *part_right), r = sum - part_left;
		detail::tgen_ensure_against_bug(r >= 0, "math: r < 0 in gen_partition");

		int nxt_sum = std::min(sum, r);
		long double random = next<long double>(0, 1);

		// We generate a value X (log space), and then choose nxt_sum such
		// that dp_pref[nxt_sum-1] < X <= dp_pref[nxt_sum].

		// Math hack:
		// Let A = pref[l-1], B = pref[r], U = rand().
		// X = log[exp(A) + U * (exp(B) - exp(A))]
		//   = log{exp(B) * [exp(A) / exp(B) + U * (1 - exp(A) / exp(B))]}
		//   = B + log[exp(A - B) + U - U * exp(A - B))]
		//   = B + log[U + (1 - U) * exp(A - B)].
		long double val_l = l ? dp_pref[l - 1] : detail::LOG_ZERO,
					val_r = dp_pref[r];
		while (nxt_sum > l and
			   dp_pref[nxt_sum - 1] >=
				   val_r + detail::log_space(random +
											 (1 - random) * exp(val_l - val_r)))
			--nxt_sum;

		part.push_back(sum - nxt_sum);
		sum = nxt_sum;
	}

	return part;
}

// Partition is ordered (composition), that is, (1, 1, 2) != (1, 2, 1).
// O(n) time/memory if part_right is not set, O(n * k) time/memory otherwise.
// 0 < k <= n.
// 0 <= part_left.
inline std::vector<int>
gen_partition_fixed_size(int n, int k, int part_left = 0,
						 std::optional<int> part_right = std::nullopt) {
	if (!part_right.has_value())
		part_right = n;
	part_right = std::min(*part_right, n);
	tgen_ensure(0 < k and k <= n and part_left >= 0,
				"math: invalid parameters to gen_partition_fixed_size");
	tgen_ensure(static_cast<long long>(k) * part_left <= n and
					n <= static_cast<long long>(k) * (*part_right),
				"math: no such partition");

	// What we need to distribute to the parts.
	int s = n - k * part_left;

	std::vector<int> part(k);
	if (*part_right == n) {
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
		int u = *part_right - part_left;

		// dp[i][j] = log(#ways to fill i parts with sum j)
		std::vector<std::vector<long double>> dp(
			k + 1, std::vector<long double>(s + 1, detail::LOG_ZERO));
		dp[0][0] = detail::LOG_ONE;

		for (int i = 1; i <= k; ++i) {
			std::vector<long double> pref = dp[i - 1];
			for (int j = 1; j <= s; ++j)
				pref[j] = detail::add_log_space(pref[j - 1], dp[i - 1][j]);

			for (int j = 0; j <= s; ++j) {
				dp[i][j] = pref[j];
				if (j >= u + 1)
					dp[i][j] = detail::sub_log_space(dp[i][j], pref[j - u - 1]);
			}
		}

		// Recovers parts backwards.
		int left_to_distribute = s;
		for (int i = k; i >= 1; --i) {
			long double log_total = detail::LOG_ZERO;
			for (int j = 0; j <= u and j <= left_to_distribute; ++j)
				log_total = detail::add_log_space(
					log_total, dp[i - 1][left_to_distribute - j]);
			detail::tgen_ensure_against_bug(
				log_total != detail::LOG_ZERO,
				"math: total == 0 in gen_partition_fixed_size");

			// Now we choose a number with probability proportional to
			// dp[i-1][.].

			// log(rand() * total) = log(rand()) + log(total).
			long double random =
				detail::log_space(next<long double>(0, 1)) + log_total;

			long double cur_prob = detail::LOG_ZERO;
			int chosen = 0;
			for (int j = 0; j <= u and j <= left_to_distribute; ++j) {
				cur_prob = detail::add_log_space(
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
		i += part_left;
	return part;
}

// Partition is ordered (composition), that is, (1, 1, 2) != (1, 2, 1).
// Inspired by jngen rndm.partition: random delimiters, sort, gap recovery;
// omits jngen's part reordering, shuffles, and two-pass redistribution.
// 0 < k <= n.
// 0 <= part_left.
// Not uniformly random; optimized for speed.
// O(k log k).
inline std::vector<uint64_t> gen_partition_fixed_size_fast(
	uint64_t n, int k, uint64_t part_left = 0,
	std::optional<uint64_t> part_right = std::nullopt) {
	if (!part_right.has_value())
		part_right = n;
	part_right = std::min(*part_right, n);

	detail::u128 n128 = n;
	detail::u128 k128 = k;
	detail::u128 part_left128 = part_left;
	detail::u128 part_right128 = *part_right;

	tgen_ensure(k > 0 and k128 <= n128,
				"math: invalid parameters to gen_partition_fixed_size_fast");
	tgen_ensure(part_right128 >= part_left128 and
					k128 * part_left128 <= n128 and
					k128 * part_right128 >= n128,
				"math: no such partition");

	uint64_t slack_total = n128 - k128 * part_left128;
	uint64_t slack_max = part_right128 - part_left128;

	std::vector<uint64_t> part(k);
	if (k == 1) {
		part[0] = slack_total;
	} else {
		std::vector<uint64_t> cuts(k - 1);
		for (uint64_t &d : cuts)
			d = next<uint64_t>(0, slack_total);
		std::sort(cuts.begin(), cuts.end());

		uint64_t prev = 0;
		for (int i = 0; i + 1 < k; ++i) {
			part[i] = cuts[i] - prev;
			prev = cuts[i];
		}
		part[k - 1] = slack_total - prev;
	}

	auto add_part_left = [part_left](uint64_t x) -> uint64_t {
		detail::u128 val = x + part_left;
		detail::tgen_ensure_against_bug(
			val <= std::numeric_limits<uint64_t>::max(),
			"math: part + part_left exceeds uint64_t in "
			"gen_partition_fixed_size_fast");
		return val;
	};

	if (slack_max >= slack_total) {
		for (uint64_t &x : part)
			x = add_part_left(x);
		return part;
	}

	detail::u128 remaining = 0;
	for (uint64_t &x : part) {
		if (x > slack_max) {
			remaining += x - slack_max;
			x = slack_max;
		}
		x = add_part_left(x);
	}

	if (remaining > 0) {
		for (uint64_t &x : part) {
			if (x < *part_right && remaining > 0) {
				detail::u128 room = *part_right - x;
				detail::u128 add = std::min(remaining, room);
				detail::u128 val = x + add;
				detail::tgen_ensure_against_bug(
					val <= *part_right,
					"math: part exceeds part_right after redistribution in "
					"gen_partition_fixed_size_fast");
				x = val;
				remaining -= add;
			}
		}
		detail::tgen_ensure_against_bug(
			remaining == 0, "math: remaining mass after redistribution in "
							"gen_partition_fixed_size_fast");
	}

	return part;
}

// Random partition of elements into k ordered groups (input order preserved).
// If max_size is unset, part sizes are uniform via gen_partition_fixed_size.
// If max_size is set, uses gen_partition_fixed_size_fast (not uniform).
// O(n) if max_size is unset; O(n + k log k) if max_size is set.
template <typename T>
std::vector<std::vector<T>>
partition_elements(std::vector<T> elements, int k, int min_size = 0,
				   std::optional<uint64_t> max_size = std::nullopt) {
	size_t n = elements.size();
	tgen_ensure(k > 0, "math: partition_elements: k must be positive");
	tgen_ensure(min_size >= 0,
				"math: partition_elements: min_size must be non-negative");

	std::vector<uint64_t> sizes;
	if (max_size.has_value()) {
		sizes = gen_partition_fixed_size_fast(n, k, min_size, max_size);
	} else {
		for (int sz : gen_partition_fixed_size(n, k, min_size))
			sizes.push_back(sz);
	}

	std::vector<std::vector<T>> groups;
	groups.reserve(k);
	size_t pos = 0;
	for (uint64_t sz : sizes) {
		groups.emplace_back(elements.begin() + pos,
							elements.begin() + pos + sz);
		pos += sz;
	}
	return groups;
}

}; // namespace math

/**************
 *            *
 *   STRING   *
 *            *
 **************/

namespace detail {

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
 * applied to "[01]abc").
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
			log_space_num_ways_ = math::detail::LOG_ONE;
			return;
		}
		tgen_ensure_against_bug(pattern[0] == '[' and pattern.back() == ']',
								"str: invalid regex: expected character class");
		int size = pattern.size() - 2;
		log_space_num_ways_ = math::detail::log_space(size);
		distinct_ = distinct_container<char>(pattern.substr(1, size));
	}
	// SEQ or OR.
	regex_node(const std::string &pattern, std::vector<regex_node> &children)
		: pattern_(pattern), left_bound_(-1), right_bound_(-1) {
		if (pattern == "SEQ") {
			// Multiply the number of ways.
			log_space_num_ways_ = math::detail::LOG_ONE;
			for (const auto &child : children)
				log_space_num_ways_ += child.log_space_num_ways_;
		} else if (pattern == "OR") {
			// Add the number of ways.
			log_space_num_ways_ = math::detail::LOG_ZERO;
			for (const auto &child : children)
				log_space_num_ways_ = math::detail::add_log_space(
					log_space_num_ways_, child.log_space_num_ways_);
		} else
			tgen_ensure_against_bug("str: invalid regex: expected SEQ or OR");

		children_ = std::move(children);
		children.clear();
	}
	// REP.
	regex_node(int left_bound, int right_bound, regex_node &child)
		: pattern_("REP"), left_bound_(left_bound), right_bound_(right_bound) {
		log_space_num_ways_ = math::detail::LOG_ZERO;
		for (int i = left_bound; i <= right_bound; ++i)
			log_space_num_ways_ = math::detail::add_log_space(
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

	for (size_t i = 0; i < regex.size(); ++i) {
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
					for (char x = a; x <= b; ++x)
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
			tgen_ensure(l <= r,
						"str: invalid regex: invalid range inside `{}`");

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
		double log_rand = math::detail::log_space(next<double>(0, 1)) +
						  node.log_space_num_ways_;
		double cur_prob = math::detail::LOG_ZERO;
		double child_num_ways = node.children_[0].log_space_num_ways_;

		for (int i = node.left_bound_; i <= node.right_bound_; ++i) {
			cur_prob =
				math::detail::add_log_space(cur_prob, i * child_num_ways);
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
		double log_rand = math::detail::log_space(next<double>(0, 1)) +
						  node.log_space_num_ways_;
		double cur_prob = math::detail::LOG_ZERO;

		for (const regex_node &child : node.children_) {
			cur_prob = math::detail::add_log_space(cur_prob,
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
	detail::tgen_ensure_against_bug(
		node.pattern_.size() == 1,
		"str: invalid regex: expected single character, but got `" +
			node.pattern_ + "`");
	str += node.pattern_[0];
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

} // namespace detail

/*
 * String generator.
 */

struct str : gen_base<str> {
	std::optional<list<char>> list_; // List of characters.
	std::optional<detail::regex_node>
		root_; // Root node of the regex tree for the whole string.

	// Creates generator for strings of size 'size', with random characters in
	// [value_left, value_right].
	str(int size, char value_left = 'a', char value_right = 'z') {
		tgen_ensure(size > 0, "str: size must be positive");
		list_ = list<char>(size, value_left, value_right);
	}

	// Creates generator for strings of size 'size', with random characters in
	// 'chars'.
	str(int size, std::set<char> chars) {
		tgen_ensure(size > 0, "str: size must be positive");
		list_ = list<char>(size, chars);
	}

	// Creates generator for strings that match the given regex.
	template <typename... Args> str(const std::string &regex, Args &&...args) {
		tgen_ensure(regex.size() > 0, "str: regex must be non-empty");

		root_ = detail::parse_regex(
			detail::regex_format(regex, std::forward<Args>(args)...));
	}

	// Restricts strings for str[idx] = value.
	str &fix(int idx, char character) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		list_->fix(idx, character);
		return *this;
	}

	// Restricts strings for list[S] to be equal, for given subset S of indices.
	str &equal(std::set<int> indices) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		list_->equal(indices);
		return *this;
	}

	// Restricts strings for str[idx_1] = str[idx_2].
	str &equal(int idx_1, int idx_2) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		list_->equal(idx_1, idx_2);
		return *this;
	}

	// Restricts strings for str[left..right] to have all equal values.
	str &equal_range(int left, int right) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		list_->equal_range(left, right);
		return *this;
	}

	// Restricts strings for all equal chars.
	str &all_equal() {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		list_->all_equal();
		return *this;
	}

	// Restricts strings for str[left..right] to be a palindrome.
	str &palindrome(int left, int right) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		tgen_ensure(0 <= left and left <= right and right < list_->size_,
					"str: range indices must be valid");
		for (int i = left; i < right - (i - left); ++i)
			equal(i, right - (i - left));
		return *this;
	}

	// Restricts strings for the entire string to be a palindrome.
	str &palindrome() {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		return palindrome(0, list_->size_ - 1);
	}

	// Restricts strings for str[S] to be different (distinct), for given subset
	// S of indices.
	str &different(std::set<int> indices) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		list_->different(indices);
		return *this;
	}

	// Restricts strings for str[idx_1] != str[idx_2].
	str &different(int idx_1, int idx_2) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		list_->different(idx_1, idx_2);
		return *this;
	}

	// Restricts lists for list[left..right] to have all different chars.
	str &different_range(int left, int right) {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		list_->different_range(left, right);
		return *this;
	}

	// Restricts strings for all chars to be different.
	str &all_different() {
		tgen_ensure(!root_, "str: cannot add restriction for regex");
		list_->all_different();
		return *this;
	}

	// str value.
	struct value : gen_value_base<value> {
		using tgen_is_sequential_tag = detail::is_sequential_tag;

		using value_type = char;
		using std_type = std::string;
		std::string str_;

		value(const std::string &str) : str_(str) {
			tgen_ensure(!str_.empty(), "str: value: cannot be empty");
		}

		// Fetches size.
		int size() const { return str_.size(); }

		// Fetches position idx.
		char &operator[](int idx) {
			tgen_ensure(0 <= idx and idx < size(),
						"str: value: index out of bounds");
			return str_[idx];
		}
		const char &operator[](int idx) const {
			tgen_ensure(0 <= idx and idx < size(),
						"str: value: index out of bounds");
			return str_[idx];
		}

		// Sorts characters in non-decreasing order.
		// O(n log n).
		value &sort() {
			std::sort(str_.begin(), str_.end());
			return *this;
		}

		// Reverses string.
		// O(n).
		value &reverse() {
			std::reverse(str_.begin(), str_.end());
			return *this;
		}

		// Lowercases all characters.
		// O(n).
		value &lowercase() {
			for (char &c : str_)
				c = std::tolower(c);
			return *this;
		}

		// Uppercases all characters.
		// O(n).
		value &uppercase() {
			for (char &c : str_)
				c = std::toupper(c);
			return *this;
		}

		// Concatenates two values.
		// Linear.
		value operator+(const value &rhs) const {
			return value(str_ + rhs.str_);
		}

		// Shuffles string uniformly.
		// O(n).
		value &shuffle() {
			for (int i = 0; i < size(); ++i)
				std::swap(str_[i], str_[next(0, size() - 1)]);
			return *this;
		}

		// Returns a random character uniformly.
		// O(1).
		char pick() const { return str_[next<int>(0, size() - 1)]; }

		// Returns str_[i] with probability proportional to distribution[i].
		// O(1).
		template <typename Dist>
		char pick_by_distribution(const std::vector<Dist> &distribution) const {
			tgen_ensure(static_cast<size_t>(size()) == distribution.size(),
						"value and distribution must have the same size");
			return str_[next_by_distribution(distribution)];
		}
		template <typename Dist>
		char pick_by_distribution(
			const std::initializer_list<Dist> &distribution) const {
			return pick_by_distribution(std::vector<Dist>(distribution));
		}

		// Chooses k characters uniformly, as in a subsequence of size k.
		// O(n).
		value choose(int k) const {
			tgen_ensure(0 < k and k <= size(),
						"number of elements to choose must be valid");
			std::string new_str;
			int need = k;
			for (int i = 0; need > 0; ++i) {
				int left = size() - i;
				if (next(1, left) <= need) {
					new_str.push_back(str_[i]);
					need--;
				}
			}
			return value(new_str);
		}

		// Prints to std::ostream.
		friend std::ostream &operator<<(std::ostream &out, const value &val) {
			return out << val.str_;
		}

		// Gets a std::string representing the value.
		std::string to_std() const { return std_type(str_); }
	};

	// Generates str value.
	// If created from restrictions: O(n log n).
	// If created from regex: expected linear.
	value gen() const {
		if (root_) {
			// Regex.
			std::string ret_str;
			gen_regex(*root_, ret_str);
			return value(ret_str);
		} else {
			// List.
			std::vector<char> vec = list_->gen().to_std();
			return value(std::string(vec.begin(), vec.end()));
		}
	}
};

/************
 *          *
 *   PAIR   *
 *          *
 ************/

namespace detail {

// Generates pair first == second.
// O(1).
template <typename T> std::pair<T, T> gen_eq(T L1, T R1, T L2, T R2) {
	T L = std::max(L1, L2);
	T R = std::min(R1, R2);

	tgen_ensure(L <= R, "pair: no valid values to generate");
	T x = next<T>(L, R);
	return {x, x};
}

// Returns {R1-L1+1, R2-L2+1}.
template <typename T>
std::pair<u128, u128> get_n_and_m(T L1, T R1, T L2, T R2) {
	u128 n = static_cast<i128>(R1) - L1 + 1;
	u128 m = static_cast<i128>(R2) - L2 + 1;
	return {n, m};
}

// Returns first + first+1 + ... + last,
// num_terms terms. Avoids overflow.
static u128 pos_arith_sum(u128 first, u128 last, u128 num_terms) {
	u128 x = first + last, y = num_terms;

	// x * y / 2, avoiding overflow.
	if (x % 2 == 0)
		x /= 2;
	else
		y /= 2;

	return x * y;
}

// Generates pair first != second.
// O(1) expected.
template <typename T> std::pair<T, T> gen_neq(T L1, T R1, T L2, T R2) {
	auto [n, m] = get_n_and_m(L1, R1, L2, R2);

	T L_intersect = std::max(L1, L2);
	T R_intersect = std::min(R1, R2);
	u128 inter = static_cast<i128>(R_intersect) - L_intersect + 1;

	u128 total = n * m - inter;
	tgen_ensure(total > 0, "pair: no valid values to generate");

	// Runs O(1) expected times in the worst case.
	T a, b;
	do {
		a = next<T>(L1, R1);
		b = next<T>(L2, R2);
	} while (a == b);

	return {a, b};
}

// For lt, splits 'second' into two regions:
// 1) second <= R1 -> number of 'first' is (second - L1)
// 2) second >  R1 -> number of 'first' is (R1 - L1 + 1)
// Returns {count_region1, count_region2}.
// O(1).
template <typename T>
std::pair<u128, u128> count_lt_regions(T L1, T R1, T L2, T R2) {
	auto [n, m] = get_n_and_m(L1, R1, L2, R2);

	// 'second' must be >= L1 + 1.
	i128 L_second = std::max<i128>(L2, static_cast<i128>(L1) + 1);
	i128 R_second = R2;

	// Split point for 'second'.
	i128 split = std::min<i128>(R_second, R1);

	// Region 1: b in [L_second, split].
	u128 len1 = std::max<i128>(0, split - L_second + 1);

	u128 count_region1 = 0;
	if (len1 > 0) {
		// For b in [L_second, split], there are (b - L1) ways.
		i128 first = L_second - L1;
		i128 last = split - L1;

		// Arithmetic series first + (first + 1) + ... + last, len1 terms.
		count_region1 = pos_arith_sum(first, last, len1);
	}

	// Region 2: b > R1.
	// For b in [R1+1, R_second], there are 'n' ways.
	i128 L_second_region2 = std::max(L_second, static_cast<i128>(R1) + 1);

	u128 len2 = std::max<i128>(0, R_second - L_second_region2 + 1);
	u128 count_region2 = len2 * n;

	return {count_region1, count_region2};
}

// Generates pair first < second.
// O(log(R1 - L1 + 1) + log(R2 - L2 + 1)).
template <typename T> std::pair<T, T> gen_lt(T L1, T R1, T L2, T R2) {
	auto [n, m] = get_n_and_m(L1, R1, L2, R2);

	// 'second' needs to be at least L1 + 1 to have a valid value for
	// 'first'.
	i128 L_second = std::max<i128>(L2, static_cast<i128>(L1) + 1);
	i128 R_second = R2;

	// Splits 'second' into two regions:
	// 1) b <= R1 -> number of 'first' is (b - L1);
	// 2) b >  R1 -> number of 'first' is (R1 - L1 + 1).
	i128 split = std::min<i128>(R_second, R1);

	auto [count_region1, count_region2] = count_lt_regions(L1, R1, L2, R2);
	u128 total = count_region1 + count_region2;
	tgen_ensure(total > 0, "pair: no valid values to generate");

	u128 k = detail::next128(total);
	if (k < count_region1) {
		// Region 1: invert arithmetic series.

		// For b in [L_second, split].
		u128 len1 = std::max<i128>(0, split - L_second + 1);

		// We consider b in [L_second, L_second + d].
		// Each b contributes (b - L1) = base + (b - L_second).
		// So we sum: base + (base+1) + ... + (base+d)
		// d in [0, len1).

		i128 base = L_second - L1;
		i128 lo = 0, hi = static_cast<i128>(len1) - 1;

		while (lo < hi) {
			i128 mid = lo + (hi - lo) / 2;

			if (pos_arith_sum(base, base + mid, mid + 1) <= k)
				lo = mid + 1;
			else
				hi = mid;
		}
		i128 d = lo;

		// Subtracts prefix sum with d-1 terms from k.
		if (d > 0)
			k -= pos_arith_sum(base, base + d - 1, d);

		return {L1 + static_cast<T>(k), L_second + d};
	} else {
		// Region 2: uniform block of size n.
		k -= count_region1;

		// For b in [R1+1, R_second], there are 'n' ways.
		i128 L_second_region2 = std::max(L_second, static_cast<i128>(R1) + 1);

		return {L1 + static_cast<T>(k % n),
				L_second_region2 + static_cast<T>(k / n)};
	}
}

// Generates pair first > second.
// O(log(R1 - L1 + 1) + log(R2 - L2 + 1)).
template <typename T> std::pair<T, T> gen_gt(T L1, T R1, T L2, T R2) {
	auto [first, second] = gen_lt(L2, R2, L1, R1);
	return {second, first};
}

// Generates pair first <= second.
// O(log(R1 - L1 + 1) + log(R2 - L2 + 1)).
template <typename T> std::pair<T, T> gen_leq(T L1, T R1, T L2, T R2) {
	// Counts how many pairs are there with first = second.
	i128 L_intersect = std::max(L1, L2);
	i128 R_intersect = std::min(R1, R2);
	u128 eq_count = std::max<i128>(0, R_intersect - L_intersect + 1);

	// Counts how many pairs are there with first < second.
	auto [lt_region1, lt_region2] = count_lt_regions(L1, R1, L2, R2);
	u128 lt_count = lt_region1 + lt_region2;

	u128 total = eq_count + lt_count;
	tgen_ensure(total > 0, "pair: no valid values to generate");

	if (detail::next128(total) < eq_count)
		return gen_eq(L1, R1, L2, R2);
	return gen_lt(L1, R1, L2, R2);
}

// Generates pair first >= second.
// O(log(R1 - L1 + 1) + log(R2 - L2 + 1)).
template <typename T> std::pair<T, T> gen_geq(T L1, T R1, T L2, T R2) {
	auto [first, second] = gen_leq(L2, R2, L1, R1);
	return {second, first};
}

}; // namespace detail

/*
 * Pair generator.
 *
 * Pairs of integral types.
 */

template <typename T> struct pair : gen_base<pair<T>> {
	std::pair<T, T> first_, second_; // Range of first and second values.
	// Type of restriction.
	enum class restriction_type { eq, neq, lt, gt, leq, geq, unspecified };
	restriction_type type_ = restriction_type::unspecified;

	// Creates a pair with random values in [first_l, first_r] and [second_l,
	// second_r].
	pair(T first_left, T first_right, T second_left, T second_right)
		: first_(first_left, first_right), second_(second_left, second_right) {
		tgen_ensure(first_left <= first_right,
					"pair: first range must be valid");
		tgen_ensure(second_left <= second_right,
					"pair: second range must be valid");
	}

	// Creates a pair with random values in [both_l, both_r].
	pair(T both_left, T both_right)
		: pair(both_left, both_right, both_left, both_right) {}

	// Restricts pair for first = second.
	pair &eq() {
		type_ = restriction_type::eq;
		return *this;
	}

	// Restricts pair for first != second.
	pair &neq() {
		type_ = restriction_type::neq;
		return *this;
	}

	// Restricts pair for first < second.
	pair &lt() {
		type_ = restriction_type::lt;
		return *this;
	}

	// Restricts pair for first > second.
	pair &gt() {
		type_ = restriction_type::gt;
		return *this;
	}

	// Restricts pair for first <= second.
	pair &leq() {
		type_ = restriction_type::leq;
		return *this;
	}

	// Restricts pair for first >= second.
	pair &geq() {
		type_ = restriction_type::geq;
		return *this;
	}

	// Pair value.
	struct value : gen_value_base<value> {
		using value_type = T;
		using std_type = std::pair<T, T>;

		std::pair<T, T> pair_;
		char sep_;

		value(const std::pair<T, T> &pair) : pair_(pair), sep_(' ') {}
		value(const T &first, const T &second)
			: pair_(first, second), sep_(' ') {}

		T first() const { return pair_.first; }
		T second() const { return pair_.second; }

		// Sets the separator for the pair, for printing.
		value &separator(char sep) {
			sep_ = sep;
			return *this;
		}

		// Prints to std::ostream, separated by sep_.
		friend std::ostream &operator<<(std::ostream &out, const value &val) {
			return out << val.pair_.first << val.sep_ << val.pair_.second;
		}

		// Gets a std::pair representing the value.
		auto to_std() const {
			if constexpr (!detail::is_generator_value<T>::value) {
				return pair_;
			} else {
				std::pair<typename T::std_type, typename T::std_type> pair(
					pair_.first.to_std(), pair_.second.to_std());
				return pair;
			}
		}
	};

	// Generates a random pair.
	// O(log(R1 - L1 + 1) + log(R2 - L2 + 1)).
	value gen() const {
		T L1 = first_.first, R1 = first_.second;
		T L2 = second_.first, R2 = second_.second;

		switch (type_) {
		case restriction_type::unspecified:
			return {next<T>(L1, R1), next<T>(L2, R2)};
		case restriction_type::eq:
			return detail::gen_eq<T>(L1, R1, L2, R2);
		case restriction_type::neq:
			return detail::gen_neq<T>(L1, R1, L2, R2);
		case restriction_type::lt:
			return detail::gen_lt<T>(L1, R1, L2, R2);
		case restriction_type::gt:
			return detail::gen_gt<T>(L1, R1, L2, R2);
		case restriction_type::leq:
			return detail::gen_leq<T>(L1, R1, L2, R2);
		case restriction_type::geq:
			return detail::gen_geq<T>(L1, R1, L2, R2);
		}
		throw detail::error("pair: unknown restriction type");
	}
};

/************
 *          *
 *   TREE   *
 *          *
 ************/

namespace detail {

// Generates edges from Prufer sequence.
// O(n).
inline std::vector<std::pair<int, int>> edges_from_prufer(std::vector<int> p) {
	int n = p.size() + 2;

	// Degrees.
	std::vector<int> d(n, 1);
	for (int i : p)
		d[i]++;

	// Adds last vertex.
	p.push_back(n - 1);

	// Finds first vertex with degree 1.
	int idx, u;
	idx = u = find(d.begin(), d.end(), 1) - d.begin();

	// Generates edges.
	std::vector<std::pair<int, int>> edges;
	for (int v : p) {
		edges.emplace_back(u, v);
		if (--d[v] == 1 and v < idx)
			u = v;
		else
			idx = u = find(d.begin() + idx + 1, d.end(), 1) - d.begin();
	}
	return edges;
}

// Disjoint set union (union-find) for connectivity queries.
struct dsu {
	std::vector<int> parent_;
	std::vector<unsigned char> rank_;

	// Creates a dsu with `n` elements, indexed from 0 to n-1.
	// Initially every element is in its own set.
	// O(n).
	dsu(int n) : parent_(n), rank_(n, 0) {
		for (int i = 0; i < n; ++i)
			parent_[i] = i;
	}

	// Adds new elements to the dsu, each in their own new set.
	// O(k) amortized.
	void add_elements(int k) {
		for (int i = 0; i < k; ++i) {
			int new_id = parent_.size();
			parent_.push_back(new_id);
			rank_.push_back(0);
		}
	}

	// Finds representative of set containing i.
	// O(alpha(n)) amortized, O(log n) worst case.
	int find(int i) {
		return parent_[i] == i ? i : parent_[i] = find(parent_[i]);
	}

	// Merges components of `a` and `b`. Returns if the sets were united, and
	// false if a and b were in the same set.
	// O(alpha(n)) amortized, O(log n) worst case.
	bool unite(int a, int b) {
		a = find(a);
		b = find(b);
		if (a == b)
			return false;
		if (rank_[a] > rank_[b])
			std::swap(a, b);
		parent_[a] = b;
		if (rank_[a] == rank_[b])
			++rank_[b];
		return true;
	}
};

} // namespace detail

// Forward declaration of wgraph.
template <typename VWeight, typename EWeight> struct wgraph;

/*
 * Tree generator.
 *
 * Unrooted trees with `n` vertices, indexed from 0 to n-1.
 * These are unrooted undirected labeled trees, that is, isomorphism is not
 * taken into account. VWeight is the type of vertex weights, and EWeight is
 * the type of edge weights. Generator does not generate weights. The weights
 * are to be set in the wtree::value.
 */

template <typename VWeight, typename EWeight>
struct wtree : gen_base<wtree<VWeight, EWeight>> {
	int n_;								  // Number of vertices.
	std::set<std::pair<int, int>> edges_; // Edges that were set.

	// Creates tree generator with `n` vertices.
	// O(1).
	wtree(int n) : n_(n) {
		tgen_ensure(n > 0, "wtree: number of vertices must be positive");
	}

	// Adds edge between u and v (this edge must be generated).
	// O(log n).
	wtree &add_edge(int u, int v) {
		tgen_ensure(0 <= std::min(u, v) and std::max(u, v) < n_,
					"wtree: vertices must be indexed in [0, n)");
		tgen_ensure(u != v, "wtree: cannot add self loop to tree");

		if (u > v)
			std::swap(u, v);
		edges_.emplace(u, v);
		return *this;
	}

	// Tree value.
	//
	// Edges are stored in both directions in adjacency list, but only u < v in
	// edge list.
	struct value : gen_value_base<value> {
		using std_type = std::pair<int, std::vector<std::set<int>>>;

		int n_;									 // Number of vertices.
		std::vector<std::set<int>> adj_;		 // Adjacency list.
		std::vector<std::pair<int, int>> edges_; // Edge list.
		bool add_1_;   // If should add 1 for printing vertex ids.
		bool print_n_; // If should print n.
		std::optional<int> print_parents_; // If should print in parent style
										   // (stores the root).
		std::optional<std::vector<VWeight>> vertex_weights_; // Vertex weights.
		std::optional<std::vector<EWeight>>
			edge_weights_; // Edge weights (in same order as edges_).
		detail::dsu dsu_;  // Connectivity of current edges (for cycle checks).

		// Creates value from adjacency list.
		// O(n).
		value(const std::vector<std::set<int>> &adj)
			: n_(static_cast<int>(adj.size())), adj_(adj), add_1_(false),
			  print_n_(false), dsu_(n_) {
			for (int u = 0; u < n_; ++u)
				for (auto v : adj[u]) {
					tgen_ensure(
						0 <= v and v < n_,
						"wtree: value: vertices must be indexed in [0, n)");
					// Symmetric adjacency: count each undirected edge once.
					if (u < v) {
						edges_.emplace_back(u, v);
						tgen_ensure(
							dsu_.unite(u, v),
							"wtree: value: initial graph must form a tree");
					}
				}
		}

		// Creates value from `n` and edge list.
		// O(n).
		value(int n, const std::vector<std::pair<int, int>> &edges)
			: n_(n), adj_(n), add_1_(false), print_n_(false), dsu_(n) {
			edges_.reserve(edges.size());
			for (auto [u, v] : edges) {
				tgen_ensure(0 <= std::min(u, v) and std::max(u, v) < n,
							"wtree: value: vertices must be indexed in [0, n)");
				tgen_ensure(dsu_.unite(u, v),
							"wtree: value: initial graph must form a tree");
				if (u > v)
					std::swap(u, v);
				edges_.emplace_back(u, v);
				adj_[u].insert(v);
				adj_[v].insert(u);
			}
		}
		value(int n, const std::set<std::pair<int, int>> &edges)
			: value(n, std::vector<std::pair<int, int>>(edges.begin(),
														edges.end())) {}
		value(int n, const std::initializer_list<std::pair<int, int>> &edges)
			: value(n, std::vector<std::pair<int, int>>(edges)) {}

		// Creates tree from graph via Kruskal-like random spanning tree.
		// Implemented after wgraph definition.
		// O(n + m alpha(n)).
		value(const typename wgraph<VWeight, EWeight>::value &g);

		// Weight type conversion.
		// O(n).
		template <typename NewVWeight, typename NewEWeight>
		typename wtree<NewVWeight, NewEWeight>::value
		convert_weight_types() const {
			tgen_ensure(!vertex_weights_.has_value() and
							!edge_weights_.has_value(),
						"wtree: value: cannot convert weight type after "
						"assigning weights");

			typename wtree<NewVWeight, NewEWeight>::value new_tree(adj_);
			new_tree.add_1_ = add_1_;
			new_tree.print_n_ = print_n_;
			new_tree.print_parents_ = print_parents_;
			return new_tree;
		}

		// Fetches number of vertices.
		int n() const { return n_; }

		// Fetches a const ref. to adjacency list.
		const std::vector<std::set<int>> &adj() const { return adj_; }

		// Fetches a const ref. to edge list.
		const std::vector<std::pair<int, int>> &edges() const { return edges_; }

		// Fetches a const ref. to vertex weights.
		const std::optional<std::vector<VWeight>> &vertex_weights() const {
			return vertex_weights_;
		}

		// Fetches a const ref. to edge weights.
		const std::optional<std::vector<EWeight>> &edge_weights() const {
			return edge_weights_;
		}

		// Sets vertex weights.
		// O(n).
		template <typename NewVWeight = VWeight>
		typename wtree<NewVWeight, EWeight>::value set_vertex_weights(
			const std::vector<NewVWeight> &vertex_weights) const {
			tgen_ensure(static_cast<int>(vertex_weights.size()) == n(),
						"wtree: value: must give `n` vertex weights");

			auto new_tree = convert_weight_types<NewVWeight, EWeight>();
			new_tree.vertex_weights_ = vertex_weights;
			return new_tree;
		}

		// Sets edge weights.
		// O(n).
		template <typename NewEWeight = EWeight>
		typename wtree<VWeight, NewEWeight>::value
		set_edge_weights(const std::vector<NewEWeight> &edge_weights) const {
			tgen_ensure(
				edge_weights.size() == edges().size(),
				"wtree: value: must give `edges().size()` edge weights");

			auto new_tree = convert_weight_types<VWeight, NewEWeight>();
			new_tree.edge_weights_ = edge_weights;
			return new_tree;
		}

		// Enables edge-weighted mode before adding weighted edges
		// incrementally. The tree must have no edges yet. O(1).
		value &edge_weighted() {
			tgen_ensure(edges().size() == 0,
						"wtree: value: edge_weighted requires a tree with no "
						"edges");
			tgen_ensure(!edge_weights_.has_value(),
						"wtree: value: tree is already edge-weighted");

			edge_weights_ = std::vector<EWeight>();
			return *this;
		}

		// Adds 1 to vertex ids, for printing.
		// O(1).
		value &add_1() {
			add_1_ = true;
			return *this;
		}

		// Prints `n` on a new line before printing the tree.
		// O(1).
		value &print_n() {
			print_n_ = true;
			return *this;
		}

		// Prints the tree in parent style.
		// If root = -1, the root is considered to be 0, and its parent is not
		// printed. Otherwise, prints the parent of the root as -1. If root = n,
		// randomizes the root. O(1).
		value &print_parents(int root = -1) {
			tgen_ensure(root == -1 or (0 <= root and root < n()) or root == n(),
						"wtree: value: root must be -1, `n`, or in [0, n)");
			print_parents_ = root;
			return *this;
		}

		// Shuffles the tree's vertex labels (except those in `indices`,
		// which keep their current label) and edge order. The change is
		// applied eagerly to the underlying adjacency list, edge list,
		// vertex weights and edge weights.
		// O(n).
		value &shuffle_except(std::set<int> indices) {
			// Builds the relabeling: for each vertex `i`, `new_label[i]` is
			// its new id. Vertices in `indices` keep their label; the others
			// are permuted among themselves.
			std::vector<int> new_label(n());
			std::vector<int> shuffled;
			for (int i = 0; i < n(); ++i) {
				if (indices.count(i))
					new_label[i] = i;
				else
					shuffled.push_back(i);
			}
			std::vector<int> targets = shuffled;
			tgen::shuffle(targets.begin(), targets.end());
			for (size_t k = 0; k < shuffled.size(); ++k)
				new_label[shuffled[k]] = targets[k];

			// Rewrites adjacency list with new labels.
			std::vector<std::set<int>> new_adj(n());
			for (int u = 0; u < n(); ++u)
				for (int v : adj_[u])
					new_adj[new_label[u]].insert(new_label[v]);
			adj_ = std::move(new_adj);

			// Rewrites edges with new labels (canonical undirected order).
			for (auto &[u, v] : edges_) {
				u = new_label[u];
				v = new_label[v];
				if (u > v)
					std::swap(u, v);
			}

			// Permutes vertex weights to match the new labels.
			if (vertex_weights_.has_value()) {
				std::vector<VWeight> new_vw(n());
				for (int i = 0; i < n(); ++i)
					new_vw[new_label[i]] = (*vertex_weights_)[i];
				vertex_weights_ = std::move(new_vw);
			}

			// Rebuilds the dsu so future `add_edge` calls see the new labels.
			dsu_ = detail::dsu(n());
			for (auto [u, v] : edges_)
				dsu_.unite(u, v);

			// Shuffles edge order, keeping edge weights aligned.

			std::vector<int> perm(edges_.size());
			std::iota(perm.begin(), perm.end(), 0);
			tgen::shuffle(perm.begin(), perm.end());

			std::vector<std::pair<int, int>> new_edges;
			std::optional<std::vector<EWeight>> new_ew;
			if (edge_weights_.has_value())
				new_ew = std::vector<EWeight>();
			for (int i : perm) {
				new_edges.push_back(edges_[i]);
				if (new_ew.has_value())
					new_ew->push_back((*edge_weights_)[i]);
			}
			edges_ = new_edges;
			if (new_ew.has_value())
				edge_weights_ = new_ew;

			return *this;
		}

		// Shuffles the tree's vertices and edge order.
		// O(n).
		value &shuffle() { return shuffle_except({}); }

		// Adds edge (u, v).
		// O(log n) amortized.
		value &add_edge(int u, int v, std::optional<EWeight> w = std::nullopt) {
			tgen_ensure(0 <= std::min(u, v) and std::max(u, v) < n(),
						"wtree: value: vertex ids must be valid");

			if (u > v)
				std::swap(u, v);

			if (adj_[u].count(v))
				return *this;

			adj_[u].insert(v);
			adj_[v].insert(u);
			edges_.emplace_back(u, v);
			tgen_ensure(dsu_.unite(u, v),
						"wtree: value: added edge must not create a cycle");

			if (w.has_value()) {
				tgen_ensure(edge_weights().has_value(),
							"wtree: value: cannot add weighted edge to "
							"edge-unweighted tree");

				edge_weights_->push_back(*w);
			} else
				tgen_ensure(!edge_weights().has_value(),
							"wtree: value: cannot add unweighted edge to "
							"edge-weighted tree");

			return *this;
		}

		// Links tree with another `rhs`, adding the edge between u (in left
		// tree) and v (in right tree). Ids for added vertices are updated
		// accordingly.
		// O(rhs.n + rhs.m * log n) amortized.
		value &link(const value &rhs, int new_u, int new_v,
					std::optional<EWeight> new_w = std::nullopt) {
			tgen_ensure(0 <= new_u and new_u < n() and 0 <= new_v and
							new_v < rhs.n(),
						"wtree: value: vertex ids must be valid");

			// Edges from right-hand side.
			int shift = n();
			add_vertices(rhs.n(), rhs.vertex_weights());
			for (int i = 0; i < static_cast<int>(rhs.edges().size()); ++i) {
				auto [u, v] = rhs.edges()[i];
				add_edge(shift + u, shift + v,
						 rhs.edge_weights().has_value()
							 ? std::optional<EWeight>((*rhs.edge_weights())[i])
							 : std::nullopt);
			}

			// New edge.
			add_edge(new_u, shift + new_v, new_w);

			return *this;
		}

		// Glues the tree with another `rhs` such that index_pairs[i].first is
		// considered to be the same as index_pairs[i].second. Ids for added
		// vertices are updated accordingly.
		// O(rhs.n + rhs.m * log n) amortized.
		value &glue(const value &rhs,
					std::set<std::pair<int, int>> index_pairs) {
			// Checks validity of indices.
			std::set<int> idx_left, idx_right;
			std::vector<int> right_id_to_left(rhs.n(), -1);
			for (auto [l, r] : index_pairs) {
				tgen_ensure(
					0 <= l and l < n() and 0 <= r and r < rhs.n(),
					"wtree: value: vertex indices to glue must be valid");
				tgen_ensure(idx_left.count(l) == 0 and idx_right.count(r) == 0,
							"wtree: value: must not have repeated indices "
							"on the same side to glue");

				idx_left.insert(l);
				idx_right.insert(r);
				right_id_to_left[r] = l;
			}

			// Computes new ids of right vertices.
			std::vector<int> new_right_id(rhs.n(), -1);
			int intersection_lt = 0;
			std::optional<std::vector<VWeight>> rhs_vertex_weights;
			for (int i = 0; i < rhs.n(); ++i) {
				if (right_id_to_left[i] != -1) {
					// Is in intersection.
					++intersection_lt;
					new_right_id[i] = right_id_to_left[i];
				} else {
					// New id.
					new_right_id[i] = n() + i - intersection_lt;
					if (rhs.vertex_weights().has_value()) {
						if (!rhs_vertex_weights.has_value())
							rhs_vertex_weights = std::vector<VWeight>();
						rhs_vertex_weights->push_back(
							(*rhs.vertex_weights())[i]);
					}
				}
			}

			// Adds new vertices and edges.
			add_vertices(rhs.n() - intersection_lt, rhs_vertex_weights);
			for (int i = 0; i < static_cast<int>(rhs.edges().size()); ++i) {
				auto [u, v] = rhs.edges()[i];
				add_edge(new_right_id[u], new_right_id[v],
						 rhs.edge_weights().has_value()
							 ? std::optional<EWeight>((*rhs.edge_weights())[i])
							 : std::nullopt);
			}

			return *this;
		}
		value &glue(const value &rhs,
					std::initializer_list<std::pair<int, int>> il) {
			return glue(rhs, std::set<std::pair<int, int>>(il));
		}

		// Glues the tree with another `rhs` at `indices`. That is, idx in
		// `indices` are considered to be the same vertex. Ids for added
		// vertices are updated accordingly.
		// O(rhs.n).
		value &glue(const value &rhs, std::set<int> indices) {
			std::set<std::pair<int, int>> index_pairs;
			for (auto i : indices)
				index_pairs.emplace(i, i);
			return glue(rhs, index_pairs);
		}
		value &glue(const value &rhs, const std::initializer_list<int> &il) {
			return glue(rhs, std::set<int>(il));
		}

		// Prints to std::ostream.
		// O(n).
		friend std::ostream &operator<<(std::ostream &out, const value &val) {
			if (val.print_n_)
				out << val.n() << '\n';

			// Prints vertex weights.
			if (val.vertex_weights()) {
				for (int i = 0; i < val.n(); ++i) {
					if (i > 0)
						out << " ";
					out << (*val.vertex_weights())[i];
				}
				out << '\n';
			}

			tgen_ensure(static_cast<int>(val.edges().size()) == val.n() - 1,
						"wtree: value: invalid tree to print (number of edges "
						"must be `n` - 1)");

			// Prints in parent style.
			if (val.print_parents_.has_value()) {
				tgen_ensure(!val.edge_weights().has_value(),
							"wtree: value: cannot print parent style if edges "
							"are weighted");

				int root = *val.print_parents_;
				bool skip_parent_0 = root == -1;
				if (root == -1)
					root = 0;
				if (root == val.n())
					root = next(0, val.n() - 1);

				std::vector<int> parent(val.n(), -1);

				std::queue<int> q;
				std::vector<int> vis(val.n(), false);
				q.push(root);
				vis[root] = true;

				while (q.size()) {
					int u = q.front();
					q.pop();
					for (int v : val.adj()[u])
						if (!vis[v]) {
							vis[v] = true;
							q.push(v);
							parent[v] = u;
						}
				}

				if (skip_parent_0) {
					for (int i = 1; i < val.n(); ++i) {
						tgen_ensure(
							parent[i] < i,
							"wtree: value: parent of i must be less than i for "
							"printing in parent style if root is -1");

						if (i > 1)
							out << " ";
						out << parent[i] + val.add_1_;
					}
				} else {
					for (int i = 0; i < val.n(); ++i) {
						if (i > 0)
							out << " ";
						out << (parent[i] == -1 ? -1 : parent[i]) + val.add_1_;
					}
				}

				out << '\n';
				return out;
			}

			// Prints edges.
			for (int i = 0; i < static_cast<int>(val.edges().size()); ++i) {
				auto [u, v] = val.edges()[i];
				out << (u + val.add_1_) << " " << (v + val.add_1_);

				// Edge weight.
				if (val.edge_weights().has_value())
					out << " " << (*val.edge_weights())[i];

				out << '\n';
			}

			return out;
		}

		// Gets a std::pair<n, adj> representing the value.
		std::pair<int, std::vector<std::set<int>>> to_std() const {
			return std_type(n_, adj_);
		}

	  private:
		// Adds `k` vertices to the tree (labeled n, n+1, ...n+k-1). Updates
		// `n` accordingly. This makes the tree invalid (not a tree anymore).
		// O(k) amortized.
		value &add_vertices(int k, std::optional<std::vector<VWeight>>
									   new_vertex_weights = std::nullopt) {
			n_ += k;
			adj_.resize(n());
			if (new_vertex_weights.has_value()) {
				tgen_ensure(vertex_weights().has_value(),
							"wtree: value: cannot add weighted vertices to "
							"vertex-unweighted tree");
				tgen_ensure(
					static_cast<int>(new_vertex_weights->size()) == k,
					"wtree: value: number of vertex weights must be equal "
					"to number of added vertices");

				vertex_weights_->insert(vertex_weights_->end(),
										new_vertex_weights->begin(),
										new_vertex_weights->end());
			} else
				tgen_ensure(!vertex_weights().has_value(),
							"wtree: value: cannot add unweighted vertices to "
							"vertex-weighted tree");

			dsu_.add_elements(k);

			return *this;
		}
	};

	// Generates tree value.
	// O(n).
	value gen() const {
		// Constructs adjacency list.
		std::vector<std::vector<int>> adj(n_);
		for (auto [u, v] : edges_) {
			adj[u].push_back(v);
			adj[v].push_back(u);
		}

		std::vector<int> comp_size;
		std::vector<std::vector<int>> component_ids;
		std::vector<bool> vis(n_, false);
		std::queue<int> q;

		for (int i = 0; i < n_; ++i) {
			if (vis[i])
				continue;

			vis[i] = true;
			q.push(i);
			comp_size.push_back(0);
			component_ids.emplace_back();
			while (q.size()) {
				int u = q.front();
				q.pop();
				++comp_size.back();
				component_ids.back().push_back(u);
				for (int v : adj[u]) {
					if (!vis[v]) {
						vis[v] = true;
						q.push(v);
					}
				}
			}
		}

		// Creates edges connecting the connected components by treating them as
		// vertices.
		std::vector<std::pair<int, int>> new_edges(edges_.begin(),
												   edges_.end());
		if (comp_size.size() > 1) {
			std::vector<int> prufer_values =
				many_by_distribution(comp_size.size() - 2, comp_size);
			for (auto [u, v] : detail::edges_from_prufer(prufer_values))
				new_edges.emplace_back(pick(component_ids[u]),
									   pick(component_ids[v]));
		}

		return value(n_, new_edges);
	}

	// Generates a (not uniformly) random skewed tree.
	// Vertex 0 is the root. For each i in 1 .. n-1, parent(i) is
	// wnext(i, elongation), i.e. a value in [0, i) with skew controlled by
	// elongation (see wnext).
	// If elongation is small enough, generates a star (center 0).
	// If elongation is large enough, generates a path (endpoints 0 and n-1).
	// O(n).
	static value gen_skewed(int n, int elongation) {
		std::vector<std::pair<int, int>> edges;
		for (int i = 1; i < n; ++i)
			edges.emplace_back(i, wnext<int>(i, elongation));
		return value(n, edges);
	}

	// Kruskal-like random tree: random vertex pairs until connected.
	// Not uniformly random.
	// O(n log(n) alpha(n)) expected.
	static value gen_kruskal(int n) {
		tgen_ensure(n > 0, "wtree: gen_kruskal: n must be positive");
		if (n == 1)
			return value(1, {});

		detail::dsu components(n);
		std::vector<std::pair<int, int>> edges;
		edges.reserve(n - 1);
		while (edges.size() < size_t(n - 1)) {
			int u = next(0, n - 1);
			int v = next(0, n - 1);
			if (u == v)
				continue;
			if (components.unite(u, v))
				edges.emplace_back(u, v);
		}
		return value(n, edges);
	}
};

/*
 * Other types of weighted-ness.
 */

// Vertex weighted tree.
template <typename VWeight> using vtree = wtree<VWeight, int>;

// Edge weighted tree.
template <typename EWeight> using etree = wtree<int, EWeight>;

// Unweighted tree.
using tree = wtree<int, int>;

/*************
 *           *
 *   GRAPH   *
 *           *
 *************/

namespace detail {

// Canonical undirected edge key for duplicate detection; stores (min(u, v),
// max(u, v)). O(1).
inline uint64_t undirected_edge_key(int u, int v) {
	if (u > v)
		std::swap(u, v);
	return (static_cast<uint64_t>(u) << 32) |
		   static_cast<uint64_t>(static_cast<uint32_t>(v));
}

// Directed edge key for duplicate detection; stores (u, v).
// O(1).
inline uint64_t directed_edge_key(int u, int v) {
	return (static_cast<uint64_t>(u) << 32) |
		   static_cast<uint64_t>(static_cast<uint32_t>(v));
}

// Maximum number of edges in a simple graph on n vertices.
// O(1).
inline long long max_graph_edges(int n, bool directed, bool self_loops) {
	if (n <= 0)
		return 0;
	if (directed)
		return self_loops ? static_cast<long long>(n) * n
						  : static_cast<long long>(n) * (n - 1);
	return self_loops ? static_cast<long long>(n) * (n + 1) / 2
					  : static_cast<long long>(n) * (n - 1) / 2;
}

// Uniform random edge for rejection sampling.
// O(1) expected.
inline std::pair<int, int> get_random_graph_edge(int n, bool directed,
												 bool self_loops) {
	if (directed) {
		if (self_loops)
			return {next<int>(0, n - 1), next<int>(0, n - 1)};
		int u = next<int>(0, n - 1);
		int v = next<int>(0, n - 1);
		while (u == v)
			v = next<int>(0, n - 1);
		return {u, v};
	}
	if (self_loops) {
		int u = next<int>(0, n - 1);
		int v = next<int>(0, n - 1);
		if (u > v)
			std::swap(u, v);
		return {u, v};
	}
	int u = next<int>(0, n - 1);
	int v = next<int>(0, n - 1);
	while (u == v)
		v = next<int>(0, n - 1);
	if (u > v)
		std::swap(u, v);
	return {u, v};
}

// Decodes a linear edge index to (u, v) for an undirected simple graph,
// with u < v.
// O(log n).
inline std::pair<int, int> decode_undirected_simple_edge(int n, long long idx) {
	auto base = [&](int u) -> long long {
		return static_cast<long long>(u) * (n - 1) -
			   static_cast<long long>(u) * (u - 1) / 2;
	};
	int lo = 0, hi = n - 2;
	while (lo < hi) {
		int mid = (lo + hi + 1) / 2;
		if (base(mid) <= idx)
			lo = mid;
		else
			hi = mid - 1;
	}
	return {lo, lo + 1 + int(idx - base(lo))};
}

// Decodes a linear edge index to (u, v) for an undirected graph with loops,
// with u <= v.
// O(log n).
inline std::pair<int, int> decode_undirected_loops_edge(int n, long long idx) {
	auto base = [&](int u) -> long long {
		return static_cast<long long>(u) * n -
			   static_cast<long long>(u) * (u - 1) / 2;
	};
	int lo = 0, hi = n - 1;
	while (lo < hi) {
		int mid = (lo + hi + 1) / 2;
		if (base(mid) <= idx)
			lo = mid;
		else
			hi = mid - 1;
	}
	return {lo, lo + int(idx - base(lo))};
}

// Decodes a linear edge index to (u, v) for a directed simple graph (no loops).
// O(1).
inline std::pair<int, int> decode_directed_simple_edge(int n, long long idx) {
	int u = idx / (n - 1);
	int rem = idx % (n - 1);
	return {u, rem + (rem >= u)};
}

// Decodes a linear edge index according to graph mode.
// O(log n) for undirected, O(1) for directed.
inline std::pair<int, int>
decode_graph_edge_index(int n, long long idx, bool directed, bool self_loops) {
	if (directed) {
		if (self_loops)
			return {int(idx / n), int(idx % n)};
		return decode_directed_simple_edge(n, idx);
	}
	if (self_loops)
		return decode_undirected_loops_edge(n, idx);
	return decode_undirected_simple_edge(n, idx);
}

} // namespace detail

/*
 * Graph generator.
 *
 * Graphs of `n` vertices labeled from 0 to n-1 and `m` edges.
 * These are labeled graphs, that is, isomorphism is not taken into
 * account. VWeight is the type of vertex weights, and EWeight is the type of
 * edge weights. Generator does not generate weights. The weights are to be set
 * in the wgraph::value.
 */

template <typename VWeight, typename EWeight>
struct wgraph : gen_base<wgraph<VWeight, EWeight>> {
	int n_, m_;							  // Number of vertices and edges.
	std::set<std::pair<int, int>> edges_; // Edges that were set.
	bool is_directed_;					  // If graph is directed.
	bool has_self_loops_;				  // If self-loops are allowed.

	// Creates graph generator with `n` vertices and `m` edges.
	// Additionally, you can set if the graph is directed and if self loops are
	// allowed.
	// O(1).
	wgraph(int n, int m, bool is_directed = false, bool has_self_loops = false)
		: n_(n), m_(m), is_directed_(is_directed),
		  has_self_loops_(has_self_loops) {
		tgen_ensure(n > 0, "wgraph: number of vertices must be positive");
	}

	// Adds edge between u and v (this edge must be generated).
	// O(log m).
	wgraph &add_edge(int u, int v) {
		tgen_ensure(0 <= std::min(u, v) and std::max(u, v) < n_,
					"wgraph: vertices must be indexed in [0, n)");

		if (!is_directed_ and u > v)
			std::swap(u, v);
		edges_.emplace(u, v);
		tgen_ensure(static_cast<int>(edges_.size()) <= m_,
					"wgraph: too many edges were added");
		return *this;
	}

	// Graph value.
	//
	// Edges are stored in both directions (if undirected) in adjacency list,
	// but only u < v in edge list.
	// Optimized for performance (lazy adjacency list; edge-list constructor
	// stores edges only).
	struct value : gen_value_base<value> {
		using std_type = std::tuple<int, int, std::vector<std::set<int>>>;

		int n_;									 // Number of vertices.
		std::vector<std::set<int>> adj_;		 // Adjacency list.
		std::vector<std::pair<int, int>> edges_; // Edge list.
		bool is_directed_;						 // If graph is directed.
		bool add_1_;	// If should add 1 for printing vertex ids.
		bool print_nm_; // If should print n and m.
		mutable bool adj_built_{
			false}; // Lazy cache: true once adj_ is built from edges_; mutable
					// so const adj() can populate it.
		std::optional<std::vector<VWeight>> vertex_weights_; // Vertex weights.
		std::optional<std::vector<EWeight>>
			edge_weights_; // Edge weights (in same order as edges_ ).

		// Creates value from adjacency list. The edges
		// are considered to be directed.
		// O(n + m).
		value(const std::vector<std::set<int>> &adj, bool is_directed = false)
			: n_(static_cast<int>(adj.size())), adj_(adj),
			  is_directed_(is_directed), add_1_(false), print_nm_(false),
			  adj_built_(true) {
			for (int u = 0; u < n_; ++u)
				for (auto v : adj[u]) {
					tgen_ensure(
						0 <= v and v < n_,
						"wgraph: value: vertices must be indexed in [0, n)");
					// Undirected adjacency is symmetric: count each edge once
					// (canonical u <= v). Directed: every out-edge appears
					// once.
					if (is_directed_ or u <= v)
						edges_.emplace_back(u, v);
				}
		}

		// Creates value from `n`, `m`, and edge list. The edges are
		// considered to be directed.
		// Optimized for performance (lazy adjacency list; unordered_set dedup).
		// O(m log m).
		value(int n, const std::vector<std::pair<int, int>> &edges = {},
			  bool is_directed = false)
			: n_(n), edges_(), is_directed_(is_directed), add_1_(false),
			  print_nm_(false), adj_built_(false) {
			edges_.reserve(edges.size());
			std::unordered_set<uint64_t> seen;
			seen.reserve(edges.size() * 2 + 1);
			for (auto [u, v] : edges) {
				tgen_ensure(
					0 <= std::min(u, v) and std::max(u, v) < n,
					"wgraph: value: vertices must be indexed in [0, n)");
				if (!is_directed_ and u > v)
					std::swap(u, v);
				uint64_t key = is_directed_ ? detail::directed_edge_key(u, v)
											: detail::undirected_edge_key(u, v);
				if (seen.insert(key).second)
					edges_.emplace_back(u, v);
			}
		}
		value(int n, const std::set<std::pair<int, int>> &edges,
			  bool is_directed = false)
			: value(
				  n,
				  std::vector<std::pair<int, int>>(edges.begin(), edges.end()),
				  is_directed) {}
		value(int n, const std::initializer_list<std::pair<int, int>> &edges,
			  bool is_directed = false)
			: value(n, std::vector<std::pair<int, int>>(edges), is_directed) {}

		// Creates graph from tree (undirected, same edges).
		// O(n).
		value(const typename wtree<VWeight, EWeight>::value &t)
			: value(t.n(), t.edges(), false) {
			if (t.vertex_weights().has_value()) {
				vertex_weights_ = *t.vertex_weights();
			}
			if (t.edge_weights().has_value()) {
				edge_weights_ = *t.edge_weights();
			}
		}

		// Weight type conversion.
		// O(n + m).
		template <typename NewVWeight, typename NewEWeight>
		typename wgraph<NewVWeight, NewEWeight>::value
		convert_weight_types() const {
			tgen_ensure(!vertex_weights_.has_value() and
							!edge_weights_.has_value(),
						"wgraph: value: cannot convert weight type after "
						"assigning weights");

			ensure_adj_built();
			typename wgraph<NewVWeight, NewEWeight>::value new_graph(
				adj_, is_directed_);
			new_graph.is_directed_ = is_directed_;
			new_graph.add_1_ = add_1_;
			new_graph.print_nm_ = print_nm_;
			return new_graph;
		}

		// Fetches number of vertices.
		int n() const { return n_; }

		// Fetches number of edges.
		int m() const { return edges_.size(); }

		// Fetches if graph is directed;
		bool is_directed() const { return is_directed_; }

		// Fetches a const ref. to adjacency list.
		const std::vector<std::set<int>> &adj() const {
			ensure_adj_built();
			return adj_;
		}

		// Fetches a const ref. to edge set.
		const std::vector<std::pair<int, int>> &edges() const { return edges_; }

		// Fetches vertex weights.
		const std::optional<std::vector<VWeight>> &vertex_weights() const {
			return vertex_weights_;
		}

		// Fetches edge weights.
		const std::optional<std::vector<EWeight>> &edge_weights() const {
			return edge_weights_;
		}

		// Sets vertex weights.
		// O(n + m).
		template <typename NewVWeight = VWeight>
		typename wgraph<NewVWeight, EWeight>::value set_vertex_weights(
			const std::vector<NewVWeight> &vertex_weights) const {
			tgen_ensure(static_cast<int>(vertex_weights.size()) == n(),
						"wgraph: value: must give `n` vertex weights");

			auto new_graph = convert_weight_types<NewVWeight, EWeight>();
			new_graph.vertex_weights_ = vertex_weights;
			return new_graph;
		}

		// Sets edge weights.
		// O(n + m).
		template <typename NewEWeight = EWeight>
		typename wgraph<VWeight, NewEWeight>::value
		set_edge_weights(const std::vector<NewEWeight> &edge_weights) const {
			tgen_ensure(static_cast<int>(edge_weights.size()) == m(),
						"wgraph: value: must give `m` edge weights");

			auto new_graph = convert_weight_types<VWeight, NewEWeight>();
			new_graph.edge_weights_ = edge_weights;
			return new_graph;
		}

		// Enables edge-weighted mode before adding weighted edges
		// incrementally. The graph must have no edges yet. O(1).
		value &edge_weighted() {
			tgen_ensure(m() == 0,
						"wgraph: value: edge_weighted requires a graph with no "
						"edges");
			tgen_ensure(!edge_weights_.has_value(),
						"wgraph: value: graph is already edge-weighted");

			edge_weights_ = std::vector<EWeight>();
			return *this;
		}

		// Adds 1 to vertex ids, for printing.
		// O(1).
		value &add_1() {
			add_1_ = true;
			return *this;
		}

		// Prints `n m` on a new line before printing the edges.
		// O(1).
		value &print_nm() {
			print_nm_ = true;
			return *this;
		}

		// Shuffles the graph's vertex labels (except those in `indices`,
		// which keep their current label) and edge order. The change is
		// applied eagerly to the underlying adjacency list, edge list,
		// vertex weights and edge weights.
		// O(n + m).
		value &shuffle_except(std::set<int> indices) {
			ensure_adj_built();
			// Builds the relabeling: for each vertex `i`, `new_label[i]` is
			// its new id. Vertices in `indices` keep their label; the others
			// are permuted among themselves.
			std::vector<int> new_label(n());
			std::vector<int> shuffled;
			for (int i = 0; i < n(); ++i) {
				if (indices.count(i))
					new_label[i] = i;
				else
					shuffled.push_back(i);
			}
			std::vector<int> targets = shuffled;
			tgen::shuffle(targets.begin(), targets.end());
			for (size_t k = 0; k < shuffled.size(); ++k)
				new_label[shuffled[k]] = targets[k];

			// Rewrites adjacency list with new labels.
			std::vector<std::set<int>> new_adj(n());
			for (int u = 0; u < n(); ++u)
				for (int v : adj_[u])
					new_adj[new_label[u]].insert(new_label[v]);
			adj_ = new_adj;

			// Rewrites edges with new labels (canonical undirected order).
			for (auto &[u, v] : edges_) {
				u = new_label[u];
				v = new_label[v];
				if (!is_directed_ and u > v)
					std::swap(u, v);
			}

			// Permutes vertex weights to match the new labels.
			if (vertex_weights_.has_value()) {
				std::vector<VWeight> new_vw(n());
				for (int i = 0; i < n(); ++i)
					new_vw[new_label[i]] = (*vertex_weights_)[i];
				vertex_weights_ = new_vw;
			}

			// Shuffles edge order, keeping edge weights aligned.

			std::vector<int> perm(edges_.size());
			std::iota(perm.begin(), perm.end(), 0);
			tgen::shuffle(perm.begin(), perm.end());

			std::vector<std::pair<int, int>> new_edges;
			std::optional<std::vector<EWeight>> new_ew;
			if (edge_weights_.has_value())
				new_ew = std::vector<EWeight>();
			for (int i : perm) {
				new_edges.push_back(edges_[i]);
				if (new_ew.has_value())
					new_ew->push_back((*edge_weights_)[i]);
			}

			edges_ = new_edges;
			if (new_ew.has_value())
				edge_weights_ = new_ew;

			return *this;
		}

		// Shuffles the graph's vertices and edge order.
		// O(n + m).
		value &shuffle() { return shuffle_except({}); }

		// Adds `k` vertices to the graph (labeled n, n+1, ...n+k-1). Updates
		// `n` accordingly.
		// O(k) amortized.
		value &add_vertices(int k, std::optional<std::vector<VWeight>>
									   new_vertex_weights = std::nullopt) {
			ensure_adj_built();
			n_ += k;
			adj_.resize(n());
			if (new_vertex_weights.has_value()) {
				tgen_ensure(vertex_weights().has_value(),
							"wgraph: value: cannot add weighted vertices to "
							"vertex-unweighted graph");
				tgen_ensure(
					static_cast<int>(new_vertex_weights->size()) == k,
					"wgraph: value: number of vertex weights must be equal "
					"to number of added vertices");

				vertex_weights_->insert(vertex_weights_->end(),
										new_vertex_weights->begin(),
										new_vertex_weights->end());
			} else
				tgen_ensure(!vertex_weights().has_value(),
							"wgraph: value: cannot add unweighted vertices to "
							"vertex-weighted graph");

			return *this;
		}

		// Adds edge (u, v).
		// O(log n) amortized.
		value &add_edge(int u, int v, std::optional<EWeight> w = std::nullopt) {
			ensure_adj_built();
			tgen_ensure(0 <= std::min(u, v) and std::max(u, v) < n(),
						"wgraph: value: vertex ids must be valid");

			if (!is_directed() and u > v)
				std::swap(u, v);

			if (adj_[u].count(v))
				return *this;

			adj_[u].insert(v);
			if (!is_directed())
				adj_[v].insert(u);
			edges_.emplace_back(u, v);

			if (w.has_value()) {
				tgen_ensure(edge_weights().has_value(),
							"wgraph: value: cannot add weighted edge to "
							"edge-unweighted graph");

				edge_weights_->push_back(*w);
			} else
				tgen_ensure(!edge_weights().has_value(),
							"wgraph: value: cannot add unweighted edge to "
							"edge-weighted graph");

			return *this;
		}

		// Links graph with another `rhs`, adding the edge between u (in left
		// graph) and v (in right graph). Ids for added vertices are updated
		// accordingly.
		// O(rhs.n + rhs.m * log n) amortized.
		value &link(const value &rhs, int new_u, int new_v,
					std::optional<EWeight> new_w = std::nullopt) {
			tgen_ensure(0 <= new_u and new_u < n() and 0 <= new_v and
							new_v < rhs.n(),
						"wgraph: value: vertex ids must be valid");

			// Edges from right-hand side.
			int shift = n();
			add_vertices(rhs.n(), rhs.vertex_weights());
			for (int i = 0; i < rhs.m(); ++i) {
				auto [u, v] = rhs.edges()[i];
				add_edge(shift + u, shift + v,
						 rhs.edge_weights().has_value()
							 ? std::optional<EWeight>((*rhs.edge_weights())[i])
							 : std::nullopt);
			}

			// New edge.
			add_edge(new_u, shift + new_v, new_w);

			return *this;
		}

		// Glues the graph with another `rhs` such that index_pairs[i].first is
		// considered to be the same as index_pairs[i].second. Ids for added
		// vertices are updated accordingly.
		// O(rhs.n + rhs.m * log n) amortized.
		value &glue(const value &rhs,
					std::set<std::pair<int, int>> index_pairs) {
			tgen_ensure(
				is_directed() == rhs.is_directed(),
				"wgraph: value: graphs must have the same is_directed value");

			// Checks validity of indices.
			std::set<int> idx_left, idx_right;
			std::vector<int> right_id_to_left(rhs.n(), -1);
			for (auto [l, r] : index_pairs) {
				tgen_ensure(
					0 <= l and l < n() and 0 <= r and r < rhs.n(),
					"wgraph: value: vertex indices to glue must be valid");
				tgen_ensure(idx_left.count(l) == 0 and idx_right.count(r) == 0,
							"wgraph: value: must not have repeated indices "
							"on the same side to glue");

				idx_left.insert(l);
				idx_right.insert(r);
				right_id_to_left[r] = l;
			}

			// Computes new ids of right vertices.
			std::vector<int> new_right_id(rhs.n(), -1);
			int intersection_lt = 0;
			std::optional<std::vector<VWeight>> rhs_vertex_weights;
			for (int i = 0; i < rhs.n(); ++i) {
				if (right_id_to_left[i] != -1) {
					// Is in intersection.
					++intersection_lt;
					new_right_id[i] = right_id_to_left[i];
				} else {
					// New id.
					new_right_id[i] = n() + i - intersection_lt;
					if (rhs.vertex_weights().has_value()) {
						if (!rhs_vertex_weights.has_value())
							rhs_vertex_weights = std::vector<VWeight>();
						rhs_vertex_weights->push_back(
							(*rhs.vertex_weights())[i]);
					}
				}
			}

			// Adds new vertices and edges.
			add_vertices(rhs.n() - intersection_lt, rhs_vertex_weights);
			for (int i = 0; i < rhs.m(); ++i) {
				auto [u, v] = rhs.edges()[i];
				add_edge(new_right_id[u], new_right_id[v],
						 rhs.edge_weights().has_value()
							 ? std::optional<EWeight>((*rhs.edge_weights())[i])
							 : std::nullopt);
			}

			return *this;
		}
		value &glue(const value &rhs,
					std::initializer_list<std::pair<int, int>> il) {
			return glue(rhs, std::set<std::pair<int, int>>(il));
		}

		// Glues the graph with another `rhs` at `indices`. That is, idx in
		// `indices` are considered to be the same vertex. Ids for added
		// vertices are updated accordingly.
		// O(rhs.n + rhs.m * log n) amortized.
		value &glue(const value &rhs, std::set<int> indices) {
			std::set<std::pair<int, int>> index_pairs;
			for (auto i : indices)
				index_pairs.emplace(i, i);
			return glue(rhs, index_pairs);
		}
		value &glue(const value &rhs, const std::initializer_list<int> &il) {
			return glue(rhs, std::set<int>(il));
		}

		// Disjoint union.
		// Shifts ids from `rhs` graph by n().
		// O(rhs.n + rhs.m * log n) amortized.
		value &disjoint_union(const value &rhs) {
			return glue(rhs, std::set<int>());
		}

		// Computes uniformly random subgraph of graph with num_edges edges.
		// O(n + m).
		value &random_subgraph(int num_edges) {
			tgen_ensure(
				num_edges <= m(),
				"wgraph: value: can choose at most `m` edges from graph");

			std::vector<std::pair<int, int>> new_edges;
			std::optional<std::vector<EWeight>> new_edge_weights;

			int left = m();
			for (int i = 0; i < m(); ++i) {
				if (next(1, left--) <= num_edges) {
					new_edges.push_back(edges()[i]);
					if (edge_weights_.has_value()) {
						if (!new_edge_weights.has_value())
							new_edge_weights = std::vector<EWeight>();
						new_edge_weights->push_back((*edge_weights())[i]);
					}
					--num_edges;
				}
			}

			edges_ = new_edges;
			edge_weights_ = new_edge_weights;
			rebuild_adj_from_edge_list();
			return *this;
		}

		// Computes a random (not uniform) subgraph with `num_edges` edges that
		// keeps every connected component connected (does not increase the
		// number of connected components).
		// 1. Picks a spanning forest via randomized Prim.
		// 2. Adds additional edges uniformly at random.
		// O(n + m).
		value &random_connected_subgraph(int num_edges) {
			tgen_ensure(!is_directed_,
						"wgraph: value: random_connected_subgraph is only for "
						"undirected graphs");
			tgen_ensure(
				num_edges <= m(),
				"wgraph: value: can choose at most `m` edges from graph");

			// Builds an incidence list: for each vertex, the (neighbor, edge
			// index) pairs.
			std::vector<std::vector<std::pair<int, int>>> incident(n());
			for (int i = 0; i < m(); ++i) {
				auto [u, v] = edges_[i];
				incident[u].emplace_back(v, i);
				incident[v].emplace_back(u, i);
			}

			// Randomized Prim.
			std::vector<bool> vis(n(), false);
			std::vector<int> queue;
			std::vector<bool> in_tree(m(), false);
			int forest_edges = 0;

			for (int start = 0; start < n(); ++start) {
				if (vis[start])
					continue;
				vis[start] = true;
				queue.push_back(start);

				while (!queue.empty()) {
					int i = tgen::next<int>(0, queue.size() - 1);
					int u = queue[i];
					std::swap(queue[i], queue.back());
					queue.pop_back();

					for (auto [v, edge_idx] : incident[u]) {
						if (!vis[v]) {
							vis[v] = true;
							queue.push_back(v);
							in_tree[edge_idx] = true;
							++forest_edges;
						}
					}
				}
			}
			tgen_ensure(
				num_edges >= forest_edges,
				"wgraph: value: random_connected_subgraph needs at least "
				"`n - c` edges, where `c` is the number of connected "
				"components");

			// Splits edge indices into forest edges and the rest.
			std::vector<int> tree_idx, rest_idx;
			for (int i = 0; i < m(); ++i) {
				if (in_tree[i])
					tree_idx.push_back(i);
				else
					rest_idx.push_back(i);
			}

			tgen::shuffle(rest_idx.begin(), rest_idx.end());

			std::vector<int> chosen_idx;
			chosen_idx.insert(chosen_idx.end(), tree_idx.begin(),
							  tree_idx.end());
			chosen_idx.insert(chosen_idx.end(), rest_idx.begin(),
							  rest_idx.begin() + num_edges - forest_edges);

			detail::tgen_ensure_against_bug(
				static_cast<int>(chosen_idx.size()) == num_edges,
				"wgraph: value: chose a wrong number of edges");

			std::vector<std::pair<int, int>> new_edges;
			std::optional<std::vector<EWeight>> new_edge_weights;
			if (edge_weights_.has_value())
				new_edge_weights = std::vector<EWeight>();
			for (int i : chosen_idx) {
				new_edges.push_back(edges_[i]);
				if (new_edge_weights.has_value())
					new_edge_weights->push_back((*edge_weights_)[i]);
			}

			edges_ = new_edges;
			edge_weights_ = new_edge_weights;
			rebuild_adj_from_edge_list();
			return *this;
		}

		// Complement. Self loops are maintained.
		// O(n^2).
		value operator!() const {
			tgen_ensure(!edge_weights_.has_value(),
						"wgraph: value: cannot compute complement of "
						"edge-weighted graph");

			value complement = *this;
			complement.ensure_adj_built();
			std::vector<std::pair<int, int>> compl_edges;
			for (int i = 0; i < complement.n_; ++i) {
				std::set<int> complement_adj;
				for (int j = 0; j < complement.n_; ++j) {
					bool add_j = false;
					if (j == i and complement.adj_[i].count(j))
						add_j = true;
					if (j != i and !complement.adj_[i].count(j))
						add_j = true;

					if (add_j) {
						complement_adj.insert(j);
						// If i > j and !is_directed(), we don't add the edge.
						if (i <= j or complement.is_directed_) {
							compl_edges.emplace_back(i, j);
						}
					}
				}
				std::swap(complement.adj_[i], complement_adj);
			}
			std::swap(complement.edges_, compl_edges);

			return complement;
		}

		// Concatenates two values.
		// O(N + M log N), N = n + rhs.n, M = m + rhs.m.
		value operator+(const value &rhs) const {
			tgen_ensure(is_directed() == rhs.is_directed(),
						"wgraph: value: graphs must have the same "
						"is_directed value");

			tgen_ensure(vertex_weights().has_value() ==
							rhs.vertex_weights().has_value(),
						"wgraph: value: cannot concatenate vertex-weighted "
						"wgraph to unweighted");
			tgen_ensure(edge_weights().has_value() ==
							rhs.edge_weights().has_value(),
						"wgraph: value: cannot concatenate edge-weighted "
						"wgraph to unweighted");

			value concat = *this;
			concat.glue(rhs, std::set<std::pair<int, int>>());
			concat.add_1_ = add_1_ | rhs.add_1_;
			concat.print_nm_ = print_nm_ | rhs.print_nm_;

			return concat;
		}

		// Prints to std::ostream.
		// O(n + m).
		friend std::ostream &operator<<(std::ostream &out, const value &val) {
			// Prints `n` and `m`.
			if (val.print_nm_)
				out << val.n() << " " << val.m() << '\n';

			// Prints vertex weights.
			if (val.vertex_weights()) {
				for (int i = 0; i < val.n(); ++i) {
					if (i > 0)
						out << " ";
					out << (*val.vertex_weights())[i];
				}
				out << '\n';
			}

			// Prints edges.
			for (int i = 0; i < val.m(); ++i) {
				auto [u, v] = val.edges()[i];
				out << (u + val.add_1_) << " " << (v + val.add_1_);

				// Edge weight.
				if (val.edge_weights().has_value())
					out << " " << (*val.edge_weights())[i];

				out << '\n';
			}

			return out;
		}

		// Gets a std::tuple<n, m, adj> representing the value.
		std::tuple<int, int, std::vector<std::set<int>>> to_std() const {
			ensure_adj_built();
			return std_type(n_, m(), adj_);
		}

	  private:
		// Rebuilds adjacency from edges_ after replacing the edge list (e.g.
		// subgraph operations).
		// O(m log n).
		void rebuild_adj_from_edge_list() {
			adj_.assign(n_, {});
			for (auto [u, v] : edges_) {
				adj_[u].insert(v);
				if (!is_directed_)
					adj_[v].insert(u);
			}
			adj_built_ = true;
		}

		// Builds adj_ from edges_ on first use.
		// O(1) if already built; O(m log n) otherwise.
		void ensure_adj_built() const {
			if (adj_built_)
				return;
			const_cast<value *>(this)->rebuild_adj_from_edge_list();
		}
	};

	// Adds all edges from `rhs` as preset edges.
	// O(rhs.m * log m).
	wgraph &add_edges_from(const value &rhs) {
		tgen_ensure(is_directed_ == rhs.is_directed(),
					"wgraph: graphs must have the same is_directed value");

		for (auto [u, v] : rhs.edges())
			add_edge(u, v);
		return *this;
	}

	// Generates graph value.
	// Optimized for performance: dense no-preset graphs use index sampling;
	// otherwise gen_remaining_edges.
	// O(n + m log n).
	value gen() const {
		detail::tgen_ensure_against_bug(static_cast<int>(edges_.size()) <= m_,
										"wgraph: too many edges were added");

		// All edges already added.
		if (static_cast<int>(edges_.size()) == m_)
			return value(n_, edges_, is_directed_);

		// Splits into two cases to optimize performance.

		// No presets and m > max_edges / 2: sample m distinct edge indices.
		if (auto indexed = try_gen_by_edge_index())
			return *indexed;

		// Otherwise: fill preset edges up to m_ with uniform random edges.
		return gen_remaining_edges(
			std::vector<std::pair<int, int>>(edges_.begin(), edges_.end()));
	}

	// Gets a (not uniformly) random connected undirected graph.
	// 1. Preset edges induce a spanning forest on their components.
	// 2. Then, uniformly random edges between components are added.
	// 3. Remaining edges are added uniformly at random.
	// O(n + m log n).
	value get_connected() const {
		tgen_ensure(!is_directed_,
					"wgraph: get_connected is only for undirected graphs");
		tgen_ensure(m_ >= n_ - 1,
					"wgraph: connected graph needs at least n - 1 edges");

		std::vector<std::pair<int, int>> edges;
		edges.reserve(m_);

		if (edges_.empty()) {
			if (n_ > 1) {
				std::vector<int> prufer(n_ - 2);
				for (int i = 0; i < n_ - 2; ++i)
					prufer[i] = next<int>(0, n_ - 1);
				for (auto [u, v] : detail::edges_from_prufer(std::move(prufer)))
					edges.emplace_back(u, v);
			}
		} else {
			edges.assign(edges_.begin(), edges_.end());

			std::vector<std::vector<int>> adj(n_);
			for (auto [u, v] : edges_) {
				adj[u].push_back(v);
				adj[v].push_back(u);
			}

			std::vector<int> comp_size;
			std::vector<std::vector<int>> component_ids;
			std::vector<bool> vis(n_, false);
			std::queue<int> q;

			for (int i = 0; i < n_; ++i) {
				if (vis[i])
					continue;

				vis[i] = true;
				q.push(i);
				comp_size.push_back(0);
				component_ids.emplace_back();
				while (q.size()) {
					int u = q.front();
					q.pop();
					++comp_size.back();
					component_ids.back().push_back(u);
					for (int v : adj[u]) {
						if (!vis[v]) {
							vis[v] = true;
							q.push(v);
						}
					}
				}
			}

			if (component_ids.size() > 1) {
				std::vector<int> prufer_values =
					many_by_distribution(component_ids.size() - 2, comp_size);
				for (auto [u, v] :
					 detail::edges_from_prufer(std::move(prufer_values)))
					edges.emplace_back(pick(component_ids[u]),
									   pick(component_ids[v]));
			}
		}

		return gen_remaining_edges(std::move(edges));
	}

	// Gets a (not uniformly) random directed acyclic graph.
	// 1. Randomized Kahn (uniform choice among indegree-0 vertices) yields a
	//    random topological order of the preset edges (which must be acyclic).
	// 2. Extra edges are sampled randomly using the order.
	// With no preset edges: sample a random graph then orient acyclically.
	// Optimized for performance (distinct upper-triangle edge-index sampling;
	// rejection instead of pair::distinct for preset edges).
	// O(n + m log n).
	value get_acyclic() const {
		tgen_ensure(is_directed_,
					"wgraph: get_acyclic is only for directed graphs");

		if (edges_.empty()) {
			std::vector<int> order(n_);
			std::iota(order.begin(), order.end(), 0);
			for (int i = n_ - 1; i > 0; --i)
				std::swap(order[i], order[next(0, i)]);

			const long long max_pairs =
				static_cast<long long>(n_) * (n_ - 1) / 2;
			tgen_ensure(m_ <= max_pairs,
						"wgraph: not enough edges to generate");

			std::vector<std::pair<int, int>> edges;
			edges.reserve(m_);
			for (long long idx : distinct_range<long long>(0, max_pairs - 1)
									 .gen_list(m_)
									 .to_std()) {
				auto [i, j] = detail::decode_undirected_simple_edge(n_, idx);
				edges.emplace_back(order[i], order[j]);
			}
			return value(n_, edges, true);
		}

		std::vector<std::vector<int>> adj(n_);
		std::vector<int> indeg(n_, 0);
		for (auto [u, v] : edges_) {
			adj[u].push_back(v);
			++indeg[v];
		}

		std::vector<int> available;
		for (int i = 0; i < n_; ++i)
			if (indeg[i] == 0)
				available.push_back(i);

		// Random topological order using randomized Kahn's algorithm.
		std::vector<int> order;
		while (!available.empty()) {
			int idx = next(0, static_cast<int>(available.size()) - 1);
			int u = available[idx];
			std::swap(available[idx], available.back());
			available.pop_back();

			order.push_back(u);
			for (int v : adj[u])
				if (--indeg[v] == 0)
					available.push_back(v);
		}

		tgen_ensure(static_cast<int>(order.size()) == n_,
					"wgraph: preset edges contain a directed cycle");

		value acyclic(n_, edges_, true);

		// Generates final edges.

		detail::tgen_ensure_against_bug(acyclic.m() <= m_,
										"wgraph: too many edges were added");

		if (acyclic.m() < m_) {
			std::vector<int> order_pos(n_);
			for (int i = 0; i < n_; ++i)
				order_pos[order[i]] = i;

			std::unordered_set<uint64_t> seen;
			seen.reserve(m_ * 2);
			for (auto [u, v] : acyclic.edges())
				seen.insert(
					detail::undirected_edge_key(order_pos[u], order_pos[v]));

			const long long max_pairs =
				static_cast<long long>(n_) * (n_ - 1) / 2;
			while (acyclic.m() < m_) {
				std::pair<int, int> edge;
				if (!detail::try_generate_distinct(seen, [&] {
						long long idx = next<long long>(0, max_pairs - 1);
						edge = detail::decode_undirected_simple_edge(n_, idx);
						return detail::undirected_edge_key(edge.first,
														   edge.second);
					}))
					throw detail::error("wgraph: not enough edges to generate");
				acyclic.add_edge(order[edge.first], order[edge.second]);
			}
		}

		return acyclic;
	}

	// Generates a (not uniformly) random skewed connected graph.
	// 1. Builds the same skewed labeled tree as wtree::gen_skewed(n,
	//    elongation)(root 0, parent(i) = wnext(i, elongation) for i >= 1).
	//    If is_directed, tree edges are oriented down the tree.
	// 2. Adds the remaining edges: pick an endpoint u uniformly;
	//    pick k uniformly in [1, spread]; walk from u toward the root k
	//    times along tree parents to get v; add edge (v, u).
	// If elongation is small, generates a graph with small diameter.
	// If elongation is large, generates a graph with large diameter, with
	// vertices 0 and n-1 being far apart.
	// O(n + m log n) if spread is O(1);
	// O(n log n + m log^2 n) otherwise.
	static value gen_skewed(int n, int m, int elongation, int spread,
							bool is_directed = false) {
		tgen_ensure(
			m >= n - 1,
			"wgraph: skewed graph needs at least n - 1 edges to be connected");
		tgen_ensure(spread >= 2,
					"wgraph: gen_skewed spread must be at least 2");

		value skewed(n, {}, is_directed);

		std::vector<int> parent(n), depth(n, 0);
		parent[0] = 0;
		for (int i = 1; i < n; ++i) {
			int p = wnext<int>(i, elongation);
			parent[i] = p;
			depth[i] = depth[p] + 1;
			skewed.add_edge(p, i);
		}

		const int extra = m - (n - 1);
		if (extra == 0)
			return skewed;

		// If spread is large, use binary lifting to find the ancestor.
		// Otherwise, enumerate O(n * spread) ancestor edges and sample
		// directly.
		constexpr int naive_ancestor_spread = 20;

		if (spread <= naive_ancestor_spread) {
			std::vector<std::pair<int, int>> candidates;
			candidates.reserve(n * spread);
			for (int u = 0; u < n; ++u) {
				int max_k = std::min(spread, depth[u]);
				if (max_k < 2)
					continue;
				int v = parent[u];
				for (int k = 2; k <= max_k; ++k) {
					v = parent[v];
					candidates.emplace_back(v, u);
				}
			}

			tgen_ensure(extra <= static_cast<int>(candidates.size()),
						"wgraph: not enough edges to generate");

			for (auto [v, u] : choose(candidates, extra))
				skewed.add_edge(v, u);
		} else {
			// Binary lifting.
			int lg = 1;
			while ((1 << lg) <= n)
				++lg;

			std::vector<std::vector<int>> up(lg, std::vector<int>(n));
			for (int v = 0; v < n; ++v)
				up[0][v] = parent[v];
			for (int j = 1; j < lg; ++j)
				for (int v = 0; v < n; ++v)
					up[j][v] = up[j - 1][up[j - 1][v]];

			// Creates uniform generator of edges (u, v) such that v is ancestor
			// of u. For that, every u has depth[u]-1 choices for v, so we
			// weight u by min(spread - 1, depth[u] - 1). After that we can
			// just pick the ancestor uniformly.
			std::vector<int> distribution = depth;
			for (int &d : distribution)
				d = std::max(0, std::min(spread - 1, d - 1));
			weighted_sampler vertex_choice(distribution);
			distinct extra_edges([&]() -> std::pair<int, int> {
				int u = vertex_choice.next();
				int k = next(2, spread);
				int v = u;
				for (int j = 0; j < lg; ++j)
					if (k >> j & 1)
						v = up[j][v];
				return {v, u};
			});

			while (skewed.m() < m) {
				std::pair<int, int> edge;
				try {
					edge = extra_edges.gen();
				} catch (const std::runtime_error &e) {
					if (std::string(e.what()) ==
						"tgen: distinct: no more distinct values")
						throw detail::error(
							"wgraph: not enough edges to generate");
					throw e;
				}

				skewed.add_edge(edge.first, edge.second);
			}
		}

		return skewed;
	}

	// Generates a random bipartite graph. The first side has vertices
	// 0 .. n1-1, the second n1 .. n1+n2-1.
	// Uniform when connected is false (distinct cross-edge indices).
	// When connected, bipartite Prüfer + rejection fill; not uniform over
	// connected bipartite graphs.
	// O(n1 + n2 + m log(n1 * n2)) expected.
	static value gen_bipartite(int n1, int n2, int m, bool connected = false) {
		tgen_ensure(m >= 0, "wgraph: number of edges must be nonnegative");
		long long num_edges = 1LL * n1 * n2;
		tgen_ensure(m <= num_edges,
					"wgraph: bipartite graph has at most n1 * n2 edges");
		if (connected)
			tgen_ensure(
				m >= n1 + n2 - 1,
				"wgraph: connected bipartite graph needs at least n1 + n2 - 1 "
				"edges");

		if (!connected) {
			std::vector<std::pair<int, int>> edges;
			edges.reserve(m);
			for (long long idx : distinct_range<long long>(0, num_edges - 1)
									 .gen_list(m)
									 .to_std())
				edges.emplace_back(static_cast<int>(idx / n2),
								   n1 + static_cast<int>(idx % n2));
			return value(n1 + n2, std::move(edges), false);
		}

		std::unordered_set<uint64_t> used_edges;
		used_edges.reserve(m * 2);
		std::vector<std::pair<int, int>> edges;
		edges.reserve(m);

		auto pack_edge = [](int u, int v) -> uint64_t {
			if (u > v)
				std::swap(u, v);
			return (static_cast<uint64_t>(u) << 32) | static_cast<uint32_t>(v);
		};

		if (n1 > 0 and n2 > 0) {
			std::vector<int> prufer(n1 + n2 - 2);
			for (int i = 0; i < n2 - 1; ++i)
				prufer[i] = next(0, n1 - 1);
			for (int i = 0; i < n1 - 1; ++i)
				prufer[n2 - 1 + i] = next(n1, n1 + n2 - 1);
			shuffle(prufer.begin(), prufer.end());
			for (auto [u, v] : detail::edges_from_prufer(std::move(prufer))) {
				if (u > v)
					std::swap(u, v);
				if (used_edges.insert(pack_edge(u, v)).second)
					edges.emplace_back(u, v);
			}
			detail::tgen_ensure_against_bug(
				used_edges.size() == size_t(n1 + n2 - 1),
				"wgraph: invalid bipartite spanning tree size");
		}

		while (edges.size() < size_t(m)) {
			int u = next(0, n1 - 1);
			int v = next(n1, n1 + n2 - 1);
			if (used_edges.insert(pack_edge(u, v)).second)
				edges.emplace_back(u, v);
		}

		return value(n1 + n2, std::move(edges), false);
	}

  private:
	// If this generator has no preset edges and m is large relative to the
	// maximum edge count, sample by distinct edge index. Otherwise
	// std::nullopt.
	// Optimized for performance (index sampling instead of rejection).
	// O(m log n).
	std::optional<value> try_gen_by_edge_index() const {
		if (!edges_.empty())
			return std::nullopt;

		long long max_edges =
			detail::max_graph_edges(n_, is_directed_, has_self_loops_);
		if (m_ > max_edges)
			throw detail::error("wgraph: not enough edges to generate");
		if (max_edges <= 0 or 2LL * m_ <= max_edges)
			return std::nullopt;

		std::vector<std::pair<int, int>> edges;
		edges.reserve(m_);
		for (long long idx :
			 distinct_range<long long>(0, max_edges - 1).gen_list(m_).to_std())
			edges.push_back(detail::decode_graph_edge_index(
				n_, idx, is_directed_, has_self_loops_));

		return value(n_, edges, is_directed_);
	}

	// Fills `edges` up to m_ with uniform random edges not already present.
	// Optimized for performance (uint64 edge keys + try_generate_distinct).
	// O(m log n).
	value gen_remaining_edges(std::vector<std::pair<int, int>> edges) const {
		detail::tgen_ensure_against_bug(static_cast<int>(edges.size()) <= m_,
										"wgraph: too many edges were added");

		if (static_cast<int>(edges.size()) == m_)
			return value(n_, edges, is_directed_);

		edges.reserve(m_);

		std::unordered_set<uint64_t> seen;
		seen.reserve(m_ * 2);
		for (auto [u, v] : edges) {
			if (!is_directed_ and u > v)
				std::swap(u, v);
			seen.insert(is_directed_ ? detail::directed_edge_key(u, v)
									 : detail::undirected_edge_key(u, v));
		}

		while (static_cast<int>(edges.size()) < m_) {
			std::pair<int, int> edge;
			if (!detail::try_generate_distinct(seen, [&] {
					edge = detail::get_random_graph_edge(n_, is_directed_,
														 has_self_loops_);
					if (!is_directed_ and edge.first > edge.second)
						std::swap(edge.first, edge.second);
					return is_directed_ ? detail::directed_edge_key(edge.first,
																	edge.second)
										: detail::undirected_edge_key(
											  edge.first, edge.second);
				}))
				throw detail::error("wgraph: not enough edges to generate");
			edges.emplace_back(edge);
		}

		return value(n_, edges, is_directed_);
	}
};

// Implementation of wtree::value constructor from wgraph.
// O(n + m alpha(n)).
template <typename VWeight, typename EWeight>
wtree<VWeight, EWeight>::value::value(
	const typename wgraph<VWeight, EWeight>::value &g)
	: n_(g.n()), adj_(g.n()), add_1_(false), print_n_(false), dsu_(g.n()) {
	tgen_ensure(g.n() > 0, "wtree: value: graph must have at least one vertex");
	tgen_ensure(!g.is_directed(),
				"wtree: value: graph must be undirected to form a tree");

	if (g.vertex_weights().has_value())
		vertex_weights_ = *g.vertex_weights();
	if (g.edge_weights().has_value())
		edge_weights_ = std::vector<EWeight>();

	if (n_ == 1)
		return;

	std::vector<int> order(g.m());
	std::iota(order.begin(), order.end(), 0);
	tgen::shuffle(order.begin(), order.end());

	std::vector<std::pair<int, int>> tree_edges;
	tree_edges.reserve(n_ - 1);

	for (int i : order) {
		auto [u, v] = g.edges()[i];
		if (!dsu_.unite(u, v))
			continue;
		if (u > v)
			std::swap(u, v);

		tree_edges.emplace_back(u, v);
		adj_[u].insert(v);
		adj_[v].insert(u);
		if (edge_weights_.has_value())
			edge_weights_->push_back((*g.edge_weights())[i]);
		if (static_cast<int>(tree_edges.size()) == n_ - 1)
			break;
	}

	tgen_ensure(static_cast<int>(tree_edges.size()) == n_ - 1,
				"wtree: value: graph must be connected to form a tree");

	edges_ = std::move(tree_edges);
}

/*
 * Other types of weighted-ness.
 */

// Vertex weighted graph.
template <typename VWeight> using vgraph = wgraph<VWeight, int>;

// Edge weighted graph.
template <typename EWeight> using egraph = wgraph<int, EWeight>;

// Unweighted graph.
using graph = wgraph<int, int>;

/*
 * Standard graphs.
 */

// Complete.
// O(n^2).
inline graph::value K(int n) { return graph(n, n * (n - 1) / 2).gen(); }

// Path.
// Path with `n` vertices. The edges of the path are 0 and n-1.
// If directed, edges are i -> i+1 for i in [0, n-2).
// O(n).
inline graph::value P(int n, bool is_directed = false) {
	graph g(n, n - 1, is_directed);
	for (int i = 0; i + 1 < n; ++i)
		g.add_edge(i, i + 1);
	return g.gen();
}

// Cycle.
// n >= 3.
// If directed, edges are i -> (i+1) % n.
// O(n).
inline graph::value C(int n, bool is_directed = false) {
	tgen_ensure(n >= 3, "graph: cycle size must be at least 3");

	graph g(n, n, is_directed);
	for (int i = 0; i < n; ++i)
		g.add_edge(i, (i + 1) % n);
	return g.gen();
}

// Complete bipartite.
// The first side has vertices `0` to `n1-1`, the second side has vertices `n1`
// to `n1+n2-1`.
// O(n1 * n2).
inline graph::value K(int n1, int n2) {
	graph g(n1 + n2, static_cast<long long>(n1) * n2);
	for (int i = 0; i < n1; ++i)
		for (int j = 0; j < n2; ++j)
			g.add_edge(i, n1 + j);
	return g.gen();
}

// Star.
// The center is vertex 0.
// O(n).
inline graph::value S(int n) { return K(1, n - 1); }

/****************
 *              *
 *   GEOMETRY   *
 *              *
 ****************/

namespace geometry {

using i128 = ::tgen::detail::i128;

// Point on the plane with coordinates of type T.
template <typename T> struct point {
	static_assert(std::is_arithmetic_v<T>,
				  "point requires an arithmetic coordinate type");

	// Dot/cross product type: __int128 for T = long long, long long for other
	// integral T, T for floating-point.
	using product_t = std::conditional_t<
		std::is_same_v<T, long long>, i128,
		std::conditional_t<std::is_integral_v<T>, long long, T>>;

	// x and y coordinates.
	T x_, y_;

	// Constructs a point with coordinates x and y.
	point(T x = 0, T y = 0) : x_(x), y_(y) {}

	// Returns the x coordinate.
	T x() const { return x_; }

	// Returns the y coordinate.
	T y() const { return y_; }

	// Equality of coordinates, with epsilon-based equality for floating-point
	// coordinates (tolerance 1e-9).
	static bool coord_eq(T a, T b) {
		if constexpr (std::is_integral_v<T>)
			return a == b;
		constexpr T eps = T(1e-9);
		T d = a - b;
		return d >= -eps and d <= eps;
	}

	// Lexicographic order (by x, then y).
	bool operator<(const point &p) const {
		if (!coord_eq(x_, p.x()))
			return x_ < p.x();
		return y_ < p.y();
	}

	// Equality of coordinates.
	bool operator==(const point &p) const {
		return coord_eq(x_, p.x()) and coord_eq(y_, p.y());
	}

	// Vector addition.
	point operator+(const point &p) const {
		return point(x_ + p.x(), y_ + p.y());
	}

	// Vector subtraction.
	point operator-(const point &p) const {
		return point(x_ - p.x(), y_ - p.y());
	}

	// Scalar multiplication.
	point operator*(T c) const { return point(x_ * c, y_ * c); }

	// Dot product.
	product_t operator*(const point &p) const {
		if constexpr (std::is_floating_point_v<T>)
			return x_ * p.x() + y_ * p.y();
		return product_t(x_) * p.x() + product_t(y_) * p.y();
	}

	// Cross product (signed area of the parallelogram).
	product_t operator^(const point &p) const {
		if constexpr (std::is_floating_point_v<T>)
			return x_ * p.y() - y_ * p.x();
		return product_t(x_) * p.y() - product_t(y_) * p.x();
	}

	// Prints the point as "x y".
	friend std::ostream &operator<<(std::ostream &out, const point &p) {
		return out << p.x() << ' ' << p.y();
	}
};

// Generates n distinct integer points in [min_coord, max_coord]^2 with no three
// collinear.
// O(n).
inline std::vector<point<long long>>
random_points_general_position(int n, long long min_coord,
							   long long max_coord) {
	tgen_ensure(n > 0,
				"geometry: random_points_general_position: n must be positive");
	tgen_ensure(max_coord >= min_coord,
				"geometry: random_points_general_position: min_coord must be "
				"at most max_coord");
	tgen_ensure(
		static_cast<i128>(max_coord) - min_coord <=
			std::numeric_limits<long long>::max(),
		"geometry: random_points_general_position: coordinate range too large");
	uint64_t width = max_coord - min_coord;
	uint64_t p = math::prime_from(2 * n);

	// Requires width >= p - 1 because sheared coordinates lie in [0, p - 1].
	tgen_ensure(width >= p - 1,
				"geometry: random_points_general_position: coordinate range "
				"too small for n");

	// Base set {(x, x^-1 mod p) : x \in {1, ..., p - 1}}.
	//
	// Over F_p, y = x^-1 is a rational map on nonzero x, so any line meets the
	// graph in at most two points. Distinct x therefore give distinct points;
	// no three of them can be collinear in the integer plane.
	std::vector<uint64_t> x_range(p - 1);
	std::iota(x_range.begin(), x_range.end(), 1);
	shuffle(x_range.begin(), x_range.end());
	std::vector<i128> bx(n), by(n);
	for (int i = 0; i < n; ++i) {
		uint64_t x = x_range[i];
		bx[i] = x;
		by[i] = math::modular_inverse(x, p);
	}

	// Applies a random element of SL(2, p) by composing several elementary
	// shear matrices over F_p.
	//
	// Each step applies either
	//     [1 r]       or       [1 0]
	//     [0 1]                [r 1]
	//
	// with r \in {-2, -1, 1, 2}, and all arithmetic performed modulo p.
	// After all iterations, the resulting transformation is
	// A = S_k S_{k-1} ... S_1
	//
	// where every S_i has determinant 1. Therefore det(A) = 1 (mod p),
	// so A is invertible.
	//
	// Invertible affine transformations of F_p^2 preserve collinearity and
	// non-collinearity. Since the base set {(x, x^-1 mod p)} contains no
	// three collinear points, the transformed set also contains no three
	// collinear points.
	const int num_shears = 8;
	std::vector<i128> lin_x = bx, lin_y = by;

	for (int it = 0; it < num_shears; ++it) {
		bool vertical_shear = next(2) == 0;
		int shear_r = pick({-2, -1, 1, 2});

		for (int i = 0; i < n; ++i) {
			if (vertical_shear)
				lin_x[i] = (lin_x[i] + shear_r * lin_y[i]) % p;
			else
				lin_y[i] = (lin_y[i] + shear_r * lin_x[i]) % p;

			if (lin_x[i] < 0)
				lin_x[i] += p;
			if (lin_y[i] < 0)
				lin_y[i] += p;
		}
	}

	i128 min_x = lin_x[0], max_x = lin_x[0], min_y = lin_y[0], max_y = lin_y[0];
	for (int i = 1; i < n; ++i) {
		min_x = std::min(min_x, lin_x[i]);
		max_x = std::max(max_x, lin_x[i]);
		min_y = std::min(min_y, lin_y[i]);
		max_y = std::max(max_y, lin_y[i]);
	}

	long long x_shift =
		min_coord - min_x + next<long long>(0, width - (max_x - min_x));
	long long y_shift =
		min_coord - min_y + next<long long>(0, width - (max_y - min_y));

	std::vector<point<long long>> pts;
	for (int i = 0; i < n; ++i)
		pts.emplace_back(lin_x[i] + x_shift, lin_y[i] + y_shift);
	return pts;
}

namespace detail {

// Signed area of triangle (a, b, p); positive iff (a, b, p) are in
// counterclockwise order. 0 iff (a, b, p) are collinear. O(1).
inline i128 ccw(const point<long long> &a, const point<long long> &b,
				const point<long long> &p) {
	return (static_cast<i128>(b.x()) - a.x()) *
			   (static_cast<i128>(p.y()) - a.y()) -
		   (static_cast<i128>(b.y()) - a.y()) *
			   (static_cast<i128>(p.x()) - a.x());
}

// Integer projection of P onto line AB (A and B need not be distinct).
inline i128 proj_on_ab(const point<long long> &P, const point<long long> &A,
					   const point<long long> &B) {
	return (P - A) * (B - A);
}

// In-place Hamiltonian path on points[left..right-1] with points[left]
// start and points[right-1] end.
// O(n log n) expected if points are "random", O(n^2) worst case.
inline void conquer(std::vector<point<long long>> &points, int left,
					int right) {
	if (right - left <= 3)
		return;

	point<long long> A = points[left], B = points[right - 1];

	// If all points are collinear, sort them properly and return.
	bool all_collinear = true;
	for (int k = left + 1; k < right - 1; ++k) {
		if (ccw(A, B, points[k]) != 0) {
			all_collinear = false;
			break;
		}
	}
	if (all_collinear) {
		std::sort(points.begin() + left, points.begin() + right,
				  [&](const point<long long> &P, const point<long long> &Q) {
					  return proj_on_ab(P, A, B) < proj_on_ab(Q, A, B);
				  });
		return;
	}

	// Choses a pivot that is not collinear with A and B.
	std::vector<int> candidates;
	for (int k = left + 1; k < right - 1; ++k) {
		if (ccw(A, B, points[k]) != 0)
			candidates.push_back(k);
	}
	int ci = candidates[next(0, static_cast<int>(candidates.size()) - 1)];
	point<long long> C = points[ci];

	uint64_t wa = next<uint64_t>(1, std::numeric_limits<uint64_t>::max());
	uint64_t wb = next<uint64_t>(1, std::numeric_limits<uint64_t>::max());
	bool a_on_positive = ccw(C, A, B) < 0;

	// Classify interior points into two sides of the wedge A-C-B for partition.
	// Collinear points on AB are tie-broken along the segment.
	i128 proj_sum = proj_on_ab(A, A, B) + proj_on_ab(B, A, B);
	auto is_positive = [&](const point<long long> &P) -> bool {
		i128 s = wa * ccw(C, A, P) + wb * ccw(C, B, P);
		// Weighted wedge side of P w.r.t. C, A, B.
		if (s != 0)
			return s > 0;
		// P is on line AB: split by projection past the midpoint.
		return 2 * proj_on_ab(P, A, B) > proj_sum;
	};

	// Holds C at points[right-2] while classifying interior points in
	// [left+1, right-3].
	if (ci != right - 2)
		std::swap(points[ci], points[right - 2]);

	int i = left + 1;
	int j = right - 3;
	while (i < j) {
		if (is_positive(points[i]) == a_on_positive)
			++i;
		else if (is_positive(points[j]) != a_on_positive)
			--j;
		else {
			std::swap(points[i], points[j]);
			++i;
			--j;
		}
	}

	// After partition:
	// points[left]=A | (A,C)... | C | (C,B)... | points[right-1]=B.

	// After the swap, p is the index of C (pivot between the two subpaths).
	int p = i;
	if (i == j and is_positive(points[i]) == a_on_positive)
		++p;
	std::swap(points[p], points[right - 2]);

	// Path A -> C.
	conquer(points, left, p + 1);
	// Path C -> B.
	conquer(points, p, right);
}

// Samples k sorted distinct integers from [left, right] uniformly.
// Optimized for performance (pool partial Fisher–Yates or complement path for
// modest ranges; sparse-map fallback otherwise).
// O(k log k); O(right - left) memory when the range is modest.
inline std::vector<long long>
sample_sorted_distinct_in_range(int k, long long left, long long right) {
	long long universe = right - left + 1;
	std::vector<long long> res;
	res.reserve(k);
	if (k == 0)
		return res;

	constexpr long long pool_threshold = 8'000'000;
	constexpr long long pool_always_below = 500'000;

	if (universe <= pool_threshold and
		(universe <= pool_always_below or k >= universe / 4)) {
		size_t u = universe;
		size_t ks = k;
		std::vector<long long> pool(u);
		std::iota(pool.begin(), pool.end(), left);
		size_t m = ks <= u / 2 ? ks : u - ks;
		for (size_t i = 0; i < m; ++i) {
			size_t j = next<size_t>(i, u - 1);
			std::swap(pool[i], pool[j]);
		}
		if (ks <= u / 2) {
			res.assign(pool.begin(), pool.begin() + ks);
			std::sort(res.begin(), res.end());
		} else {
			std::vector<char> excluded(u, 0);
			for (size_t i = 0; i < m; ++i)
				excluded[pool[i] - left] = 1;
			for (long long v = left; v <= right; ++v)
				if (!excluded[v - left])
					res.push_back(v);
		}
	} else {
		std::unordered_map<long long, long long> virtual_list;
		virtual_list.reserve(k * 2);
		for (long long i = 0; i < k; ++i) {
			long long j = next<long long>(i, universe - 1);
			long long vi = virtual_list.count(i) ? virtual_list[i] : i;
			long long vj = virtual_list.count(j) ? virtual_list[j] : j;
			virtual_list[j] = vi;
			virtual_list[i] = vj;
			res.push_back(virtual_list[i] + left);
		}
		std::sort(res.begin(), res.end());
	}
	return res;
}

// Generates n >= 3 sorted distinct coordinates in [0, width] with endpoints 0
// and width.
// Optimized for performance via sample_sorted_distinct_in_range.
// O(n log n).
inline std::vector<long long>
generate_sorted_coords_with_endpoints(int n, long long width) {
	std::vector<long long> coords;
	coords.reserve(n);
	coords.push_back(0);
	std::vector<long long> inner =
		sample_sorted_distinct_in_range(n - 2, 1, width - 1);
	coords.insert(coords.end(), inner.begin(), inner.end());
	coords.push_back(width);
	return coords;
}

// Valtr-style signed edge components along one axis from n sorted distinct
// coordinates. The n differences sum to zero.
inline std::vector<long long>
valtr_edge_components(const std::vector<long long> &sorted_coords) {
	int n = sorted_coords.size();
	std::vector<long long> left, right;
	left.reserve(n / 2);
	right.reserve(n / 2);
	for (int i = 1; i + 1 < n; ++i) {
		if (next(2) == 0)
			left.push_back(sorted_coords[i]);
		else
			right.push_back(sorted_coords[i]);
	}
	long long lo = sorted_coords.front(), hi = sorted_coords.back();
	std::vector<long long> seq;
	seq.reserve(n + 1);
	seq.push_back(lo);
	for (long long v : left)
		seq.push_back(v);
	seq.push_back(hi);
	for (auto it = right.rbegin(); it != right.rend(); ++it)
		seq.push_back(*it);
	seq.push_back(lo);
	std::vector<long long> comps(n);
	for (int i = 0; i < n; ++i)
		comps[i] = seq[i + 1] - seq[i];
	return comps;
}

} // namespace detail

// Generates n vertices of a convex integer polygon inside a box.
// If strict is true, boundary vertices are guaranteed non-collinear when
// generation succeeds; retry count depends on n and width.
// O(n log n).
inline std::vector<point<long long>>
random_convex_polygon(int n, long long min_coord, long long max_coord,
					  bool strict = false) {
	tgen_ensure(n >= 3,
				"geometry: random_convex_polygon: n must be at least 3");
	tgen_ensure(max_coord >= min_coord,
				"geometry: random_convex_polygon: min_coord must be at most "
				"max_coord");
	tgen_ensure(static_cast<i128>(max_coord) - min_coord <=
					std::numeric_limits<long long>::max(),
				"geometry: random_convex_polygon: coordinate range too large");
	long long width = max_coord - min_coord;
	tgen_ensure(
		width >= n - 1,
		"geometry: random_convex_polygon: coordinate range too small for n");

	// Grid spans [0, width]; after the walk, the bounding box span equals
	// width on each axis, so shifting by (min_coord - min_x, min_coord - min_y)
	// fills the box.
	std::vector<long long> x_sorted =
		detail::generate_sorted_coords_with_endpoints(n, width);
	std::vector<long long> y_sorted =
		detail::generate_sorted_coords_with_endpoints(n, width);

	std::vector<long long> x_comp = detail::valtr_edge_components(x_sorted);
	std::vector<long long> y_comp = detail::valtr_edge_components(y_sorted);

	std::vector<point<long long>> edges(n);
	auto upper = [](const point<long long> &p) {
		return p.y() > 0 or (p.y() == 0 and p.x() > 0);
	};

	// When strict, retry pairings that yield consecutive parallel edges.
	int max_strict_attempts = 1;
	if (strict) {
		if (width < 2 * n - 1 or n > 1000)
			max_strict_attempts = 4;
		else if (n <= 50)
			max_strict_attempts = 12;
		else
			max_strict_attempts = 8;
	}
	for (int attempt = 0;; ++attempt) {
		shuffle(y_comp.begin(), y_comp.end());
		for (int i = 0; i < n; ++i)
			edges[i] = point<long long>(x_comp[i], y_comp[i]);
		std::sort(
			edges.begin(), edges.end(),
			[&upper](const point<long long> &a, const point<long long> &b) {
				bool au = upper(a), bu = upper(b);
				if (au != bu)
					return au;
				auto cross = a ^ b;
				if (cross != 0)
					return cross > 0;
				return (a * a) < (b * b);
			});
		if (!strict)
			break;
		bool collinear = false;
		for (int i = 0; i < n; ++i) {
			const auto &cur = edges[i];
			const auto &nxt = edges[(i + 1) % n];
			if ((cur ^ nxt) == 0) {
				collinear = true;
				break;
			}
		}
		if (!collinear)
			break;

		tgen_ensure(attempt + 1 < max_strict_attempts,
					"geometry: random_convex_polygon: generation failed: "
					"coordinate range too small for n");
	}

	i128 cur_x = 0, cur_y = 0;
	std::vector<i128> px(n), py(n);
	for (int i = 0; i < n; ++i) {
		px[i] = cur_x;
		py[i] = cur_y;
		cur_x += edges[i].x();
		cur_y += edges[i].y();
	}
	tgen::detail::tgen_ensure_against_bug(
		cur_x == 0 and cur_y == 0, "geometry: random_convex_polygon: walk did "
								   "not close");

	i128 min_x = px[0], min_y = py[0];
	for (int i = 1; i < n; ++i) {
		min_x = std::min(min_x, px[i]);
		min_y = std::min(min_y, py[i]);
	}

	// Translates the polygon so the bounding box is [min_coord, min_coord].
	i128 shift_x = min_coord - min_x;
	i128 shift_y = min_coord - min_y;

	std::vector<point<long long>> pts;
	pts.reserve(n);
	for (int i = 0; i < n; ++i)
		pts.emplace_back(px[i] + shift_x, py[i] + shift_y);

	// Randomly rotates the polygon.
	int rot = next(n);
	if (rot > 0)
		std::rotate(pts.begin(), pts.begin() + rot, pts.end());
	if (next(2) == 0)
		std::reverse(pts.begin(), pts.end());
	return pts;
}

// Random simple polygon through given distinct points.
// Collinear triples are allowed; fails if all points are collinear.
// O(n log n) expected if points are "random", O(n^2) worst case.
inline std::vector<point<long long>> random_simple_polygon_through_points(
	const std::vector<point<long long>> &points) {
	int n = points.size();
	tgen_ensure(n >= 3,
				"geometry: random_simple_polygon_through_points: need at "
				"least 3 points");
	tgen_ensure(
		static_cast<int>(
			std::set<point<long long>>(points.begin(), points.end()).size()) ==
			n,
		"geometry: random_simple_polygon_through_points: points must "
		"be distinct");

	int idx_a = 0, idx_b = 0;
	for (int i = 1; i < n; ++i) {
		if (points[i] < points[idx_a])
			idx_a = i;
		if (points[idx_b] < points[i])
			idx_b = i;
	}
	point<long long> A = points[idx_a], B = points[idx_b];

	bool all_collinear = true;
	for (int i = 0; i < n; ++i) {
		if (i == idx_a or i == idx_b)
			continue;
		if (detail::ccw(A, B, points[i]) != 0) {
			all_collinear = false;
			break;
		}
	}
	tgen_ensure(!all_collinear,
				"geometry: random_simple_polygon_through_points: all points "
				"are collinear; no simple polygon exists");

	std::vector<point<long long>> chain;
	chain.push_back(A);
	int left_count = 0;
	for (int i = 0; i < n; ++i) {
		if (i == idx_a or i == idx_b)
			continue;
		if (detail::ccw(A, B, points[i]) > 0) {
			chain.push_back(points[i]);
			++left_count;
		}
	}
	chain.push_back(B);
	for (int i = 0; i < n; ++i) {
		if (i == idx_a or i == idx_b)
			continue;
		if (detail::ccw(A, B, points[i]) <= 0)
			chain.push_back(points[i]);
	}
	chain.push_back(A);

	int n1 = 2 + left_count;
	// Upper chain: A -> B.
	detail::conquer(chain, 0, n1);
	// Lower chain: B -> A.
	detail::conquer(chain, n1 - 1, chain.size());

	// Cyclic vertex order: chain[1..n1) then chain[n1..end) (skip each path's
	// start vertex).
	std::vector<point<long long>> poly;
	poly.insert(poly.end(), chain.begin() + 1, chain.begin() + n1);
	poly.insert(poly.end(), chain.begin() + n1, chain.end());
	return poly;
}

// Random simple polygon.
// O(n log n) expected.
inline std::vector<point<long long>>
random_simple_polygon(int n, long long min_coord, long long max_coord) {
	tgen_ensure(n >= 3,
				"geometry: random_simple_polygon: n must be at least 3");

	return random_simple_polygon_through_points(
		random_points_general_position(n, min_coord, max_coord));
}

} // namespace geometry

/************
 *          *
 *   HACK   *
 *          *
 ************/

namespace hack {

namespace detail {

using namespace tgen::detail;

// Computes polynomial hash of a string.
// O(|s|).
inline int hash_string(const std::string &s, int base, int mod) {
	long long h = 0;
	for (char c : s)
		h = (h * base + c - 'a' + 1) % mod;
	return h;
}

// Estimates the length of the string to very likely have a collision.
inline int estimate_length(int alphabet_size, int mod) {
	// Magic constants.
	double base_len = 2.5 * std::log(std::sqrt(mod));
	double scale = std::log(alphabet_size) / std::log(2.0);
	double adjusted = base_len / std::max(1.0, scale * 0.7);

	return static_cast<int>(std::ceil(adjusted));
}

// Collides two strings to have the same polynomial hash.
// O(sqrt(mod) log(mod)) with high probability.
inline std::pair<std::string, std::string>
birthday_attack(const std::vector<std::string> &alphabet, int base, int mod) {
	tgen_ensure(0 < base and base < mod,
				"birthday_attack: base must be in (0, mod)");
	std::map<uint64_t, std::vector<int>> seen;
	int length = estimate_length(alphabet.size(), mod);

	while (true) {
		std::vector<int> seq(length);

		std::string s;

		for (int i = 0; i < length; ++i) {
			seq[i] = next<int>(0, alphabet.size() - 1);
			s += alphabet[seq[i]];
		}

		int h = hash_string(s, base, mod);

		auto it = seen.find(h);
		if (it != seen.end() and it->second != seq) {
			std::string a, b;

			for (int x : it->second)
				a += alphabet[x];
			for (int x : seq)
				b += alphabet[x];

			if (a != b)
				return {a, b};
		}

		seen[h] = seq;
	}
}

// Tried to find correct multipliers for unordered_map/set to force
// collisions. O(1).
inline std::set<long long> std_hash_multipliers() {
	std::set<long long> multipliers = {85229};

	// Codeforces GCC GNU G++17 7.3.0 case.
	bool codeforces_gcc_case = true;
	if (cpp.version_ != 0 and cpp.version_ != 17)
		codeforces_gcc_case = false;
	if (compiler.kind_ != compiler_kind::unknown and
		compiler.kind_ != compiler_kind::gcc)
		codeforces_gcc_case = false;
	if (compiler.major_ > 7)
		codeforces_gcc_case = false;

	if (codeforces_gcc_case)
		multipliers.insert(107897);

	return multipliers;
}

} // namespace detail

// Fetches prefix of length n of the string "abacabadabacabae...".
// O(n).
inline std::string abacaba(int n) {
	tgen_ensure(n > 0, "str: size must be positive");
	std::string str = "a";
	char c = 'a';
	while (static_cast<int>(str.size()) < n) {
		int prev_size = str.size();
		str += ++c;
		for (int j = 0; j < prev_size and static_cast<int>(str.size()) < n; ++j)
			str += str[j];
	}
	return str;
}

// Two strings that have same polynomial hash for any base, for
// mod = power of 2 up to 2^64.
// Thue–Morse.
// O(1).
inline std::pair<std::string, std::string> unsigned_polynomial_hash_hack() {
	std::string a, b;
	int size = 1 << 10;
	for (int i = 0; i < size; ++i) {
		a += 'a' + math::detail::popcount(i) % 2;
		b += 'a' + ('b' - a[i]);
	}
	return {a, b};
}

// Collides two strings to have the same polynomial hash.
// O(sqrt(mod) log(mod)) with high probability.
// 0 < base < mod.
inline std::pair<std::string, std::string>
polynomial_hash_hack(int alphabet_size, int base, int mod) {
	tgen_ensure(alphabet_size > 1,
				"hack: polynomial_hash_hack: alphabet size must be greater "
				"than 1");
	tgen_ensure(0 < base and base < mod,
				"hack: polynomial_hash_hack: base must be in (0, mod)");

	std::vector<std::string> alphabet(alphabet_size);
	for (int i = 0; i < alphabet_size; ++i)
		alphabet[i] = std::string(1, 'a' + i);
	std::iota(alphabet.begin(), alphabet.end(), 'a');
	return detail::birthday_attack(alphabet, base, mod);
}

// Collides two strings to have the same polynomial hash for multiple bases
// and mods (up to 2 pairs).
// O(sqrt(mod) log^2 (mod)) with high probability,
// with mod = max(mod_1, mod_2).
inline std::pair<std::string, std::string>
polynomial_hash_hack(int alphabet_size, std::vector<int> bases,
					 std::vector<int> mods) {
	tgen_ensure(bases.size() == mods.size(),
				"hack: polynomial_hash_hack: bases and mods must have the same "
				"size");
	tgen_ensure(
		bases.size() > 0,
		"hack: polynomial_hash_hack: must have at least one (base, mod) "
		"pair");
	tgen_ensure(bases.size() <= 2,
				"hack: polynomial_hash_hack: multi-hash hack only supported "
				"for up to 2 (base, mod) pairs");

	std::vector<std::string> alphabet(alphabet_size);
	for (int i = 0; i < alphabet_size; ++i)
		alphabet[i] = std::string(1, 'a' + i);
	auto [S1, T1] = detail::birthday_attack(alphabet, bases[0], mods[0]);
	if (bases.size() == 1)
		return {S1, T1};
	return detail::birthday_attack({S1, T1}, bases[1], mods[1]);
}

// Returns a list of integers for unordered_map/set to force collisions.
// O(size).
inline std::vector<long long> std_unordered(int size) {
	tgen_ensure(size > 0, "hack: std_unordered: size must be positive");
	std::set<long long> multipliers = detail::std_hash_multipliers();
	long long mult = 1;
	std::set<long long>::iterator it = multipliers.begin();

	std::vector<long long> list;
	while (static_cast<int>(list.size()) < size) {
		list.push_back(mult * (*it));
		++it;
		if (it == multipliers.end()) {
			it = multipliers.begin();
			++mult;
		}
	}
	return list;
}

// Returns queries that force \Theta(q sqrt n) asymptotic
// for Mo algorithm for offline range queries.
// Forces \Theta(q sqrt n) pointer moves for any ordering.
// O(n log n + q).
inline std::vector<std::pair<int, int>> mo(int n, int q) {
	std::set<std::pair<int, int>> queries;

	// Adversarial case.
	int sq = std::sqrt(n);
	for (int i = 0; i < sq; ++i) {
		for (int j = i; j < sq; ++j) {
			if (i * sq < n and j * sq < n)
				queries.emplace(i * sq, j * sq);
		}
	}

	// Push extra queries.
	for (int i = 0; i < n; ++i)
		if (queries.size() < size_t(q)) {
			queries.emplace(0, i);
			queries.emplace(i, i);
			queries.emplace(i, n - 1);
		}

	std::vector<std::pair<int, int>> pool(queries.begin(), queries.end());
	while (pool.size() < size_t(q)) {
		int l = next(0, n - 1);
		pool.emplace_back(l, next(l, n - 1));
	}

	return choose(shuffled(pool), q);
}

// Returns list of strings that have a high cost to insert in a std::set.
// Forces cost \Theta(size log(size)).
// Generates: {b, ab, aab, aaab, ...}.
// O(size log(size)).
inline std::vector<std::string> string_set(int size) {
	std::vector<std::string> list;
	int k = 0, left = size;
	while (left > 0) {
		int cur_size = std::min(left, k + 1);
		left -= cur_size;

		char right_char = cur_size == k + 1 ? 'b' : 'c';
		list.push_back(std::string(cur_size - 1, 'a') + right_char);

		++k;
	}
	return tgen::shuffled(list);
}

// Graph for Dijkstra implementations that relax with <= instead of <.
// Unit-weight layered graph: 0 -> {1,2}, then disjoint
// 2x2 gadgets (i,i+1) -> {i+2,i+3} for i = 1,3,5,... Many vertices share the
// same dist from 0; with `d + w <= dist[j]` each pop re-relaxes the whole
// frontier below it. m = 2(n - 2) edges.
// O(n).
inline egraph<int>::value non_strict_relaxation_dijkstra_bug(int n) {
	tgen_ensure(
		n >= 3,
		"hack: non_strict_relaxation_dijkstra_bug: needs at least 3 vertices");

	egraph<int>::value g(n, {}, true);
	g.edge_weighted();
	g.add_edge(0, 1, 1);
	g.add_edge(0, 2, 1);
	for (int i = 1; i + 2 < n; i += 2) {
		g.add_edge(i, i + 2, 1);
		if (i + 3 < n)
			g.add_edge(i, i + 3, 1);

		g.add_edge(i + 1, i + 2, 1);
		if (i + 3 < n)
			g.add_edge(i + 1, i + 3, 1);
	}

	return g.shuffle_except({0});
}

// Graph for Dijkstra implementations that do not skip stale heap entries
// (`if (d > dist[i]) continue`).
// Hub mid = n/2: star 0 -> 1..mid-1 (weights 1..mid-1), funnel i -> mid
// (weights 1,3,5,...), then mid -> mid+1.. (weight 1).
// Without a stale-heap check, mid and its in-neighbors are re-popped and
// re-relax.
// m = n + mid - 3 edges, mid = floor(n/2).
// O(n).
inline egraph<int>::value stale_heap_dijkstra_bug(int n) {
	tgen_ensure(n >= 4,
				"hack: stale_heap_dijkstra_bug: needs at least 4 vertices");

	int mid = n / 2;
	egraph<int>::value g(n, {}, true);
	g.edge_weighted();
	for (int i = 1; i < mid; ++i)
		g.add_edge(0, i, i);
	for (int i = 1; i < mid; ++i)
		g.add_edge(i, mid, 2 * (mid - i) - 1);
	for (int i = mid + 1; i < n; ++i)
		g.add_edge(mid, i, 1);

	return g.shuffle_except({0});
}

// Worst-case for FIFO-SPFA.
// Forces Omega(n^2) from vertex 0 (Theta(n*m), m = 2n - 3).
// Upper chain ai -> a(i+1) weight 1; lower chain bi -> b(i+1) weight 0;
// vertical ai -> bi weight 0; cross bi -> a(i+1) weight 1. Upper chain sets
// loose dist first; cross edges from settled bi then improve a(i+1).
// m = 2n - 3.
// O(n).
inline egraph<int>::value spfa_hack(int n) {
	tgen_ensure(n >= 2, "hack: spfa_hack: n must be at least 2");
	tgen_ensure(n % 2 == 0, "hack: spfa_hack: n must be even");

	egraph<int>::value g(n, {}, true);
	g.edge_weighted();

	const int k = n / 2;
	for (int i = 0; i + 1 < k; ++i)
		g.add_edge(i, i + 1, 1);
	for (int i = 0; i + 1 < k; ++i)
		g.add_edge(k + i, k + i + 1, 0);
	for (int i = 0; i < k; ++i)
		g.add_edge(i, k + i, 0);
	for (int i = 0; i + 1 < k; ++i)
		g.add_edge(k + i, i + 1, 1);

	return g.shuffle_except({0});
}

// Zadeh (1972) anti-shortest-paths flow network for Edmonds-Karp and Dinitz.
// Source is vertex 0; sink is vertex 4l + 2k + 1.
// n = 4l + 2k + 2, m = 6l + 4k + k^2 - 4.
// O(l + k^2).
inline egraph<int>::value dinitz_worst_case(int k, int l) {
	tgen_ensure(k >= 1, "hack: dinitz_worst_case: k must be at least 1");
	tgen_ensure(l >= 1, "hack: dinitz_worst_case: l must be at least 1");

	const int p1 = 2 * l - 1;
	const int p2 = 2 * l;
	const int q1 = 2 * l + 1;
	const int q2 = 2 * l + 2;
	const int n = 4 * l + 2 * k + 2;

	const int flow_cap = k * k * l;
	const int layer_cap = k * k;

	auto a = [&](int i) { return 2 * l + 3 + 2 * i; };
	auto b = [&](int i) { return 2 * l + 4 + 2 * i; };
	auto t = [&](int i) { return 4 * l + 2 * k + 1 - i; };

	egraph<int>::value g(n, {}, true);
	g.edge_weighted();

	for (int i = 0; i + 1 < 2 * l - 1; ++i)
		g.add_edge(i, i + 1, flow_cap);
	for (int i = 0; i + 1 < 2 * l - 1; ++i)
		g.add_edge(t(i + 1), t(i), flow_cap);

	for (int i = 0; i < 2 * l - 1; i += 2) {
		g.add_edge(i, i % 4 == 0 ? p1 : p2, layer_cap);
		g.add_edge(i % 4 == 0 ? q1 : q2, t(i), layer_cap);
	}

	for (int i = 0; i < k; ++i) {
		g.add_edge(p1, a(i), flow_cap);
		g.add_edge(p2, b(i), flow_cap);
		g.add_edge(a(i), q2, flow_cap);
		g.add_edge(b(i), q1, flow_cap);
	}

	for (int i = 0; i < k; ++i)
		for (int j = 0; j < k; ++j)
			g.add_edge(a(i), b(j), 1);

	return g;
}

// Returns a mask of length 19938, with weights such that xor-ing with mt19937
// outputs yields 0.
// O(1).
template <typename T> std::vector<bool> mt19937_xor_hash_hack() {
	static_assert(std::is_same_v<T, int> or std::is_same_v<T, long long>,
				  "hack: mt19937_xor_hash_hack: T must be int or long long");

	constexpr std::size_t deg = 19937;

	std::bitset<deg + 1> a, b, c;
	b[deg] = c[deg] = 1;
	std::size_t l = 0, shift = 1;
	std::mt19937 rng32;
	std::mt19937_64 rng64;
	for (std::size_t n = 0; n < deg * 2; ++n) {
		a >>= 1;
		if constexpr (std::is_same_v<T, int>)
			a[deg] = rng32() & 1;
		else
			a[deg] = rng64() & 1;

		if ((c & a).count() % 2 == 0) {
			++shift;
			continue;
		}

		std::bitset<deg + 1> oc = c;
		c ^= (b >> shift);
		if (2 * l <= n) {
			l = n + 1 - l;
			b = oc;
			shift = 1;
		} else {
			++shift;
		}
	}

	std::vector<bool> mask(deg + 1);
	for (std::size_t i = 0; i <= deg; ++i)
		mask[i] = c[i];
	return mask;
}

// Convex polygon that breaks naive rotating calipers for maximum vertex
// distance (advances j while dist(i, next(j)) > dist(i, j) instead of using
// ccw).
// O(1).
inline std::vector<geometry::point<double>>
naive_rotating_calipers_max_dist_bug() {
	return {
		{-0.9846, -1.53251}, {0.49946, 1.19525},  {0.79916, 0.98291},
		{4.02136, -1.57843}, {3.92734, -2.37856}, {3.88558, -2.37188},
	};
}

namespace detail {

// Builds a hack block of order k (length fib(2k+1)).
// O(fib(2k+1)).
inline std::vector<int> segment_tree_beats_hack_block(int k) {
	tgen_ensure(k >= 1, "hack: segment_tree_beats_hack: k must be at least 1");

	std::vector<int> a(k + 1), b(k + 1);
	std::vector<std::vector<int>> vf(k + 1), vg(k + 1);

	a[1] = b[1] = 1;
	vf[1] = {1};
	vg[1] = {1, 0};

	for (int i = 2; i <= k; ++i) {
		b[i] = b[i - 1] + a[i - 1];
		a[i] = b[i] + a[i - 1];
		for (int x : vf[i - 1])
			vf[i].push_back(x + a[i] + b[i]);
		vf[i].push_back(a[i]);
		for (int x : vg[i - 1])
			vf[i].push_back(x + a[i]);
		vg[i] = vf[i];
		vg[i].push_back(0);
		for (int x : vg[i - 1])
			vg[i].push_back(x);
	}

	vf[k].push_back(0);
	return vf[k];
}

// Appends one update round for the tiled array (offset (round * an) mod L).
// O(fib(2k+1)).
inline void
segment_tree_beats_append_round(std::vector<std::vector<int>> &updates,
								int block_len, int an, int bn, int n,
								int round) {
	const int off = (round * an) % block_len;
	const int add_off = (off + block_len - bn) % block_len;
	for (int k = 0; k < block_len; ++k) {
		const int s = k * block_len * block_len;
		const int sub_end = off + an;
		if (sub_end <= block_len)
			updates.push_back({1, s + off, s + sub_end, bn});
		else {
			updates.push_back({1, s + off, s + block_len, bn});
			updates.push_back({1, s, s + (sub_end - block_len), bn});
		}
		const int add_end = add_off + bn;
		if (add_end <= block_len)
			updates.push_back({0, s + add_off, s + add_end, an});
		else {
			updates.push_back({0, s + add_off, s + block_len, an});
			updates.push_back({0, s, s + (add_end - block_len), an});
		}
	}
	updates.push_back({2, 0, n, an});
	for (int k = 0; k < block_len; ++k) {
		const int s = k * block_len * block_len;
		updates.push_back({3, s + (off + an - 1) % block_len, 0});
	}
}

} // namespace detail

// Array and updates for worst case of segment tree beats.
// O(fib(2k+1)^3 + q).
inline std::pair<std::vector<int>, std::vector<std::vector<int>>>
segment_tree_beats_hack(int k, int q) {
	tgen_ensure(k >= 1, "hack: segment_tree_beats_hack: k must be at least 1");
	tgen_ensure(k <= 7, "hack: segment_tree_beats_hack: k too large");
	tgen_ensure(q > 0, "hack: segment_tree_beats_hack: q must be positive");

	const auto &fib = math::fibonacci();
	const int block_len = fib[k * 2 + 1];
	const int an = fib[k * 2];
	const int bn = fib[k * 2 - 1];

	const int len = block_len;
	const int total = len * len * len;

	std::vector<int> block = detail::segment_tree_beats_hack_block(k);
	std::vector<int> arr(total, 0);
	for (int x = 0; x < block_len; ++x) {
		const int s = x * len * len;
		for (int i = 0; i < block_len; ++i)
			arr[s + i] = block[i];
	}

	std::vector<std::vector<int>> updates;
	updates.reserve(q);
	const int n = total;
	for (int round = 0; updates.size() < static_cast<std::size_t>(q); ++round) {
		detail::segment_tree_beats_append_round(updates, block_len, an, bn, n,
												round);
		if (updates.size() > static_cast<std::size_t>(q))
			updates.resize(q);
	}
	return {arr, updates};
}

} // namespace hack

/*********************
 *                   *
 *   MISCELLANEOUS   *
 *                   *
 *********************/

namespace misc {

// Generates a uniformly random balanced parentheses sequence with k '(' and k
// ')'. Valid means that for no prefix there are more ')' than '('.
// O(size).
inline std::string gen_parenthesis(int size) {
	tgen_ensure(size > 0 and size % 2 == 0,
				"misc: parenthesis: size must be a positive even number");

	int k = size / 2;
	std::string s;
	int open = 0, close = 0;

	for (int i = 0; i < size; ++i) {
		if (open == k) {
			s += ')';
			++close;
			continue;
		}
		if (open == close) {
			s += '(';
			++open;
			continue;
		}

		long long a = k - open, b = k - close, h = open - close;

		// Probability of placing '(':
		// P('(') = (k - open) * (h + 2) / ((k - open + k - close) * (h + 1))
		// Derived from ballot numbers ratio.
		long long num = a * (h + 2);
		long long den = (a + b) * (h + 1);

		if (next<long long>(1, den) <= num) {
			s += '(';
			++open;
		} else {
			s += ')';
			++close;
		}
	}

	return s;
}

} // namespace misc

} // namespace tgen

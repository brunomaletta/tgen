#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tgen {

/**************************
 *                        *
 *   GENERAL OPERATIONS   *
 *                        *
 **************************/

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
void __contradiction_error(const std::string &type,
						   const std::string &msg = "") {
	// Tried to generate a contradicting sequence.
	std::string error_msg = "invalid " + type + " (contradicting constraints)";
	if (msg.size() > 0)
		error_msg += ": " + msg;
	throw __error(error_msg);
}

// Ensures condition is true, with nice debug.
#define tgen_ensure(cond, ...)                                                 \
	if (!(cond))                                                               \
		tgen::__throw_assertion_error(#cond, ##__VA_ARGS__);

/*
 * Global random operations.
 */

std::mt19937 __rng;

template <typename T> T __next_integral(T l, T r) {
	tgen_ensure(l <= r, "range for `next` bust be valid");
	std::uniform_int_distribution<T> dist(l, r);
	return dist(__rng);
}
double __next_double(double l, double r) {
	tgen_ensure(l <= r, "range for `next` bust be valid");
	std::uniform_real_distribution<double> dist(l, r);
	return dist(__rng);
}

// Returns a random number in [l, r].
template <typename T> T next(T l, T r) {
	if (std::is_integral<T>())
		return __next_integral<T>(l, r);
	if (std::is_floating_point<T>())
		return __next_double(l, r);
	throw __error("invalid type for next (" + std::string(typeid(T).name()) +
				  ")");
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

// Returns a random element from [first, last).
template <typename It> typename It::value_type any(It first, It last) {
	int size = std::distance(first, last);
	It it = first;
	std::advance(it, next(0, size - 1));
	return *it;
}

// Returns a random element from container.
template <typename C> typename C::value_type any(const C &container) {
	return any(container.begin(), container.end());
}
template <typename T> T any(const std::initializer_list<T> &list) {
	return any(std::vector<T>(list.begin(), list.end()));
}

// Chooses k values from the container, as in a subsequence of size k. Returns a
// copy.
template <typename C> C choose(int k, const C &container) {
	tgen_ensure(0 < k and k <= container.size(),
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

std::vector<std::string>
	__pos_opts; // Dictionary containing the positional parsed opts.
std::map<std::string, std::string>
	__named_opts; // Global dictionary the named parsed opts.

// Returns true if there is an opt at a given index.
bool has_opt(int index) { return 0 <= index and index < __pos_opts.size(); }

// Returns true if there is an opt with a given key.
bool has_opt(const std::string &key) { return __named_opts.count(key) != 0; }

template <typename T>
T __get_opt(const std::string &value, std::true_type, std::false_type,
			std::false_type) {
	// Parses 'value' into bool.
	if (value == "true" or value == "1")
		return true;
	if (value == "false" or value == "0")
		return false;
	throw __error("invalid value " + value + " for type bool");
}
template <typename T>
T __get_opt(const std::string &value, std::false_type, std::true_type,
			std::false_type) {
	// Parses 'value' into integral type T.
	if (std::is_unsigned<T>())
		return static_cast<T>(std::stoull(value));
	return static_cast<T>(std::stoll(value));
}
template <typename T>
T __get_opt(const std::string &value, std::false_type, std::false_type,
			std::true_type) {
	// Parses 'value' into floating type T.
	return static_cast<T>(std::stold(value));
}
template <typename T>
T __get_opt(const std::string &value, std::false_type, std::false_type,
			std::false_type) {
	// Parses 'value' into std::string.
	return value;
}
template <typename T> T __get_opt(const std::string &value) {
	// Parses 'value' into type T.
	try {
		return __get_opt<T>(value, std::is_same<T, bool>(),
							std::is_integral<T>(), std::is_floating_point<T>());
	} catch (const std::invalid_argument &er) {
	}
	throw __error("invalid value " + value + " for type " + typeid(T).name());
}

// Returns the parsed opt by a given index.
template <typename T> T opt(int index) {
	tgen_ensure(has_opt(index), "cannot find key with index " + index);
	return __get_opt<T>(__pos_opts[index]);
}

// Returns the parsed opt by a given index. If no opts with the index are
// found, returns the given default_value.
template <typename T> T opt(int index, const T &default_value) {
	if (!has_opt(index))
		return default_value;
	return __get_opt<T>(__pos_opts[index]);
}

// Returns the parsed opt by a given key.
template <typename T> T opt(const std::string &key) {
	tgen_ensure(has_opt(key), "cannot find key with key " + key);
	return __get_opt<T>(__named_opts[key]);
}

// Returns the parsed opt by a given key. If no opts with the given key are
// found, returns the given default_value.
template <typename T> T opt(const std::string &key, const T &default_value) {
	if (!has_opt(key))
		return default_value;
	return __get_opt<T>(__named_opts[key]);
}

char __fetch_char(char *s, int idx) {
	tgen_ensure(s[idx] != '\n', "tried to fetch end of string");
	return s[idx];
}
std::string __read_until(char *s) {
	// Reads non-empty string until it hits a ' ' or the string ends.
	std::string read_str;
	int idx = 0;
	while (s[idx] != '\0') {
		char nxt_char = __fetch_char(s, idx++);
		if (nxt_char == ' ')
			break;
		read_str += nxt_char;
	}

	tgen_ensure(read_str.size() > 0, "read string cannot be empty");
	return read_str;
}
void __parse_opts(int argc, char **argv) {
	// Parses the opts into __pos_opts vector and __named_opts map.
	// Starting from 1 to ignore the name of the executable.
	for (int i = 1; i < argc; i++) {
		std::string key = __read_until(argv[i]);

		if (key[0] == '-') {
			tgen_ensure(key.size() > 1,
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
			tgen_ensure(key.size() > 1,
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
			tgen_ensure(
				key.size() > 0 and value.size() > 0, "expected non-empty\
						key/value in opt (" + std::string(argv[1]));
			tgen_ensure(__named_opts.count(key) == 0,
						"cannot have repeated keys");
			__named_opts[key] = value;
		} else {
			// This is the '--key value' case.
			tgen_ensure(__named_opts.count(key) == 0,
						"cannot have repeated keys");
			tgen_ensure(argv[i + 1], "value cannot be empty");
			__named_opts[key] = __read_until(argv[i + 1]);
			i++;
		}
	}
}

// Registers generator by initializing rnd and parsing opts.
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

/****************
 *              *
 *   SEQUENCE   *
 *              *
 ****************/

/*
 * Sequence generator.
 */

template <typename T> struct sequence {
	int size;			// Size of sequence.
	T value_l, value_r; // Range of defined values.
	std::set<T> values; // Set of values. If empty, use range. if not,
						// represents the possible values, and the range
						// represents the index in this set)
	std::map<T, int> value_idx_in_set; // Index of every value in the set above.
	std::vector<std::pair<T, T>> val_range; // Range of values of each index.
	std::vector<std::vector<int>> neigh;	// Adjacency list of equality.
	std::set<int>
		idx_distinct_constraints; // Indices in some distinction constraint.
	std::vector<std::set<int>>
		distinct_constraints; // All distinct constraints.

	// Creates generator for sequences of size 'size', with random T in [l, r]
	sequence(int size, T value_l, T value_r)
		: size(size), value_l(value_l), value_r(value_r), neigh(size) {
		tgen_ensure(value_l <= value_r, "value range must be valid");
		for (int i = 0; i < size; ++i)
			val_range.emplace_back(value_l, value_r);
	}

	// Creates sequence with value set.
	sequence(int size, const std::set<T> &values)
		: size(size), values(values), neigh(size) {
		tgen_ensure(values.size() > 0, "must have at least one value");
		value_l = 0, value_r = values.size() - 1;
		for (int i = 0; i < size; ++i)
			val_range.emplace_back(value_l, value_r);
		int idx = 0;
		for (T value : values)
			value_idx_in_set[value] = idx++;
	}
	sequence(int size, const std::vector<T> &values)
		: sequence(size, std::set<T>(values.begin(), values.end())) {}
	sequence(int size, const std::initializer_list<T> &values)
		: sequence(size, std::set<T>(values.begin(), values.end())) {}

	// Restricts sequences for sequence[idx] = value.
	sequence &value_at_idx(int idx, T value) {
		tgen_ensure(0 <= idx and idx < size, "index must be valid");
		if (values.size() == 0) {
			auto &[left, right] = val_range[idx];
			tgen_ensure(left <= value and value <= right,
						"value must be in the defined range");
			left = right = value;
		} else {
			tgen_ensure(values.count(value),
						"value must be in the set of values");
			auto &[left, right] = val_range[idx];
			int new_val = value_idx_in_set[value];
			tgen_ensure(left <= new_val and new_val <= right,
						"must not set to two different values");
			left = right = new_val;
		}
		return *this;
	}

	// Restricts sequences for sequence[idx_1] = sequence[idx_2].
	sequence &equal_idx_pair(int idx_1, int idx_2) {
		tgen_ensure(0 <= std::min(idx_1, idx_2) and
						std::max(idx_1, idx_2) < size,
					"indices must be valid");
		if (idx_1 == idx_2)
			return *this;

		neigh[idx_1].push_back(idx_2);
		neigh[idx_2].push_back(idx_1);
		return *this;
	}

	// Restricts sequences for sequence[left..right] to have all equal values.
	sequence &equal_range(int left, int right) {
		tgen_ensure(0 <= left and left <= right and right < size,
					"range indices bust be valid");
		for (int i = left; i < right; ++i)
			equal_idx_pair(i, i + 1);
		return *this;
	}

	// Restricts sequences for sequence[S] to be distinct, for given subset S of
	// indices.
	// You can not add two of these restrictions with intersection.
	sequence &distinct_idx_set(const std::set<int> &indices) {
		for (int idx : indices) {
			if (idx_distinct_constraints.count(idx))
				throw __error(
					"cannot add same index in two distinct constraints");
			idx_distinct_constraints.insert(idx);
		}

		distinct_constraints.push_back(indices);
		return *this;
	}

	// Restricts sequences for sequence[idx_1] != sequence[idx_2]
	sequence &different_idx_pair(int idx_1, int idx_2) {
		std::set<int> indices = {idx_1, idx_2};
		distinct_idx_set(indices);
		return *this;
	}

	// Restricts sequences with distinct elements.
	sequence &distinct() {
		std::set<int> indices;
		for (int i = 0; i < size; ++i)
			indices.insert(i);
		distinct_idx_set(indices);
		return *this;
	}

	// Sequence instance.
	// Operations on an instance are not random.
	struct instance {
		using value_type = T; // Value type, for templates.
		std::vector<T> vec;	  // Sequence.

		instance(const std::vector<T> &vec) : vec(vec) {}

		// Fetches size.
		size_t size() { return vec.size(); }

		// Fetches position idx.
		T operator[](int idx) const { return vec[idx]; }

		// Sorts values in increasign order.
		instance &sort() {
			std::sort(vec.begin(), vec.end());
			return *this;
		}

		// Reverses sequence.
		instance &reverse() {
			std::reverse(vec.begin(), vec.end());
			return *this;
		}

		// Prints in stdout, separated by spaces.
		friend std::ostream &operator<<(std::ostream &out,
										const instance &inst) {
			for (int i = 0; i < inst.vec.size(); ++i) {
				if (i > 0)
					out << ' ';
				out << inst.vec[i];
			}
			return out;
		}

		// Gets a std::vector representing the instance.
		std::vector<T> to_std() { return vec; }
	};

	// Generate sequence instance.
	instance gen() {
		std::vector<T> vec(size);

		std::vector<int> comp_id(size, -1);		  // component id of each index.
		std::vector<std::vector<int>> comp(size); // component of each comp-id.
		int comp_count = 0; // number of different components.
		std::vector<bool> vis(size, false);
		for (int i = 0; i < size; ++i)
			if (!vis[i]) {
				T new_value;
				bool value_set = false;

				// BFS to visit the connected component, grouping equal values.
				std::queue<int> q;
				q.push(i);
				vis[i] = true;
				std::vector<int> component;
				while (q.size()) {
					int j = q.front();
					q.pop();

					component.push_back(j);

					// Checks value.
					auto [l, r] = val_range[j];
					if (l == r) {
						if (!value_set) {
							// We found the value.
							value_set = true;
							new_value = l;
						} else if (new_value != l) {
							// We found a contradiction
							std::stringstream ss;
							ss << "tried to set value to `" << new_value
							   << "`, but it was already set as `" << l << "`";
							__contradiction_error("sequence", ss.str());
						}
					}

					for (int k : neigh[j]) {
						if (!vis[k]) {
							vis[k] = true;
							q.push(k);
						}
					}
				}

				// Group entire component, checking if value is defined.
				for (int j : component) {
					if (value_set)
						val_range[j].first = val_range[j].second = new_value;
					comp_id[j] = comp_count;
					comp[comp_id[j]].push_back(j);
				}
				++comp_count;
			}

		// Adds artificial distinct constraints.
		for (int i = 0; i < size; ++i)
			if (!idx_distinct_constraints.count(i)) {
				std::set<int> cur = {i};
				distinct_constraints.push_back(cur);
			}

		// Loops through distinct constraints.
		for (const std::set<int> &distinct : distinct_constraints) {
			// Checks if there are too many distinct values.
			if (distinct.size() > value_r - value_l + 1)
				__contradiction_error(
					"sequence", "tried to generate " +
									std::to_string(distinct.size()) +
									" distinct values, but the maximum is " +
									std::to_string(value_r - value_l + 1));

			// Checks if two values in same component are marked as different.
			std::set<int> comp_ids;
			for (int idx : distinct) {
				if (comp_ids.count(comp_id[idx]))
					__contradiction_error(
						"sequence",
						"tried to set indices two indices as equal and "
						"different");
				comp_ids.insert(comp_id[idx]);
			}

			// Generates set of already defined values.
			std::set<T> defined_values;
			for (int idx : distinct)
				if (val_range[idx].first == val_range[idx].second)
					defined_values.insert(val_range[idx].first);

			// We have values in [l, r], but those in defined_values are
			// taken. We generate our numbers in the range [0, lim) with
			// lim = (r-l+1)-(defined_values.size()), and then map them to
			// the correct range.
			// We will run (distinct.size()-defined_values.size()) steps of
			// Fisher–Yates, using a map to store a virtual sequence that starts
			// with the a[i] = i.
			int lim = (value_r - value_l + 1) - int(defined_values.size());
			std::map<T, T> virtual_sequence;
			std::vector<T> initial_gen;
			for (int i = 0; i < distinct.size() - int(defined_values.size());
				 i++) {
				T j = next<T>(i, lim - 1);
				T vj = virtual_sequence.count(j) ? virtual_sequence[j] : j;
				T vi = virtual_sequence.count(i) ? virtual_sequence[i] : T(i);

				virtual_sequence[j] = vi, virtual_sequence[i] = vj;

				initial_gen.push_back(virtual_sequence[i]);
			}

			// Shifts back to correct range, but there might still be values
			// that we can not use.
			for (T &value : initial_gen)
				value += value_l;

			// Now for every generated value that is in defined_values, we map
			// it to [l + lim, l + lim + defined_values.size()).
			std::vector<std::pair<T, int>> values_to_map;
			for (int i = 0; i < initial_gen.size(); ++i)
				if (defined_values.count(initial_gen[i])) {
					values_to_map.emplace_back(initial_gen[i], i);
				}
			// We iterate through them in increasing order.
			std::sort(values_to_map.begin(), values_to_map.end());
			auto cur_it = defined_values.begin();
			int cur_defined_idx = 0;
			for (auto [val, idx] : values_to_map) {
				while (*cur_it != val)
					++cur_it, ++cur_defined_idx;
				initial_gen[idx] = value_l + lim + cur_defined_idx;
			}

			int cur = 0;
			for (int idx : distinct) {
				T value;
				if (val_range[idx].first == val_range[idx].second)
					value = val_range[idx].first; // Value was already set.
				else
					value = initial_gen[cur++]; // Use a generated random value.

				// For every index in this component, sets it.
				for (int i : comp[comp_id[idx]])
					vec[i] = value;
			}
		}

		if (values.size() > 0) {
			// Needs to fetch the values from the value set.
			std::vector<T> value_vec(values.begin(), values.end());
			std::vector<T> final_vec;
			for (int i = 0; i < size; ++i)
				final_vec.push_back(value_vec[vec[i]]);
			swap(vec, final_vec);
		}

		return instance(vec);
	}

	// Calls the generator until predicate is true.
	template <typename PRED>
	instance gen_until(PRED predicate, int max_tries,
					   bool random_default = false) {
		for (int i = 0; i < max_tries; ++i) {
			instance inst = gen();
			if (predicate(inst))
				return inst;
		}
		if (random_default)
			return gen();
		else
			throw __error("could not generate instance matching predicate");
	}
};

/*
 * Sequence random operations.
 */

namespace sequence_op {

// Shuffles an sequence.
template <typename INST> INST shuffle(const INST &inst) {
	INST new_inst = inst;
	tgen::shuffle(new_inst.vec);
	return new_inst;
}

// Choses any value in the sequence.
template <typename INST> typename INST::value_type any(const INST &inst) {
	return inst.vec[next(0, inst.vec.size() - 1)];
}

// Chooses k values from the sequence, as in a subsequence of size k.
template <typename INST> INST choose(int k, const INST &inst) {
	tgen_ensure(0 < k and k <= inst.vec.size(),
				"number of elements to choose must be valid");
	std::vector<typename INST::value_type> new_vec;
	int need = k;
	for (int i = 0; need > 0; ++i) {
		int left = inst.vec.size() - i;
		if (next(1, left) <= need) {
			new_vec.push_back(inst.vec[i]);
			need--;
		}
	}
	return INST(new_vec);
}

}; // namespace sequence_op

}; // namespace tgen

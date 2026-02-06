#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <queue>
#include <random>
#include <set>
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

void throw_assertion_error_internal(const std::string &condition,
									const std::string &msg) {
	throw std::runtime_error("tgen: " + msg + " (assertion `" + condition +
							 "` failed).");
}
void throw_assertion_error_internal(const std::string &condition) {
	throw std::runtime_error("tgen: assertion `" + condition + "` failed.");
}
std::runtime_error error_internal(const std::string &msg) {
	return std::runtime_error("tgen: " + msg);
}
void contradiction_error_internal(const std::string &type,
								  const std::string &msg = "") {
	// Tried to generate a contradicting sequence.
	std::string error_msg = "invalid " + type + " (contradicting constraints)";
	if (msg.size() > 0)
		error_msg += ": " + msg;
	throw error_internal(error_msg);
}

// Ensures condition is true, with nice debug.
#define tgen_ensure(cond, ...)                                                 \
	if (!(cond))                                                               \
		tgen::throw_assertion_error_internal(#cond, ##__VA_ARGS__);

/*
 * Global random operations.
 */

std::mt19937 rng_internal;

template <typename T> T next_integral_internal(T l, T r) {
	tgen_ensure(l <= r, "range for `next` bust be valid");
	std::uniform_int_distribution<T> dist(l, r);
	return dist(rng_internal);
}
double next_double_internal(double l, double r) {
	tgen_ensure(l <= r, "range for `next` bust be valid");
	std::uniform_real_distribution<double> dist(l, r);
	return dist(rng_internal);
}

// Returns a random number in [l, r].
template <typename T> T next(T l, T r) {
	if (std::is_integral<T>())
		return next_integral_internal<T>(l, r);
	if (std::is_floating_point<T>())
		return next_double_internal(l, r);
	throw error_internal("invalid type for next (" +
						 std::string(typeid(T).name()) + ")");
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
	pos_opts_internal; // Dictionary containing the positional parsed opts.
std::map<std::string, std::string>
	named_opts_internal; // Global dictionary the named parsed opts.

// Returns true if there is an opt at a given index.
bool has_opt(int index) {
	return 0 <= index and index < pos_opts_internal.size();
}

// Returns true if there is an opt with a given key.
bool has_opt(const std::string &key) {
	return named_opts_internal.count(key) != 0;
}

template <typename T>
T get_opt_internal(const std::string &value, std::true_type, std::false_type,
				   std::false_type) {
	// Parses 'value' into bool.
	if (value == "true" or value == "1")
		return true;
	if (value == "false" or value == "0")
		return false;
	throw error_internal("invalid value " + value + " for type bool");
}
template <typename T>
T get_opt_internal(const std::string &value, std::false_type, std::true_type,
				   std::false_type) {
	// Parses 'value' into integral type T.
	if (std::is_unsigned<T>())
		return static_cast<T>(std::stoull(value));
	return static_cast<T>(std::stoll(value));
}
template <typename T>
T get_opt_internal(const std::string &value, std::false_type, std::false_type,
				   std::true_type) {
	// Parses 'value' into floating type T.
	return static_cast<T>(std::stold(value));
}
template <typename T>
T get_opt_internal(const std::string &value, std::false_type, std::false_type,
				   std::false_type) {
	// Parses 'value' into std::string.
	return value;
}
template <typename T> T get_opt_internal(const std::string &value) {
	// Parses 'value' into type T.
	try {
		return get_opt_internal<T>(value, std::is_same<T, bool>(),
								   std::is_integral<T>(),
								   std::is_floating_point<T>());
	} catch (const std::invalid_argument &er) {
	}
	throw error_internal("invalid value " + value + " for type " +
						 typeid(T).name());
}

// Returns the parsed opt by a given index.
template <typename T> T opt(int index) {
	tgen_ensure(has_opt(index), "cannot find key with index " + index);
	return get_opt_internal<T>(pos_opts_internal[index]);
}

// Returns the parsed opt by a given index. If no opts with the index are
// found, returns the given default_value.
template <typename T> T opt(int index, const T &default_value) {
	if (!has_opt(index))
		return default_value;
	return get_opt_internal<T>(pos_opts_internal[index]);
}

// Returns the parsed opt by a given key.
template <typename T> T opt(const std::string &key) {
	tgen_ensure(has_opt(key), "cannot find key with key " + key);
	return get_opt_internal<T>(named_opts_internal[key]);
}

// Returns the parsed opt by a given key. If no opts with the given key are
// found, returns the given default_value.
template <typename T> T opt(const std::string &key, const T &default_value) {
	if (!has_opt(key))
		return default_value;
	return get_opt_internal<T>(named_opts_internal[key]);
}

char fetch_char_internal(char *s, int idx) {
	tgen_ensure(s[idx] != '\n', "tried to fetch end of string");
	return s[idx];
}
std::string read_until_internal(char *s) {
	// Reads non-empty string until it hits a ' ' or the string ends.
	std::string read_str;
	int idx = 0;
	while (s[idx] != '\0') {
		char nxt_char = fetch_char_internal(s, idx++);
		if (nxt_char == ' ')
			break;
		read_str += nxt_char;
	}

	tgen_ensure(read_str.size() > 0, "read string cannot be empty");
	return read_str;
}
void parse_opts_internal(int argc, char **argv) {
	// Parses the opts into `pos_opts_internal` vector and `named_opts_internal`
	// map. Starting from 1 to ignore the name of the executable.
	for (int i = 1; i < argc; i++) {
		std::string key = read_until_internal(argv[i]);

		if (key[0] == '-') {
			tgen_ensure(key.size() > 1,
						"invalid opt (" + std::string(argv[i]) + ")");
			if (isdigit(key[1])) {
				// This case is a positional negative number argument
				pos_opts_internal.push_back(key);
				continue;
			}

			// pops first char '-'
			key = key.substr(1);
		} else {
			// This case is a positional argument that does not start with '-'
			pos_opts_internal.push_back(key);
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
			tgen_ensure(
				key.size() > 0 and value.size() > 0, "expected non-empty\
						key/value in opt (" + std::string(argv[1]));
			tgen_ensure(named_opts_internal.count(key) == 0,
						"cannot have repeated keys");
			named_opts_internal[key] = value;
		} else {
			// This is the '--key value' case.
			tgen_ensure(named_opts_internal.count(key) == 0,
						"cannot have repeated keys");
			tgen_ensure(argv[i + 1], "value cannot be empty");
			named_opts_internal[key] = read_until_internal(argv[i + 1]);
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
	rng_internal.seed(seq);

	parse_opts_internal(argc, argv);
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
		distinct_constraints_; // All distinct constraints.

	// Creates generator for sequences of size 'size', with random T in [l, r]
	sequence(int size, T value_l, T value_r)
		: size_(size), value_l_(value_l), value_r_(value_r), neigh_(size) {
		tgen_ensure(value_l_ <= value_r_, "value range must be valid");
		for (int i = 0; i < size_; ++i)
			val_range_.emplace_back(value_l_, value_r_);
	}

	// Creates sequence with value set.
	sequence(int size, const std::set<T> &values)
		: size_(size), values_(values), neigh_(size) {
		tgen_ensure(values.size() > 0, "must have at least one value");
		value_l_ = 0, value_r_ = values.size() - 1;
		for (int i = 0; i < size_; ++i)
			val_range_.emplace_back(value_l_, value_r_);
		int idx = 0;
		for (T value : values_)
			value_idx_in_set_[value] = idx++;
	}
	sequence(int size, const std::vector<T> &values)
		: sequence(size, std::set<T>(values.begin(), values.end())) {}
	sequence(int size, const std::initializer_list<T> &values)
		: sequence(size, std::set<T>(values.begin(), values.end())) {}

	// Restricts sequences for sequence[idx] = value.
	sequence &set(int idx, T value) {
		tgen_ensure(0 <= idx and idx < size_, "index must be valid");
		if (values_.size() == 0) {
			auto &[left, right] = val_range_[idx];
			tgen_ensure(left <= value and value <= right,
						"value must be in the defined range");
			left = right = value;
		} else {
			tgen_ensure(values_.count(value),
						"value must be in the set of values");
			auto &[left, right] = val_range_[idx];
			int new_val = value_idx_in_set_[value];
			tgen_ensure(left <= new_val and new_val <= right,
						"must not set to two different values");
			left = right = new_val;
		}
		return *this;
	}

	// Restricts sequences for sequence[idx_1] = sequence[idx_2].
	sequence &equal(int idx_1, int idx_2) {
		tgen_ensure(0 <= std::min(idx_1, idx_2) and
						std::max(idx_1, idx_2) < size_,
					"indices must be valid");
		if (idx_1 == idx_2)
			return *this;

		neigh_[idx_1].push_back(idx_2);
		neigh_[idx_2].push_back(idx_1);
		return *this;
	}

	// Restricts sequences for sequence[left..right] to have all equal values.
	sequence &equal_range(int left, int right) {
		tgen_ensure(0 <= left and left <= right and right < size_,
					"range indices bust be valid");
		for (int i = left; i < right; ++i)
			equal(i, i + 1);
		return *this;
	}

	// Restricts sequences for sequence[S] to be distinct, for given subset S of
	// indices.
	// You can not add two of these restrictions with intersection.
	sequence &distinct(const std::set<int> &indices) {
		distinct_constraints_.push_back(indices);
		return *this;
	}
	sequence &distinct(const std::initializer_list<int> &indices) {
		return distinct(std::set<int>(indices.begin(), indices.end()));
	}

	// Restricts sequences for sequence[idx_1] != sequence[idx_2]
	sequence &different(int idx_1, int idx_2) {
		std::set<int> indices = {idx_1, idx_2};
		distinct(indices);
		return *this;
	}

	// Restricts sequences with distinct elements.
	sequence &distinct() {
		std::set<int> indices;
		for (int i = 0; i < size_; ++i)
			indices.insert(i);
		distinct(indices);
		return *this;
	}

	// Sequence instance.
	// Operations on an instance are not random.
	struct instance {
		using value_type = T; // Value type, for templates.
		std::vector<T> vec_;  // Sequence.

		instance(const std::vector<T> &vec) : vec_(vec) {}
		instance(const std::initializer_list<T> &list)
			: vec_(list.begin(), list.end()) {}

		// Fetches size.
		std::size_t size() const { return vec_.size(); }

		// Fetches position idx.
		T &operator[](int idx) { return vec_[idx]; }
		const T &operator[](int idx) const { return vec_[idx]; }

		// Sorts values in increasign order.
		instance &sort() {
			std::sort(vec_.begin(), vec_.end());
			return *this;
		}

		// Reverses sequence.
		instance &reverse() {
			std::reverse(vec_.begin(), vec_.end());
			return *this;
		}

		// Concatenates two instances.
		instance operator+(const instance &rhs) {
			std::vector<T> new_vec = vec_;
			for (int i = 0; i < rhs.size(); ++i)
				new_vec.push_back(rhs[i]);
			return instance(new_vec);
		}

		// Prints in stdout, separated by spaces.
		friend std::ostream &operator<<(std::ostream &out,
										const instance &inst) {
			for (int i = 0; i < inst.vec_.size(); ++i) {
				if (i > 0)
					out << ' ';
				out << inst.vec_[i];
			}
			return out;
		}

		// Gets a std::vector representing the instance.
		std::vector<T> to_std() const { return vec_; }
	};

	// Generate sequence instance.
	instance gen() {
		std::vector<T> vec(size_);
		std::vector<bool> defined_idx(
			size_, false); // For every index, if it has been set in `vec`.

		std::vector<int> comp_id(size_, -1); // Component id of each index.
		std::vector<std::vector<int>> comp(size_); // Component of each comp-id.
		int comp_count = 0; // Number of different components.

		// Defines value of entire component.
		auto define_comp = [&](int comp_id, T val) {
			for (int idx : comp[comp_id]) {
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
					std::queue<int> q;
					q.push(idx);
					vis[idx] = true;
					std::vector<int> component;
					while (q.size()) {
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
								contradiction_error_internal(
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

					// Sets value if needed.
					if (value_defined)
						define_comp(comp_count, new_value);

					++comp_count;
				}
		}

		// Initial parsing of distinct constraints.
		std::vector<std::set<int>> distinct_containing_comp_idx(comp_count);
		{
			int dist_id = 0;
			for (const std::set<int> &distinct : distinct_constraints_) {
				// Checks if there are too many distinct values.
				if (distinct.size() > value_r_ - value_l_ + 1)
					contradiction_error_internal(
						"sequence",
						"tried to generate " + std::to_string(distinct.size()) +
							" distinct values, but the maximum is " +
							std::to_string(value_r_ - value_l_ + 1));

				// Checks if two values in same component are marked as
				// different.
				std::set<int> comp_ids;
				for (int idx : distinct) {
					if (comp_ids.count(comp_id[idx]))
						contradiction_error_internal(
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
				throw error_internal(
					"failed to generate sequence: complex constraints");

		// Generates a uniformly random list of k distinct values in [l, r],
		// such that no value is in `forbidden_values`.
		auto generate_distinct_values =
			[&](int k, const std::set<T> forbidden_values) {
				// We generate our numbers in the range [0, lim) with
				// lim = (r-l+1)-(forbidden_values.size()), and then map them to
				// the correct range.
				// We will run k steps of Fisher–Yates, using a map to store a
				// virtual sequence that starts with a[i] = i.
				int lim =
					(value_r_ - value_l_ + 1) - int(forbidden_values.size());
				std::map<T, T> virtual_list;
				std::vector<T> list;
				for (int i = 0; i < k; i++) {
					T j = next<T>(i, lim - 1);
					T vj = virtual_list.count(j) ? virtual_list[j] : j;
					T vi = virtual_list.count(i) ? virtual_list[i] : T(i);

					virtual_list[j] = vi, virtual_list[i] = vj;

					list.push_back(virtual_list[i]);
				}

				// Shifts back to correct range, but there might still be values
				// that we can not use.
				for (T &value : list)
					value += value_l_;

				// Now for every generated value that is in forbidden_values, we
				// map it to [l + lim, l + lim + forbidden_values.size()).
				std::vector<std::pair<T, int>> values_to_map;
				for (int i = 0; i < list.size(); ++i)
					if (forbidden_values.count(list[i])) {
						values_to_map.emplace_back(list[i], i);
					}
				// We iterate through them in increasing order.
				std::sort(values_to_map.begin(), values_to_map.end());
				auto cur_it = forbidden_values.begin();
				int cur_defined_idx = 0;
				for (auto [val, idx] : values_to_map) {
					while (*cur_it != val)
						++cur_it, ++cur_defined_idx;
					list[idx] = value_l_ + lim + cur_defined_idx;
				}

				return list;
			};

		std::vector<bool> vis_distinct(distinct_constraints_.size(), false);
		std::vector<bool> initially_defined_comp_idx(comp_count, false);

		// Fills the value in a tree defined by distinct constraints.
		auto define_tree = [&](int distinct_id) {
			// The set `distinct_constraints_[distinct_id]` can have some values
			// that are defined.

			// Generates set of already defined values.
			std::set<T> defined_values;
			for (int idx : distinct_constraints_[distinct_id])
				if (defined_idx[idx]) {
					// Checks if two values in `distinct_constraints_[dist_id]`
					// have been set to the same value
					if (defined_values.count(vec[idx]))
						contradiction_error_internal(
							"sequence",
							"tried to set two indices as equal and different");

					defined_values.insert(vec[idx]);
				}

			// Generates values in this root distinct constraint.
			{
				int new_value_count =
					distinct_constraints_[distinct_id].size() -
					int(defined_values.size());
				std::vector<T> generated_values =
					generate_distinct_values(new_value_count, defined_values);
				auto val_it = generated_values.begin();
				for (int idx : distinct_constraints_[distinct_id])
					if (defined_idx[idx]) {
						// The root can cover these components, but there should
						// not be any other defined in this tree.
						initially_defined_comp_idx[comp_id[idx]] = false;
					} else {
						define_comp(comp_id[idx], *val_it);
						++val_it;
					}
			}

			// BFS on the tree of distinct constraints.
			std::queue<std::pair<int, int>> q; // {id, parent id}
			q.emplace(distinct_id, -1);
			vis_distinct[distinct_id] = true;
			while (q.size()) {
				auto [cur_distinct, parent] = q.front();
				q.pop();

				std::set<int> neigh_distinct;
				for (int idx : distinct_constraints_[cur_distinct])
					for (int nxt_distinct :
						 distinct_containing_comp_idx[comp_id[idx]]) {
						if (nxt_distinct == cur_distinct or
							nxt_distinct == parent)
							continue;

						// Cycle.
						if (vis_distinct[nxt_distinct])
							throw error_internal("failed to generate sequence: "
												 "complex constraints");

						neigh_distinct.insert(nxt_distinct);
					}

				for (int nxt_distinct : neigh_distinct) {
					vis_distinct[nxt_distinct] = true;
					q.emplace(nxt_distinct, cur_distinct);

					// Generates this distinct constraint.
					std::set<T> defined_values;
					for (int idx2 : distinct_constraints_[nxt_distinct])
						if (defined_idx[idx2]) {
							// There can not be any more defined. This case is
							// when there are values not coverered by a single
							// distinct constraint in the tree.
							if (initially_defined_comp_idx[comp_id[idx2]])
								throw error_internal(
									"failed to generate sequence: "
									"complex constraints");

							defined_values.insert(vec[idx2]);
						}
					int new_value_count =
						distinct_constraints_[nxt_distinct].size() -
						int(defined_values.size());
					std::vector<T> generated_values = generate_distinct_values(
						new_value_count, defined_values);
					auto val_it = generated_values.begin();
					for (int idx2 : distinct_constraints_[nxt_distinct])
						if (!defined_idx[idx2]) {
							define_comp(comp_id[idx2], *val_it);
							++val_it;
						}
				}
			}
		};

		// Loops through distinct constraints, sorts distinct constraints
		// by number of defined components (non-increasing). This guarantees
		// that if there is a valid root (that covers all 'defined'), we find
		// it.
		{
			std::vector<std::pair<int, int>> defined_cnt_and_distinct_idx;
			int dist_id = 0;
			for (const std::set<int> &distinct : distinct_constraints_) {
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

		// Loops through distinct constraints do define the rest.
		for (int dist_id = 0; dist_id < distinct_constraints_.size(); ++dist_id)
			if (!vis_distinct[dist_id])
				define_tree(dist_id);

		// Define final values. These values all should be random in [l, r], and
		// the distinct constraints have already been processed. However, there
		// can be still equality constraints, so we set entire components.
		for (int idx = 0; idx < size_; ++idx)
			if (!defined_idx[idx])
				define_comp(comp_id[idx], next<T>(value_l_, value_r_));

		if (values_.size() > 0) {
			// Needs to fetch the values from the value set.
			std::vector<T> value_vec(values_.begin(), values_.end());
			std::vector<T> final_vec;
			for (int i = 0; i < size_; ++i)
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
			throw error_internal(
				"could not generate instance matching predicate");
	}
};

/*
 * Sequence random operations.
 */

namespace sequence_op {

// Shuffles an sequence.
template <typename INST> INST shuffle(const INST &inst) {
	INST new_inst = inst;
	tgen::shuffle(new_inst.vec_);
	return new_inst;
}

// Choses any value in the sequence.
template <typename INST> typename INST::value_type any(const INST &inst) {
	return inst.vec_[next<int>(0, inst.vec_.size() - 1)];
}

// Chooses k values from the sequence, as in a subsequence of size k.
template <typename INST> INST choose(int k, const INST &inst) {
	tgen_ensure(0 < k and k <= inst.vec.size(),
				"number of elements to choose must be valid");
	std::vector<typename INST::value_type> new_vec;
	int need = k;
	for (int i = 0; need > 0; ++i) {
		int left = inst.vec_.size() - i;
		if (next(1, left) <= need) {
			new_vec.push_back(inst.vec_[i]);
			need--;
		}
	}
	return INST(new_vec);
}

}; // namespace sequence_op

}; // namespace tgen

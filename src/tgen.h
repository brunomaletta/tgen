#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
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

inline void throw_assertion_error_internal(const std::string &condition,
										   const std::string &msg) {
	throw std::runtime_error("tgen: " + msg + " (assertion `" + condition +
							 "` failed)");
}
inline void throw_assertion_error_internal(const std::string &condition) {
	throw std::runtime_error("tgen: assertion `" + condition + "` failed");
}
inline std::runtime_error error_internal(const std::string &msg) {
	return std::runtime_error("tgen: " + msg);
}
inline void contradiction_error_internal(const std::string &type,
										 const std::string &msg = "") {
	// Tried to generate a contradicting sequence.
	std::string error_msg = "invalid " + type + " (contradicting constraints)";
	if (!msg.empty())
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

inline std::mt19937 rng_internal;

// Returns a random number in [l, r].
template <typename T> T next(T l, T r) {
	tgen_ensure(l <= r, "range for `next` bust be valid");
	if constexpr (std::is_integral_v<T>)
		return std::uniform_int_distribution<T>(l, r)(rng_internal);
	else if constexpr (std::is_floating_point_v<T>)
		return std::uniform_real_distribution<T>(l, r)(rng_internal);
	else
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

// Shuffles container uniformly.
template <typename C> [[nodiscard]] C shuffle(const C &container) {
	auto new_container = container;
	shuffle(new_container.begin(), new_container.end());
	return new_container;
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

// Base struct for generators.
template <typename GEN> struct gen_base {
	// Calls the generator until predicate is true.
	template <typename PRED, typename... Args>
	auto gen_until(PRED predicate, int max_tries, Args &&...args) {

		for (int i = 0; i < max_tries; ++i) {
			auto inst =
				static_cast<GEN *>(this)->gen(std::forward<Args>(args)...);

			if (predicate(inst))
				return inst;
		}

		throw error_internal("could not generate instance matching predicate");
	}
};

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

inline std::vector<std::string>
	pos_opts_internal; // Dictionary containing the positional parsed opts.
inline std::map<std::string, std::string>
	named_opts_internal; // Global dictionary the named parsed opts.

// Returns true if there is an opt at a given index.
inline bool has_opt(int index) {
	return 0 <= index and index < pos_opts_internal.size();
}

// Returns true if there is an opt with a given key.
inline bool has_opt(const std::string &key) {
	return named_opts_internal.count(key) != 0;
}

template <typename T> T get_opt_internal(const std::string &value) {
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

	throw error_internal("invalid value `" + value + "` for type " +
						 typeid(T).name());
}

// Returns the parsed opt by a given key. If no opts with the given key are
// found, returns the given default_value.
template <typename T, typename KEY>
T opt(const KEY &key, std::optional<T> default_value = std::nullopt) {
	if constexpr (std::is_same_v<KEY, int>) {
		if (!has_opt(key)) {
			if (default_value)
				return *default_value;
			throw error_internal("cannot find key with index " +
								 std::to_string(key));
		}
		return get_opt_internal<T>(pos_opts_internal[key]);
	} else { // std::string
		if (!has_opt(key)) {
			if (default_value)
				return *default_value;
			throw error_internal("cannot find key with key " +
								 std::string(key));
		}
		return get_opt_internal<T>(named_opts_internal[key]);
	}
}

inline void parse_opts_internal(int argc, char **argv) {
	// Parses the opts into `pos_opts_internal` vector and `named_opts_internal`
	// map. Starting from 1 to ignore the name of the executable.
	for (int i = 1; i < argc; i++) {
		std::string key(argv[i]);

		if (key[0] == '-') {
			tgen_ensure(key.size() > 1,
						"invalid opt (" + std::string(argv[i]) + ")");
			if ('0' <= key[1] and key[1] <= '9') {
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
			tgen_ensure(!key.empty() and !value.empty(),
						"expected non-empty key/value in opt (" +
							std::string(argv[1]));
			tgen_ensure(named_opts_internal.count(key) == 0,
						"cannot have repeated keys");
			named_opts_internal[key] = value;
		} else {
			// This is the '--key value' case.
			tgen_ensure(named_opts_internal.count(key) == 0,
						"cannot have repeated keys");
			tgen_ensure(argv[i + 1], "value cannot be empty");
			named_opts_internal[key] = std::string(argv[i + 1]);
			i++;
		}
	}
}
inline void set_seed_internal(int argc, char **argv) {
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
}

// Registers generator by initializing rnd and parsing opts.
inline void register_gen(int argc, char **argv) {
	set_seed_internal(argc, argv);

	pos_opts_internal.clear();
	named_opts_internal.clear();
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

template <typename T> struct sequence : gen_base<sequence<T>> {
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

	// Creates generator for sequences of size 'size', with random T in [l, r].
	sequence(int size, T value_l, T value_r)
		: size_(size), value_l_(value_l), value_r_(value_r), neigh_(size) {
		tgen_ensure(size_ > 0, "size must be positive");
		tgen_ensure(value_l_ <= value_r_, "value range must be valid");
		for (int i = 0; i < size_; ++i)
			val_range_.emplace_back(value_l_, value_r_);
	}

	// Creates sequence with value set.
	sequence(int size, const std::set<T> &values)
		: size_(size), values_(values), neigh_(size) {
		tgen_ensure(size_ > 0, "size must be positive");
		tgen_ensure(!values.empty(), "value set must be non-empty");
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

	// Restricts sequences for sequence[idx_1] != sequence[idx_2].
	sequence &different(int idx_1, int idx_2) {
		std::set<int> indices = {idx_1, idx_2};
		return distinct(indices);
	}

	// Restricts sequences with distinct elements.
	sequence &distinct() {
		std::set<int> indices;
		for (int i = 0; i < size_; ++i)
			indices.insert(i);
		return distinct(indices);
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

		// Sorts values in non-decreasing order.
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
	generate_distinct_values(int k, const std::set<T> &forbidden_values) {
		for (auto forbidden : forbidden_values)
			tgen_ensure(value_l_ <= forbidden and forbidden <= value_r_);
		// We generate our numbers in the range [0, num_available) with
		// num_available = (r-l+1)-(forbidden_values.size()), and then map them
		// to the correct range. We will run k steps of Fisher–Yates, using a
		// map to store a virtual sequence that starts with a[i] = i.
		T num_available = (value_r_ - value_l_ + 1) - forbidden_values.size();
		if (num_available < k)
			throw error_internal(
				"failed to generate sequence: complex constraints");
		std::map<T, T> virtual_list;
		std::vector<T> list;
		for (int i = 0; i < k; i++) {
			T j = next<T>(i, num_available - 1);
			T vj = virtual_list.count(j) ? virtual_list[j] : j;
			T vi = virtual_list.count(i) ? virtual_list[i] : i;

			virtual_list[j] = vi, virtual_list[i] = vj;

			list.push_back(virtual_list[i]);
		}

		// Shifts back to correct range, but there might still be values
		// that we can not use.
		for (T &value : list)
			value += value_l_;

		// Now for every generated value, we shift it by how many forbidden
		// values are <= to it.
		std::vector<std::pair<T, int>> values_sorted;
		for (int i = 0; i < list.size(); ++i)
			values_sorted.emplace_back(list[i], i);
		// We iterate through them in increasing order.
		std::sort(values_sorted.begin(), values_sorted.end());
		auto cur_it = forbidden_values.begin();
		int smaller_forbidden_count = 0;
		for (auto [val, idx] : values_sorted) {
			while (cur_it != forbidden_values.end() and
				   *cur_it <= val + smaller_forbidden_count)
				++cur_it, ++smaller_forbidden_count;
			list[idx] += smaller_forbidden_count;
		}

		return list;
	}

	// Generates sequence instance.
	instance gen() {
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
			while (!q.empty()) {
				auto [cur_distinct, parent] = q.front();
				q.pop();

				std::set<int> neigh_distinct;
				for (int idx : distinct_constraints_[cur_distinct])
					for (int nxt_distinct :
						 distinct_containing_comp_idx[comp_id[idx]]) {
						if (nxt_distinct == cur_distinct or
							nxt_distinct == parent)
							continue;

						// Cycle found.
						if (vis_distinct[nxt_distinct])
							throw error_internal("failed to generate sequence: "
												 "complex constraints");

						neigh_distinct.insert(nxt_distinct);
					}

				for (int nxt_distinct : neigh_distinct) {
					vis_distinct[nxt_distinct] = true;
					q.emplace(nxt_distinct, cur_distinct);

					// Generates this distinct constraint.
					std::set<T> nxt_defined_values;
					for (int idx2 : distinct_constraints_[nxt_distinct])
						if (defined_idx[idx2]) {
							// There can not be any more defined. This case is
							// when there are values not coverered by a single
							// distinct constraint in the tree.
							if (initially_defined_comp_idx[comp_id[idx2]])
								throw error_internal(
									"failed to generate sequence: "
									"complex constraints");

							nxt_defined_values.insert(vec[idx2]);
						}
					int new_value_count =
						distinct_constraints_[nxt_distinct].size() -
						int(nxt_defined_values.size());
					std::vector<T> generated_values = generate_distinct_values(
						new_value_count, nxt_defined_values);
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

		if (!values_.empty()) {
			// Needs to fetch the values from the value set.
			std::vector<T> value_vec(values_.begin(), values_.end());
			for (T &val : vec)
				val = value_vec[val];
		}

		return instance(vec);
	}
};

/*
 * Sequence random operations.
 */

namespace sequence_op {

// Shuffles a sequence.
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
	tgen_ensure(0 < k and k <= inst.vec_.size(),
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
	int size_;							   // Size of permutation.
	std::vector<std::pair<int, int>> sets; // {idx, value}.

	// Creates generator for permutation of size 'size'.
	permutation(int size) : size_(size) {
		tgen_ensure(size_ > 0, "size must be positive");
	}

	// Restricts sequences for permutation[idx] = value.
	permutation &set(int idx, int value) {
		tgen_ensure(0 <= idx and idx < size_, "index must be valid");
		sets.emplace_back(idx, value);
		return *this;
	}

	// Permutation instance.
	// Operations on an instance are not random.
	struct instance {
		std::vector<int> vec_; // Permutation.
		bool add_1_;		   // If should add 1, for printing.

		instance(const std::vector<int> &vec) : vec_(vec), add_1_(false) {
			tgen_ensure(!vec_.empty(), "permutation cannot be empty");
			std::vector<bool> vis(vec_.size(), false);
			for (int i = 0; i < vec_.size(); i++) {
				tgen_ensure(0 <= vec_[i] and vec_[i] < vec_.size(),
							"permutation values must be from `0` to `size-1`");
				tgen_ensure(!vis[vec_[i]],
							"cannot have repeated values in permutation");
				vis[vec_[i]] = true;
			}
		}
		instance(const std::initializer_list<int> &list)
			: instance(std::vector<int>(list.begin(), list.end())) {}

		// Fetches size.
		std::size_t size() const { return vec_.size(); }

		// Fetches position idx.
		int &operator[](int idx) { return vec_[idx]; }
		const int &operator[](int idx) const { return vec_[idx]; }

		// Sorts values in increasign order.
		instance &sort() {
			std::sort(vec_.begin(), vec_.end());
			return *this;
		}

		// Reverses permutation.
		instance &reverse() {
			std::reverse(vec_.begin(), vec_.end());
			return *this;
		}

		// Inverse of the permutation.
		instance &inverse() {
			std::vector<int> inv(vec_.size());
			for (int i = 0; i < vec_.size(); ++i)
				inv[vec_[i]] = i;
			swap(vec_, inv);
			return *this;
		}

		// Sets that should print values 1-based.
		instance &add_1() {
			add_1_ = true;
			return *this;
		}

		// Prints in stdout, separated by spaces.
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
	instance gen() {
		sequence<int> seq(size_, 0, size_ - 1);
		seq.distinct();
		for (auto [idx, val] : sets)
			seq.set(idx, val);
		return instance(seq.gen().to_std());
	}

	// Generates permutation instance, given cycle sizes.
	instance gen(const std::vector<int> &cycle_sizes) {
		tgen_ensure(
			size_ == std::accumulate(cycle_sizes.begin(), cycle_sizes.end(), 0),
			"cycle sizes must add up to size of permutation");
		tgen_ensure(
			sets.empty(),
			"cannot generate permutation with set values and cycle sizes");

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
	instance gen(const std::initializer_list<int> &cycle_sizes) {
		return gen(std::vector<int>(cycle_sizes.begin(), cycle_sizes.end()));
	}
};

}; // namespace tgen

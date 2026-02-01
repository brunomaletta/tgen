#pragma once

#include "tgen.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

namespace tgen {

// Tried to generate a contradicting sequence.
void __sequence_contradiction_error(const std::string &msg = "") {
	std::string error_msg = "invalid sequence (contradicting constraints)";
	if (msg.size() > 0)
		error_msg += ": " + msg;
	throw __error(error_msg);
}

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
	sequence(int size_, T value_l_, T value_r_)
		: size(size_), value_l(value_l_), value_r(value_r_), neigh(size) {
		ensure(value_l <= value_r, "value range must be valid");
		for (int i = 0; i < size; ++i)
			val_range.emplace_back(value_l, value_r);
	}

	// Creates sequence with value set.
	sequence(int size_, const std::set<T> &values_)
		: size(size_), values(values_), neigh(size) {
		ensure(values.size() > 0, "must have at least one value");
		value_l = 0, value_r = values.size() - 1;
		for (int i = 0; i < size; ++i)
			val_range.emplace_back(value_l, value_r);
		int idx = 0;
		for (T value : values)
			value_idx_in_set[value] = idx++;
	}
	sequence(int size_, const std::vector<T> &values_)
		: sequence(size_, std::set<T>(values_.begin(), values_.end())) {}
	sequence(int size_, const std::initializer_list<T> &values_)
		: sequence(size_, std::set<T>(values_.begin(), values_.end())) {}

	// Restricts sequences for sequence[idx] = value.
	sequence &value_at_idx(int idx, T value) {
		ensure(0 <= idx and idx < size, "index must be valid");
		if (values.size() == 0) {
			auto &[left, right] = val_range[idx];
			ensure(left <= value and value <= right,
				   "value must be in the defined range");
			left = right = value;
		} else {
			ensure(values.count(value), "value must be in the set of values");
			auto &[left, right] = val_range[idx];
			int new_val = value_idx_in_set[value];
			ensure(left <= new_val and new_val <= right,
				   "must not set to two different values");
			left = right = new_val;
		}
		return *this;
	}

	// Restricts sequences for sequence[idx_1] = sequence[idx_2].
	sequence &equal_idx_pair(int idx_1, int idx_2) {
		ensure(0 <= std::min(idx_1, idx_2) and std::max(idx_1, idx_2) < size,
			   "indices must be valid");
		if (idx_1 == idx_2)
			return *this;

		neigh[idx_1].push_back(idx_2);
		neigh[idx_2].push_back(idx_1);
		return *this;
	}

	// Restricts sequences for sequence[left..right] to have all equal values.
	sequence &equal_range(int left, int right) {
		ensure(0 <= left and left <= right and right < size,
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

		instance(const std::vector<T> &vec_) : vec(vec_) {}

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
										const instance &repr) {
			for (int i = 0; i < repr.vec.size(); ++i) {
				if (i > 0)
					out << ' ';
				out << repr.vec[i];
			}
			return out;
		}

		// Gets a std::vector representing the instance.
		std::vector<T> stdvec() { return vec; }
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
							__sequence_contradiction_error(ss.str());
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
				__sequence_contradiction_error(
					"tried to generate " + std::to_string(distinct.size()) +
					" distinct values, but the maximum is " +
					std::to_string(value_r - value_l + 1));

			// Checks if two values in same component are marked as different.
			std::set<int> comp_ids;
			for (int idx : distinct) {
				if (comp_ids.count(comp_id[idx]))
					__sequence_contradiction_error(
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
	instance gen_until(int max_tries, PRED predicate,
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
	ensure(0 < k and k <= inst.vec.size(),
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

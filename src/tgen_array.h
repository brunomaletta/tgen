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

// Tried to generate a contradicting array.
void __array_contradiction_error(const std::string &msg = "") {
	std::string error_msg = "invalid array (contradicting constraints)";
	if (msg.size() > 0)
		error_msg += ": " + msg;
	throw __error(error_msg);
}

/*
 * Array generator.
 */
template <typename T> struct array_gen {
	int size;								// size of array.
	T l, r;									// range of defined values.
	std::vector<std::pair<T, T>> val_range; // range of values of each index.
	std::vector<std::vector<int>> neigh;	// adjacency list of equality.
	std::set<int>
		idx_distinct_constraints; // indices in some distinction constraint.
	std::vector<std::set<int>>
		distinct_constraints; // all distinct constraints.

	// Creates generator for arrays of size 'size', with random T in [l, r]
	array_gen(int size_, T l_, T r_) : size(size_), l(l_), r(r_), neigh(size) {
		ensure(l <= r, "value range must be valid");
		for (int i = 0; i < size; ++i)
			val_range.emplace_back(l, r);
	}

	// Restricts arrays for array[idx] = value.
	array_gen &value_at_idx(int idx, T value) {
		ensure(0 <= idx and idx < size, "index must be valid");
		auto &[l, r] = val_range[idx];
		ensure(l <= value and value <= r, "must have valid range intersection");
		l = r = value;
		return *this;
	}

	// Restricts arrays for array[idx_1] = array[idx_2].
	array_gen &equal_idx_pair(int idx_1, int idx_2) {
		ensure(0 <= std::min(idx_1, idx_2) and std::max(idx_1, idx_2) < size,
			   "indices must be valid");
		if (idx_1 == idx_2)
			return *this;

		neigh[idx_1].push_back(idx_2);
		neigh[idx_2].push_back(idx_1);
		return *this;
	}

	// Restricts arrays for array[left..right] to have all equal values.
	array_gen &equal_range(int left, int right) {
		ensure(0 <= left and left <= right and right < size,
			   "range indices bust be valid");
		for (int i = left; i < right; ++i)
			equal_idx_pair(i, i + 1);
		return *this;
	}

	// Restricts arrays for array[S] to be distinct, for given subset S of
	// indices.
	// You can not add two of these restrictions with intersection.
	array_gen &distinct_idx_set(const std::set<int> &indices) {
		for (int idx : indices) {
			if (idx_distinct_constraints.count(idx))
				throw __error(
					"cannot add same index in two distinct constraints");
			idx_distinct_constraints.insert(idx);
		}

		distinct_constraints.push_back(indices);
		return *this;
	}

	// Restricts arrays for array[idx_1] != array[idx_2]
	array_gen &different_idx_pair(int idx_1, int idx_2) {
		std::set<int> indices = {idx_1, idx_2};
		distinct_idx_set(indices);
		return *this;
	}

	// Restricts arrays with distinct elements.
	array_gen &distinct() {
		std::set<int> indices;
		for (int i = 0; i < size; ++i)
			indices.insert(i);
		distinct_idx_set(indices);
		return *this;
	}

	// Array instance.
	struct instance {
		std::vector<T> vec;

		instance(const std::vector<T> &vec_) : vec(vec_) {}

		// Shuffles the values.
		instance &shuffle() {
			tgen::shuffle(vec.begin(), vec.end());
			return *this;
		}

		// Sorts values in increasign order.
		instance &sort() {
			std::sort(vec.begin(), vec.end());
			return *this;
		}

		// Reverses array.
		instance &reverse() {
			std::reverse(vec.begin(), vec.end());
			return *this;
		}

		// Chooses 'k' elements from the array, without repetition.
		// Maintains the relative order.
		instance &choose(int k) {
			ensure(0 < k and k <= vec.size(),
				   "number of elements to choose must be valid");
			std::vector<T> new_vec;
			int need = k;
			for (int i = 0; need > 0; ++i) {
				int left = vec.size() - i;
				if (next(1, left) <= need) {
					new_vec.push_back(vec[i]);
					need--;
				}
			}
			swap(vec, new_vec);
			return *this;
		}

		// Prints in stdout, with space as separator.
		instance &print(std::string sep = " ", std::string end = "\n") {
			for (int i = 0; i < vec.size(); ++i) {
				if (i > 0)
					std::cout << sep;
				std::cout << vec[i];
			}
			std::cout << end;
			return *this;
		}

		// Gets a std::vector representing the instance.
		std::vector<int> operator()() { return vec; }
	};

	// Generate array instance.
	instance operator()() {
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
							__array_contradiction_error(ss.str());
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
			if (distinct.size() > r - l + 1)
				__array_contradiction_error(
					"tried to generate " + std::to_string(distinct.size()) +
					" distinct values, but the maximum is " +
					std::to_string(r - l + 1));

			// Checks if two values in same component are marked as different.
			std::set<int> comp_ids;
			for (int idx : distinct) {
				if (comp_ids.count(comp_id[idx]))
					__array_contradiction_error(
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
			// Fisher–Yates, using a map to store a virtual array that starts
			// with the a[i] = i.
			int lim = (r - l + 1) - int(defined_values.size());
			std::map<T, T> virtual_array;
			std::vector<T> initial_gen;
			for (int i = 0; i < distinct.size() - int(defined_values.size());
				 i++) {
				T j = next<T>(i, lim - 1);
				T vj = virtual_array.count(j) ? virtual_array[j] : j;
				T vi = virtual_array.count(i) ? virtual_array[i] : T(i);

				virtual_array[j] = vi, virtual_array[i] = vj;

				initial_gen.push_back(virtual_array[i]);
			}

			// Shifts back to correct range, but there might still be values
			// that we can not use.
			for (T &value : initial_gen)
				value += l;

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
				initial_gen[idx] = l + lim + cur_defined_idx;
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

		return instance(vec);
	}
};
}; // namespace tgen

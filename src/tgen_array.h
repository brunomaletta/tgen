#pragma once

#include "tgen.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <utility>
#include <vector>

namespace tgen {

/*
 * Array generator.
 */
template <typename T> struct array_gen {
	int __size;
	std::vector<std::pair<T, T>> val_range;
	std::vector<std::vector<int>> neigh;

	// Creates generator for arrays of size 'size', with random T in [l, r]
	array_gen(int size, T l, T r) : __size(size), neigh(size) {
		ensure(l <= r, "array value range must be valid");
		for (int i = 0; i < size; ++i)
			val_range.emplace_back(l, r);
	}

	// Restricts arrays for array[idx] = value.
	array_gen &set_value_at_idx(int idx, T value) {
		auto &[l, r] = val_range[idx];
		ensure(l <= value and value <= r, "must have valid range intersection");
		l = r = value;
		return *this;
	}

	// Restricts arrays for array[idx_1] = array[idx_2].
	array_gen &set_equal_idx(int idx_1, int idx_2) {
		ensure(0 <= std::min(idx_1, idx_2) and std::max(idx_1, idx_2) < __size,
			   "indices must be valid");
		if (idx_1 == idx_2)
			return *this;

		neigh[idx_1].push_back(idx_2);
		neigh[idx_2].push_back(idx_1);
		return *this;
	}

	// Restricts arrays for array[left..right] is a palindrome.
	array_gen &set_palindromic_substring(int left, int right) {
		ensure(left <= right, "substring indices bust be valid");
		for (int i = left; right - (i - left) > left; i++)
			set_equal_idx(i, right - (i - left));
		return *this;
	}

	// Array tnstance.
	struct instance {
		std::vector<T> __vec;

		instance(const std::vector<T> &vec) : __vec(vec) {}

		// Sorts values in increasign order.
		instance &sort() {
			std::sort(__vec.begin(), __vec.end());
			return *this;
		}

		// Reverses array.
		instance &reverse() {
			std::reverse(__vec.begin(), __vec.end());
			return *this;
		}

		// Prints in stdout, with space as separator.
		instance &print() {
			for (int i = 0; i < __vec.size(); ++i) {
				if (i > 0)
					std::cout << " ";
				std::cout << __vec[i];
			}
			std::cout << std::endl;
			return *this;
		}
	};

	// Generate array instance.
	instance operator()() {
		std::vector<T> vec(__size);

		std::vector<bool> vis(__size, false);
		for (int i = 0; i < __size; ++i)
			if (!vis[i]) {
				T new_value;
				bool value_set = false;

				// BFS to visit the connected component.
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
						} else if (new_value != l)
							// We found a contradiction
							throw __error(
								"invalid array (contradicting constraints)");
					}

					for (int k : neigh[j]) {
						if (!vis[k]) {
							vis[k] = true;
							q.push(k);
						}
					}
				}

				// Chooses value now if not set.
				if (!value_set)
					new_value =
						val_range[i].first +
						next(0, val_range[i].second - val_range[i].first + 1);

				for (int j : component)
					vec[j] = new_value;
			}

		return instance(vec);
	}
};
}; // namespace tgen

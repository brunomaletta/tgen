#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace {

template <typename Tree> bool is_tree_like(const Tree &t) {
	if (t.n() <= 0)
		return false;
	if (static_cast<int>(t.edges().size()) != t.n() - 1)
		return false;
	std::vector<bool> vis(t.n(), false);
	std::queue<int> q;
	q.push(0);
	vis[0] = true;
	int seen = 0;
	while (!q.empty()) {
		int u = q.front();
		q.pop();
		++seen;
		for (int v : t.adj()[u]) {
			if (!vis[v]) {
				vis[v] = true;
				q.push(v);
			}
		}
	}
	return seen == t.n();
}

bool is_tree_shape(const tgen::tree::value &t) { return is_tree_like(t); }

// Unrooted tree isomorphism via centroid decomposition and multiset hashing.
// `mphash` must be shared across both trees when comparing for isomorphism;
// otherwise numeric ids depend on DFS discovery order and isomorphic trees can
// disagree.
struct tree_iso_hash {
	std::map<std::vector<int>, int> &mphash;
	int n;
	std::vector<std::set<int>> g;
	std::vector<int> sz;
	std::vector<int> cs;

	tree_iso_hash(std::map<std::vector<int>, int> &mph,
				  const std::vector<std::set<int>> &adj)
		: mphash(mph), n(static_cast<int>(adj.size())), g(adj), sz(n) {}

	void dfs_centroid(int u, int p) {
		sz[u] = 1;
		bool cent = true;
		for (int v : g[u]) {
			if (v != p) {
				dfs_centroid(v, u);
				sz[u] += sz[v];
				if (sz[v] > n / 2)
					cent = false;
			}
		}
		if (cent && n - sz[u] <= n / 2)
			cs.push_back(u);
	}

	int fhash(int u, int p) {
		std::vector<int> h;
		for (int v : g[u]) {
			if (v != p)
				h.push_back(fhash(v, u));
		}
		std::sort(h.begin(), h.end());
		if (!mphash.count(h))
			mphash[h] = mphash.size();
		return mphash[h];
	}

	long long tree_hash() {
		cs.clear();
		dfs_centroid(0, -1);
		if (cs.size() == 1)
			return fhash(cs[0], -1);
		int h1 = fhash(cs[0], cs[1]);
		int h2 = fhash(cs[1], cs[0]);
		if (h1 > h2)
			std::swap(h1, h2);
		return (static_cast<long long>(h1) << 30) + h2;
	}
};

bool trees_isomorphic(const std::vector<std::set<int>> &adj_1,
					  const std::vector<std::set<int>> &adj_b) {
	if (adj_1.size() != adj_b.size())
		return false;
	if (adj_1.empty())
		return true;
	std::map<std::vector<int>, int> mphash;
	return tree_iso_hash(mphash, adj_1).tree_hash() ==
		   tree_iso_hash(mphash, adj_b).tree_hash();
}

} // namespace

TEST(tree_test, constructor_non_positive_n) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::tree(0), "wtree: n must be positive");
}

TEST(tree_test, gen_is_labeled_tree) {
	tgen::register_gen();

	for (int n = 1; n <= 12; ++n)
		for (int it = 0; it < 30; ++it)
			EXPECT_TRUE(is_tree_shape(tgen::tree(n).gen()));
}

TEST(tree_test, gen_with_preset_edges) {
	tgen::register_gen();

	for (int it = 0; it < 20; ++it) {
		auto t = tgen::tree(6).add_edge(0, 1).add_edge(2, 3).gen();
		EXPECT_TRUE(is_tree_shape(t));
		EXPECT_TRUE(std::find(t.edges().begin(), t.edges().end(),
							  std::pair(0, 1)) != t.edges().end());
		EXPECT_TRUE(std::find(t.edges().begin(), t.edges().end(),
							  std::pair(2, 3)) != t.edges().end());
	}
}

TEST(tree_test, gen_skewed_is_tree) {
	tgen::register_gen();

	for (int n = 2; n <= 15; ++n) {
		for (int el = -5; el <= 5; ++el) {
			EXPECT_TRUE(is_tree_shape(tgen::tree::gen_skewed(n, el)));
			EXPECT_TRUE(
				is_tree_shape(tgen::wtree<int, int>::gen_skewed(n, el)));
		}
	}
}

TEST(tree_test, value_constructor_rejects_cycle) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::tree::value(3,
											   {
												   {0, 1},
												   {1, 2},
												   {0, 2},
											   }),
							 "wtree: value: initial edges must form a tree");
}

TEST(tree_test, value_from_adjacency_list) {
	tgen::register_gen();

	std::vector<std::set<int>> a(3);
	a[0] = {1};
	a[1] = {0, 2};
	a[2] = {1};
	tgen::tree::value t(a);

	EXPECT_EQ(t.n(), 3);
	EXPECT_TRUE(is_tree_shape(t));
	EXPECT_EQ(t.edges(), (std::vector<std::pair<int, int>>({{0, 1}, {1, 2}})));
}

TEST(tree_test, link_composes_trees) {
	tgen::register_gen();

	for (int it = 0; it < 20; ++it) {
		auto a = tgen::tree(4).gen();
		auto b = tgen::tree(5).gen();
		tgen::tree::value c = a;
		c.link(b, 1, 2);

		EXPECT_TRUE(is_tree_shape(c));
		EXPECT_EQ(c.n(), 9);
		EXPECT_TRUE(std::find(c.edges().begin(), c.edges().end(),
							  std::pair(1, a.n() + 2)) != c.edges().end());
	}
}

TEST(tree_test, shuffle_preserves_tree) {
	tgen::register_gen();

	for (int it = 0; it < 15; ++it) {
		auto t = tgen::tree(10).gen();
		auto adj_before = t.adj();
		t.shuffle();

		EXPECT_TRUE(is_tree_shape(t));
		EXPECT_TRUE(trees_isomorphic(adj_before, t.adj()));
	}
}

TEST(tree_test, shuffle_except_fixed_labels_and_isomorphism) {
	tgen::register_gen();

	// Shuffles all but 0.
	for (int it = 0; it < 20; ++it) {
		auto t = tgen::tree(12).gen();
		int deg0 = t.adj()[0].size();
		auto adj_before = t.adj();
		t.shuffle_except({0});

		EXPECT_EQ(t.adj()[0].size(), deg0);
		EXPECT_TRUE(trees_isomorphic(adj_before, t.adj()));
	}

	// Don't shuffle any vertex.
	for (int n = 2; n <= 10; ++n) {
		auto t = tgen::tree(n).gen();
		auto adj_before = t;
		std::set<int> all;
		for (int i = 0; i < n; ++i)
			all.insert(i);
		adj_before.shuffle_except(all);

		EXPECT_EQ(t.adj(), adj_before.adj());
	}
}

TEST(tree_test, set_vertex_weights) {
	tgen::register_gen();

	auto t = tgen::tree(5).gen();
	auto w = t.set_vertex_weights({10, 20, 30, 40, 50});

	EXPECT_TRUE(is_tree_shape(w));
	EXPECT_EQ(t.adj(), w.adj());
	EXPECT_TRUE(w.vertex_weights().has_value());
	EXPECT_EQ(*w.vertex_weights(), std::vector<int>({10, 20, 30, 40, 50}));
}

TEST(tree_test, add_unweighted_vertices_to_weighted_tree) {
	tgen::register_gen();

	auto t = tgen::tree(5).gen();
	auto w = t.set_vertex_weights(std::vector<int>{0, 1, 2, 3, 4});

	EXPECT_THROW_TGEN_PREFIX(w.add_vertices(10),
							 "wtree: value: cannot add unweighted vertices to "
							 "vertex-weighted tree");
}

TEST(tree_test, set_edge_weights) {
	tgen::register_gen();

	auto t = tgen::tree(4).gen();
	std::vector<int> ew(t.edges().size(), 7);
	auto w = t.set_edge_weights(ew);

	EXPECT_TRUE(is_tree_shape(w));
	EXPECT_EQ(t.adj(), w.adj());
	EXPECT_TRUE(w.edge_weights().has_value());
	EXPECT_EQ(*w.edge_weights(), ew);
}

TEST(tree_test, edge_weighted) {
	tgen::register_gen();

	tgen::etree<int>::value t(4, {});
	t.edge_weighted();
	t.add_edge(0, 1, 5);
	t.add_edge(1, 2, 6);
	t.add_edge(2, 3, 7);

	EXPECT_TRUE(is_tree_shape(t));
	EXPECT_TRUE(t.edge_weights().has_value());
	EXPECT_EQ(t.edges().size(), 3u);
	EXPECT_EQ((*t.edge_weights())[0], 5);
	EXPECT_EQ((*t.edge_weights())[2], 7);
}

TEST(tree_test, edge_weighted_invalid_nonempty) {
	tgen::register_gen();

	auto t = tgen::tree(4).gen();
	EXPECT_THROW_TGEN_PREFIX(
		t.edge_weighted(),
		"wtree: value: edge_weighted requires a tree with no "
		"edges");
}

TEST(tree_test, glue_composes_trees) {
	tgen::register_gen();

	for (int it = 0; it < 15; ++it) {
		auto a = tgen::tree(4).gen();
		auto b = tgen::tree(3).gen();
		int n_a = a.n();
		tgen::tree::value c = a.glue(b, {{1, 0}});

		EXPECT_TRUE(is_tree_shape(c));
		EXPECT_EQ(c.n(), n_a + b.n() - 1);
	}
}

TEST(tree_test, value_add_vertices_and_add_edge) {
	tgen::register_gen();

	tgen::tree::value t(3, std::vector<std::pair<int, int>>({{0, 1}, {1, 2}}));
	t.add_vertices(2);

	EXPECT_EQ(t.n(), 5);
	EXPECT_TRUE(!is_tree_shape(t));
	EXPECT_EQ(t.edges(), (std::vector<std::pair<int, int>>({{0, 1}, {1, 2}})));

	t.add_edge(0, 3);
	t.add_edge(3, 4);

	EXPECT_TRUE(is_tree_shape(t));
	EXPECT_EQ(t.edges(), (std::vector<std::pair<int, int>>(
							 {{0, 1}, {1, 2}, {0, 3}, {3, 4}})));
}

TEST(tree_test, print_edge_list) {
	tgen::register_gen();

	auto t = tgen::tree::value(5, {{0, 1}, {1, 2}, {0, 3}, {3, 4}});

	EXPECT_EQ((std::ostringstream() << t).str(), "0 1\n1 2\n0 3\n3 4\n");

	auto ta = t;
	EXPECT_EQ((std::ostringstream() << ta.add_1()).str(),
			  "1 2\n2 3\n1 4\n4 5\n");

	auto tn = t;
	EXPECT_EQ((std::ostringstream() << tn.print_n()).str(),
			  "5\n0 1\n1 2\n0 3\n3 4\n");

	auto tb = t;
	EXPECT_EQ((std::ostringstream() << tb.print_parents(-1)).str(),
			  "0 1 0 3\n");

	auto tc = t;
	EXPECT_EQ((std::ostringstream() << tc.add_1().print_parents(-1)).str(),
			  "1 2 1 4\n");

	auto td = t;
	EXPECT_EQ((std::ostringstream() << td.print_parents(0)).str(),
			  "-1 0 1 0 3\n");

	auto te = t;
	EXPECT_EQ((std::ostringstream() << te.add_1().print_parents(0)).str(),
			  "0 1 2 1 4\n");
}

TEST(tree_test, print_vertex_weights) {
	tgen::register_gen();

	auto t = tgen::tree::value(3, {{0, 1}, {1, 2}})
				 .set_vertex_weights(std::vector<int>({5, 6, 7}));

	EXPECT_EQ((std::ostringstream() << t).str(), "5 6 7\n0 1\n1 2\n");

	auto ta = t;
	EXPECT_EQ((std::ostringstream() << ta.add_1()).str(), "5 6 7\n1 2\n2 3\n");

	auto tb = t;
	EXPECT_EQ((std::ostringstream() << tb.print_parents(-1)).str(),
			  "5 6 7\n0 1\n");

	auto tc = t;
	EXPECT_EQ((std::ostringstream() << tc.add_1().print_parents(1)).str(),
			  "5 6 7\n2 0 2\n");
}

TEST(tree_test, print_weighted_edge_list) {
	tgen::register_gen();

	auto t = tgen::tree::value(3, {{0, 1}, {1, 2}})
				 .set_edge_weights(std::vector<int>({5, 6}));

	EXPECT_EQ((std::ostringstream() << t).str(), "0 1 5\n1 2 6\n");

	auto ta = t;
	EXPECT_EQ((std::ostringstream() << ta.add_1()).str(), "1 2 5\n2 3 6\n");
}

TEST(tree_test, to_std) {
	tgen::register_gen();

	auto t = tgen::tree(8).gen();
	auto p = t.to_std();
	EXPECT_EQ(p.first, t.n());
	EXPECT_EQ(p.second, t.adj());
}

TEST(tree_test, glue_cycle_fails) {
	tgen::register_gen();

	tgen::tree::value a(3, std::vector<std::pair<int, int>>({{0, 1}, {1, 2}}));
	tgen::tree::value b(3, std::vector<std::pair<int, int>>({{0, 1}, {1, 2}}));
	EXPECT_THROW_TGEN_PREFIX(
		a.glue(b, {{0, 0}, {2, 2}}),
		"wtree: value: added edge must not create a cycle");
}

TEST(tree_test, gen_uniform_labeled_trees) {
	tgen::register_gen();

	// Cayley: n^(n-2) labeled trees on n vertices.
	expect_generator_uniform(tgen::tree(4), 16);
	// For k fixed edges that form a single connected component, the number of
	// trees is (k+1) * n^(n-2-k).
	expect_generator_uniform(tgen::tree(5).add_edge(0, 1), 50);
}

TEST(tree_test, weight_type_aliases) {
	tgen::register_gen();

	EXPECT_TRUE((std::is_same_v<tgen::tree, tgen::wtree<int, int>>));
	EXPECT_TRUE((std::is_same_v<tgen::vtree<long>, tgen::wtree<long, int>>));
	EXPECT_TRUE(
		(std::is_same_v<tgen::etree<unsigned>, tgen::wtree<int, unsigned>>));
}

TEST(tree_test, vtree_gen_and_vertex_weights) {
	tgen::register_gen();

	auto t = tgen::vtree<double>(6).gen();

	EXPECT_TRUE(is_tree_like(t));

	std::vector<double> vw = {0.5, 1.5, 2.5, 3.5, 4.5, 5.5};
	auto w = t.set_vertex_weights(vw);

	ASSERT_TRUE(w.vertex_weights().has_value());
	EXPECT_EQ(*w.vertex_weights(), vw);
}

TEST(tree_test, etree_gen_and_edge_weights) {
	tgen::register_gen();

	auto t = tgen::etree<std::string>(5).gen();

	EXPECT_TRUE(is_tree_like(t));

	std::vector<std::string> ew(4, "xy");
	auto w = t.set_edge_weights(ew);

	ASSERT_TRUE(w.edge_weights().has_value());
	EXPECT_EQ(*w.edge_weights(), ew);
}

TEST(tree_test, wtree_both_weight_dimensions) {
	tgen::register_gen();

	auto t = tgen::wtree<bool, char>(4).gen();

	EXPECT_TRUE(is_tree_like(t));

	// Each setter requires an unweighted value; both dimensions are still
	// supported on the same wtree<V,E> template.
	std::vector<bool> vw = {true, false, true, false};
	std::vector<char> ew = {'a', 'b', 'c'};
	auto wv = t.set_vertex_weights(vw);
	auto we = t.set_edge_weights(ew);

	EXPECT_EQ(*wv.vertex_weights(), vw);
	EXPECT_EQ(*we.edge_weights(), ew);
}

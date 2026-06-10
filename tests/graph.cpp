#include <gtest/gtest.h>

#include "../single_include/tgen.h"
#include "tgen_test_utility.h"

#include <algorithm>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <type_traits>
#include <vector>

namespace {

template <typename Graph> bool is_connected_undirected(const Graph &g) {
	if (g.n() <= 0)
		return false;
	std::vector<bool> vis(g.n(), false);
	std::queue<int> q;
	q.push(0);
	vis[0] = true;
	int seen = 0;
	while (!q.empty()) {
		int u = q.front();
		q.pop();
		++seen;
		for (int v : g.adj()[u]) {
			if (!vis[v]) {
				vis[v] = true;
				q.push(v);
			}
		}
	}
	return seen == g.n();
}

template <typename Graph>
std::vector<std::vector<int>> connected_components(const Graph &g) {
	std::vector<bool> vis(g.n(), false);
	std::vector<std::vector<int>> components;
	std::queue<int> q;
	for (int s = 0; s < g.n(); ++s) {
		if (vis[s])
			continue;
		components.emplace_back();
		vis[s] = true;
		q.push(s);
		while (!q.empty()) {
			int u = q.front();
			q.pop();
			components.back().push_back(u);
			for (int v : g.adj()[u]) {
				if (!vis[v]) {
					vis[v] = true;
					q.push(v);
				}
			}
		}
	}
	return components;
}

template <typename Graph>
bool is_connected_on_vertices(const Graph &g,
							  const std::vector<int> &vertices) {
	if (vertices.size() <= 1)
		return true;
	std::vector<bool> in_comp(g.n(), false);
	for (int v : vertices)
		in_comp[v] = true;
	std::vector<bool> vis(g.n(), false);
	std::queue<int> q;
	q.push(vertices[0]);
	vis[vertices[0]] = true;
	int seen = 0;
	while (!q.empty()) {
		int u = q.front();
		q.pop();
		++seen;
		for (int v : g.adj()[u]) {
			if (in_comp[v] and !vis[v]) {
				vis[v] = true;
				q.push(v);
			}
		}
	}
	return seen == static_cast<int>(vertices.size());
}

bool is_bipartite(const tgen::graph::value &g) {
	std::vector<int> color(g.n(), -1);
	for (int s = 0; s < g.n(); ++s) {
		if (color[s] != -1)
			continue;
		color[s] = 0;
		std::queue<int> q;
		q.push(s);
		while (!q.empty()) {
			int u = q.front();
			q.pop();
			for (int v : g.adj()[u]) {
				if (color[v] == -1) {
					color[v] = color[u] ^ 1;
					q.push(v);
				} else if (color[v] == color[u])
					return false;
			}
		}
	}
	return true;
}

template <typename Graph> bool is_weakly_connected(const Graph &g, int root) {
	if (g.n() <= 0)
		return false;
	std::vector<bool> vis(g.n(), false);
	std::queue<int> q;
	q.push(root);
	vis[root] = true;
	int seen = 0;
	while (!q.empty()) {
		int u = q.front();
		q.pop();
		++seen;
		for (int v : g.adj()[u]) {
			if (!vis[v]) {
				vis[v] = true;
				q.push(v);
			}
		}
	}
	return seen == g.n();
}

bool is_dag(const tgen::graph::value &g) {
	if (!g.is_directed())
		return false;
	std::vector<int> indeg(g.n(), 0);
	for (auto [u, v] : g.edges()) {
		++indeg[v];
	}
	std::queue<int> q;
	for (int i = 0; i < g.n(); ++i)
		if (indeg[i] == 0)
			q.push(i);
	int cnt = 0;
	while (!q.empty()) {
		int u = q.front();
		q.pop();
		++cnt;
		for (int v : g.adj()[u]) {
			if (--indeg[v] == 0)
				q.push(v);
		}
	}
	return cnt == g.n();
}

long long max_simple_undirected_edges(int n) {
	return static_cast<long long>(n) * (n - 1) / 2;
}

// Maximum simple edges for n vertices given directed / self-loop mode
// (matches wgraph::gen pair restrictions).
long long max_graph_edges(int n, bool directed, bool self_loops) {
	if (n <= 0)
		return 0;
	if (directed)
		return self_loops ? static_cast<long long>(n) * n
						  : static_cast<long long>(n) * (n - 1);
	return self_loops ? static_cast<long long>(n) * (n + 1) / 2
					  : static_cast<long long>(n) * (n - 1) / 2;
}

// Brute-force isomorphism for small n (used to validate shuffle).
template <typename Graph>
bool graphs_isomorphic(const Graph &a, const Graph &b) {
	if (a.n() != b.n() or a.m() != b.m() or a.is_directed() != b.is_directed())
		return false;
	const int n = a.n();
	const bool directed = a.is_directed();

	std::set<std::pair<int, int>> eb(b.edges().begin(), b.edges().end());

	std::vector<int> p(n);
	std::iota(p.begin(), p.end(), 0);
	do {
		bool ok = true;
		for (auto [u, v] : a.edges()) {
			int pu = p[u], pv = p[v];
			if (!directed and pu > pv)
				std::swap(pu, pv);
			if (!eb.count({pu, pv})) {
				ok = false;
				break;
			}
		}
		if (ok)
			return true;
	} while (std::next_permutation(p.begin(), p.end()));
	return false;
}

// Every edge of `sub` appears in `super` (e.g. subgraph of a host graph).
template <typename Graph>
bool edges_subset_of_edge_list(const Graph &sub,
							   const std::set<std::pair<int, int>> &super) {
	for (const auto &e : sub.edges()) {
		if (!super.count(e))
			return false;
	}
	return true;
}

} // namespace

TEST(graph_test, constructor_non_positive_n) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::graph(0, 0),
							 "wgraph: number of vertices must be positive");
}

TEST(graph_test, gen_edge_count_and_simple_bounds) {
	tgen::register_gen();

	for (int n = 2; n <= 10; ++n) {
		for (int m = 0; m <= max_simple_undirected_edges(n); ++m) {
			auto g = tgen::graph(n, m).gen();
			EXPECT_TRUE((graph_gen_result_valid(g, n, m, false, false)));
		}
	}
}

TEST(graph_test, gen_with_preset_edges) {
	tgen::register_gen();

	for (int it = 0; it < 25; ++it) {
		auto g = tgen::graph(8, 14).add_edge(0, 1).add_edge(2, 3).gen();

		EXPECT_TRUE((graph_gen_result_valid(g, 8, 14, false, false)));
		EXPECT_TRUE((std::find(g.edges().begin(), g.edges().end(),
							   std::pair(0, 1)) != g.edges().end()));
		EXPECT_TRUE((std::find(g.edges().begin(), g.edges().end(),
							   std::pair(2, 3)) != g.edges().end()));
	}
}

TEST(graph_test, gen_impossible_edge_count) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::graph(4, 7).gen(),
							 "wgraph: not enough edges to generate");
}

TEST(graph_test, single_vertex_no_edges) {
	tgen::register_gen();

	auto g = tgen::graph(1, 0).gen();
	EXPECT_TRUE((graph_gen_result_valid(g, 1, 0, false, false)));
}

TEST(graph_test, single_vertex_positive_edges_impossible) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::graph(1, 1).gen(),
							 "wgraph: not enough edges to generate");
}

TEST(graph_test, get_connected) {
	tgen::register_gen();

	for (int it = 0; it < 40; ++it) {
		auto g = tgen::graph(12, 18).get_connected();
		EXPECT_TRUE((graph_gen_result_valid(g, 12, 18, false, false)));
		EXPECT_TRUE(is_connected_undirected(g));
	}
}

TEST(graph_test, get_connected_requires_enough_edges) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::graph(5, 2).get_connected(),
		"wgraph: connected graph needs at least n - 1 edges");
}

TEST(graph_test, get_acyclic_is_dag) {
	tgen::register_gen();

	for (int it = 0; it < 40; ++it) {
		auto g = tgen::graph(10, 14, true).get_acyclic();

		EXPECT_TRUE((graph_gen_result_valid(g, 10, 14, true, false)));
		EXPECT_TRUE(is_dag(g));
	}
}

TEST(graph_test, get_acyclic_cycle_in_preset) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(tgen::graph(3, 5, true)
								 .add_edge(0, 1)
								 .add_edge(1, 2)
								 .add_edge(2, 0)
								 .get_acyclic(),
							 "wgraph: preset edges contain a directed cycle");
}

TEST(graph_test, gen_skewed_connected) {
	tgen::register_gen();

	for (int it = 0; it < 25; ++it) {
		auto g = tgen::graph::gen_skewed(11, 14, 3, 6);

		EXPECT_TRUE((graph_gen_result_valid(g, 11, 14, false, false)));
		EXPECT_TRUE(is_connected_undirected(g));
	}
}

TEST(graph_test, gen_skewed_is_tree_when_m_is_n_minus_one) {
	tgen::register_gen();

	for (int n = 2; n <= 14; ++n) {
		for (int el = -4; el <= 4; ++el) {
			auto g = tgen::graph::gen_skewed(n, n - 1, el, 2);

			EXPECT_TRUE((graph_gen_result_valid(g, n, n - 1, false, false)));
			EXPECT_TRUE(is_connected_undirected(g));
		}
	}
}

TEST(graph_test, gen_skewed_directed_is_dag) {
	tgen::register_gen();

	for (int it = 0; it < 25; ++it) {
		auto g = tgen::graph::gen_skewed(11, 14, 3, 6, true);

		EXPECT_TRUE((graph_gen_result_valid(g, 11, 14, true, false)));
		EXPECT_TRUE(is_dag(g));
		EXPECT_TRUE(is_weakly_connected(g, 0));
	}
}

TEST(graph_test, gen_skewed_directed_tree_when_m_is_n_minus_one) {
	tgen::register_gen();

	for (int n = 2; n <= 14; ++n) {
		for (int el = -4; el <= 4; ++el) {
			auto g = tgen::graph::gen_skewed(n, n - 1, el, 2, true);

			EXPECT_TRUE((graph_gen_result_valid(g, n, n - 1, true, false)));
			EXPECT_TRUE(is_dag(g));
			EXPECT_TRUE(is_weakly_connected(g, 0));
		}
	}
}

TEST(graph_test, value_edge_list_dedupes_undirected) {
	tgen::register_gen();

	tgen::graph::value g(
		3, std::vector<std::pair<int, int>>({{0, 1}, {1, 0}, {1, 2}}));

	EXPECT_TRUE((graph_gen_result_valid(g, 3, 2, false, false)));
}

TEST(graph_test, value_from_adjacency_list) {
	tgen::register_gen();

	std::vector<std::set<int>> tri(3);
	tri[0] = {1, 2};
	tri[1] = {0, 2};
	tri[2] = {0, 1};
	tgen::graph::value gu(tri);

	EXPECT_TRUE((graph_gen_result_valid(gu, 3, 3, false, false)));

	std::vector<std::set<int>> dir(3);
	dir[0] = {1};
	tgen::graph::value gd(dir, true);

	EXPECT_TRUE((graph_gen_result_valid(gd, 3, 1, true, false)));
}

TEST(graph_test, gen_bipartite) {
	tgen::register_gen();

	// Presets m cross edges; if m <= n1*n2 they are distinct and fill the
	// budget, so gen() adds no further edges and the graph stays bipartite.
	for (int it = 0; it < 30; ++it) {
		const int n1 = 4, n2 = 5;
		std::vector<tgen::graph::value> gs = {
			tgen::graph::gen_bipartite(n1, n2, 11),
			tgen::wgraph<int, int>::gen_bipartite(n1, n2, 11),
		};
		for (const auto &g : gs) {
			EXPECT_TRUE((graph_gen_result_valid(g, n1 + n2, 11, false, false)));
			EXPECT_TRUE(is_bipartite(g));
		}
	}
}

TEST(graph_test, gen_bipartite_connected) {
	tgen::register_gen();

	// jngen-style connected bipartite: Prüfer tree for connectivity, then
	// cross-part rejection sampling (tree edges may be same-part).
	for (int it = 0; it < 30; ++it) {
		const int n1 = 4, n2 = 5, m = 12;
		auto g = tgen::graph::gen_bipartite(n1, n2, m, true);
		EXPECT_TRUE((graph_gen_result_valid(g, n1 + n2, m, false, false)));
		EXPECT_TRUE(is_connected_undirected(g));
	}
}

TEST(graph_test, gen_bipartite_connected_needs_enough_edges) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::graph::gen_bipartite(4, 5, 7, true),
		"wgraph: connected bipartite graph needs at least n1 + n2 - 1 edges");
}

TEST(graph_test, standard_graphs) {
	tgen::register_gen();

	{
		auto k4 = tgen::K(4);
		EXPECT_TRUE((graph_gen_result_valid(k4, 4, 6, false, false)));
		EXPECT_TRUE(is_connected_undirected(k4));

		for (auto &list : k4.adj())
			EXPECT_TRUE(static_cast<int>(list.size()) == k4.n() - 1);
	}
	{
		auto p = tgen::P(6);
		EXPECT_TRUE((graph_gen_result_valid(p, 6, 5, false, false)));
		EXPECT_TRUE(is_connected_undirected(p));

		EXPECT_TRUE(p.adj()[0].size() == 1 and p.adj()[5].size() == 1);
		for (int i = 1; i < p.n() - 1; ++i) {
			EXPECT_EQ(p.adj()[i], std::set<int>({i - 1, i + 1}));
		}
	}
	{
		auto p = tgen::P(6, true);
		EXPECT_TRUE((graph_gen_result_valid(p, 6, 5, true, false)));

		EXPECT_TRUE(p.adj()[0].size() == 1 and p.adj()[0].count(1));
		EXPECT_TRUE(p.adj()[5].empty());
		for (int i = 1; i < p.n() - 1; ++i) {
			EXPECT_EQ(p.adj()[i], std::set<int>({i + 1}));
		}
	}
	{
		auto c = tgen::C(5);
		EXPECT_TRUE((graph_gen_result_valid(c, 5, 5, false, false)));
		EXPECT_TRUE(is_connected_undirected(c));

		for (int i = 0; i < c.n(); ++i) {
			EXPECT_TRUE(c.adj()[i].size() == 2);
			EXPECT_TRUE(c.adj()[i].count((i + 1) % c.n()));
		}
	}
	{
		auto c = tgen::C(5, true);
		EXPECT_TRUE((graph_gen_result_valid(c, 5, 5, true, false)));

		for (int i = 0; i < c.n(); ++i) {
			EXPECT_EQ(c.adj()[i], std::set<int>({(i + 1) % c.n()}));
		}
	}
	{
		auto s = tgen::S(7);
		EXPECT_TRUE((graph_gen_result_valid(s, 7, 6, false, false)));
		EXPECT_TRUE(is_connected_undirected(s));

		EXPECT_TRUE(static_cast<int>(s.adj()[0].size()) == s.n() - 1);
		for (int i = 1; i < s.n(); ++i) {
			EXPECT_EQ(s.adj()[i], (std::set<int>({0})));
		}
	}
	{
		auto kb = tgen::K(3, 4);
		EXPECT_TRUE((graph_gen_result_valid(kb, 7, 12, false, false)));
		EXPECT_TRUE(is_bipartite(kb));
	}
}

TEST(graph_test, complement_full_simple_graph) {
	tgen::register_gen();

	auto g = tgen::graph(5, 0).gen();
	EXPECT_TRUE((graph_gen_result_valid(g, 5, 0, false, false)));
	g = !g;
	EXPECT_TRUE(is_connected_undirected(g));
	EXPECT_TRUE((graph_gen_result_valid(
		g, 5, static_cast<int>(max_simple_undirected_edges(5)), false, false)));
}

TEST(graph_test, complement_rejects_edge_weights) {
	tgen::register_gen();

	auto g = tgen::egraph<int>(3, 2).gen().set_edge_weights({1, 2});
	EXPECT_THROW_TGEN_PREFIX((!g),
							 "wgraph: value: cannot compute complement of "
							 "edge-weighted graph");
}

TEST(graph_test, random_subgraph_edge_count) {
	tgen::register_gen();

	auto g = tgen::graph(8, 20).gen();
	const std::set<std::pair<int, int>> orig(g.edges().begin(),
											 g.edges().end());
	g.random_subgraph(9);

	EXPECT_TRUE((edges_subset_of_edge_list(g, orig)));
	EXPECT_TRUE((graph_gen_result_valid(g, 8, 9, false, false)));
}

TEST(graph_test, random_connected_subgraph) {
	tgen::register_gen();

	auto g = tgen::graph(9, 24).get_connected();
	const std::set<std::pair<int, int>> orig(g.edges().begin(),
											 g.edges().end());
	g.random_connected_subgraph(15);

	EXPECT_TRUE((edges_subset_of_edge_list(g, orig)));
	EXPECT_TRUE(is_connected_undirected(g));
	EXPECT_TRUE((graph_gen_result_valid(g, 9, 15, false, false)));
}

TEST(graph_test, random_connected_subgraph_disconnected) {
	tgen::register_gen();

	auto g = tgen::graph(6, 10).gen();
	tgen::graph::value other = tgen::graph(6, 10).gen();
	g.disjoint_union(other);

	const auto components_before = connected_components(g);
	const int min_edges = g.n() - static_cast<int>(components_before.size());
	const int keep_edges = std::min(g.m(), min_edges + 2);
	const std::set<std::pair<int, int>> orig(g.edges().begin(),
											 g.edges().end());
	g.random_connected_subgraph(keep_edges);

	EXPECT_TRUE((edges_subset_of_edge_list(g, orig)));
	EXPECT_EQ(connected_components(g).size(), components_before.size());
	for (const auto &verts : components_before)
		EXPECT_TRUE(is_connected_on_vertices(g, verts));
	EXPECT_TRUE((graph_gen_result_valid(g, g.n(), keep_edges, false, false)));
}

TEST(graph_test, directed_gen_distinct_edges) {
	tgen::register_gen();

	auto g = tgen::graph(6, 10, true).gen();

	EXPECT_TRUE((graph_gen_result_valid(g, 6, 10, true, false)));
}

TEST(graph_test, link_composes_graphs) {
	tgen::register_gen();

	for (int it = 0; it < 20; ++it) {
		auto a = tgen::graph(4, 3).gen();
		auto b = tgen::graph(5, 4).gen();
		tgen::graph::value c = a;
		c.link(b, 1, 2);

		EXPECT_TRUE((std::find(c.edges().begin(), c.edges().end(),
							   std::pair(1, a.n() + 2)) != c.edges().end()));
		EXPECT_TRUE((graph_gen_result_valid(c, 9, 8, false, false)));
	}
}

TEST(graph_test, operator_plus_disjoint_union) {
	tgen::register_gen();

	auto a = tgen::graph(3, 1).gen();
	auto b = tgen::graph(2, 0).gen();
	auto c = a + b;
	EXPECT_TRUE((graph_gen_result_valid(c, 5, 1, false, false)));
}

TEST(graph_test, disjoint_union_shifts_rhs_vertices) {
	tgen::register_gen();

	tgen::graph::value left(2, {{0, 1}});
	tgen::graph::value right(2, {{0, 1}});
	left.disjoint_union(right);

	EXPECT_TRUE((graph_gen_result_valid(left, 4, 2, false, false)));
	const std::set<std::pair<int, int>> es(left.edges().begin(),
										   left.edges().end());
	EXPECT_TRUE((es == std::set<std::pair<int, int>>({{0, 1}, {2, 3}})));
}

TEST(graph_test, shuffle_preserves_isomorphism) {
	tgen::register_gen();

	for (int it = 0; it < 12; ++it) {
		auto g = tgen::graph(7, 11).gen();
		const auto before = g;
		g.shuffle();
		EXPECT_TRUE((graph_gen_result_valid(g, 7, 11, false, false)));
		EXPECT_TRUE(graphs_isomorphic(before, g));
	}
}

TEST(graph_test, shuffle_directed_preserves_isomorphism) {
	tgen::register_gen();

	for (int it = 0; it < 10; ++it) {
		auto g = tgen::graph(5, 7, true).gen();
		const auto before = g;
		g.shuffle();
		EXPECT_TRUE((graph_gen_result_valid(g, 5, 7, true, false)));
		EXPECT_TRUE(graphs_isomorphic(before, g));
	}
}

TEST(graph_test, shuffle_except_fixed_labels_and_isomorphism) {
	tgen::register_gen();

	// Shuffle except 0.
	for (int it = 0; it < 15; ++it) {
		auto g = tgen::graph(7, 12).gen();
		int deg0 = static_cast<int>(g.adj()[0].size());
		const auto before = g;
		g.shuffle_except({0});

		EXPECT_TRUE(static_cast<int>(g.adj()[0].size()) == deg0);
		EXPECT_TRUE(graphs_isomorphic(before, g));
	}

	// Shuffle except all (does not shuffle anything).
	for (int n = 3; n <= 8; ++n) {
		auto g =
			tgen::graph(n, std::min(10, static_cast<int>(
											max_simple_undirected_edges(n))))
				.gen();
		auto adj_before = g;
		std::set<int> all;
		for (int i = 0; i < n; ++i)
			all.insert(i);
		adj_before.shuffle_except(all);

		EXPECT_TRUE(g.adj() == adj_before.adj());
	}
}

TEST(graph_test, set_vertex_weights) {
	tgen::register_gen();

	auto g = tgen::graph(5, 4).gen();
	std::vector<int> vw = {10, 20, 30, 40, 50};
	auto w = g.set_vertex_weights(vw);

	EXPECT_TRUE(g.adj() == w.adj());
	EXPECT_TRUE(*w.vertex_weights() == vw);
	EXPECT_TRUE((graph_gen_result_valid(w, 5, 4, false, false)));
}

TEST(graph_test, add_unweighted_vertices_to_weighted_graph) {
	tgen::register_gen();

	auto g = tgen::graph(5, 4).gen();
	auto w = g.set_vertex_weights(std::vector<int>{0, 1, 2, 3, 4});

	EXPECT_THROW_TGEN_PREFIX(w.add_vertices(10),
							 "wgraph: value: cannot add unweighted vertices to "
							 "vertex-weighted graph");
}

TEST(graph_test, set_edge_weights) {
	tgen::register_gen();

	auto g = tgen::graph(4, 3).gen();
	std::vector<int> ew(g.edges().size(), 7);
	auto w = g.set_edge_weights(ew);

	EXPECT_TRUE(g.adj() == w.adj());
	EXPECT_TRUE(*w.edge_weights() == ew);
	EXPECT_TRUE((graph_gen_result_valid(w, 4, 3, false, false)));
}

TEST(graph_test, edge_weighted) {
	tgen::register_gen();

	tgen::egraph<int>::value g(3, {}, true);
	g.edge_weighted();
	g.add_edge(0, 1, 10);
	g.add_edge(1, 2, 20);

	EXPECT_TRUE(g.edge_weights().has_value());
	EXPECT_EQ(g.m(), 2);
	EXPECT_EQ((*g.edge_weights())[0], 10);
	EXPECT_EQ((*g.edge_weights())[1], 20);
	EXPECT_EQ((std::ostringstream() << g).str(), "0 1 10\n1 2 20\n");
}

TEST(graph_test, edge_weighted_invalid_nonempty) {
	tgen::register_gen();

	auto g = tgen::graph(3, 2).gen();
	EXPECT_THROW_TGEN_PREFIX(
		g.edge_weighted(), "wgraph: value: edge_weighted requires a graph with "
						   "no edges");
}

TEST(graph_test, edge_weighted_invalid_already_weighted) {
	tgen::register_gen();

	tgen::egraph<int>::value g(3, {}, true);
	g.edge_weighted();
	EXPECT_THROW_TGEN_PREFIX(g.edge_weighted(),
							 "wgraph: value: graph is already edge-weighted");
}

TEST(graph_test, glue_disjoint_union_size) {
	tgen::register_gen();

	for (int it = 0; it < 15; ++it) {
		auto a = tgen::graph(4, 3).gen();
		auto b = tgen::graph(3, 2).gen();
		int n_a = a.n();
		tgen::graph::value c = a;
		c.glue(b, std::set<std::pair<int, int>>());

		EXPECT_TRUE((graph_gen_result_valid(c, n_a + b.n(), a.m() + b.m(),
											false, false)));
	}
}

TEST(graph_test, glue_disjoint_union_directedness_mismatch) {
	tgen::register_gen();

	auto a = tgen::graph(4, 3).gen();		// undirected
	auto b = tgen::graph(3, 2, true).gen(); // directed
	tgen::graph::value c = a;
	tgen::graph::value d = b;

	EXPECT_THROW_TGEN_PREFIX(
		c.glue(d, std::set<std::pair<int, int>>()),
		"wgraph: value: graphs must have the same is_directed value");
}

TEST(graph_test, value_add_vertices_and_add_edge) {
	tgen::register_gen();

	tgen::graph::value g(3, std::vector<std::pair<int, int>>({{0, 1}, {1, 2}}));
	g.add_vertices(2);

	EXPECT_TRUE(
		(g.edges() == std::vector<std::pair<int, int>>({{0, 1}, {1, 2}})));

	EXPECT_TRUE((graph_gen_result_valid(g, 5, 2, false, false)));

	g.add_edge(0, 3);
	g.add_edge(3, 4);
	g.add_edge(2, 4);

	EXPECT_TRUE(g.edges() == (std::vector<std::pair<int, int>>(
								 {{0, 1}, {1, 2}, {0, 3}, {3, 4}, {2, 4}})));
	EXPECT_TRUE((graph_gen_result_valid(g, 5, 5, false, false)));
}

TEST(graph_test, add_edges_from) {
	tgen::register_gen();

	auto g = tgen::graph(4, 3).add_edges_from(tgen::P(4)).gen();

	const std::set<std::pair<int, int>> es(g.edges().begin(), g.edges().end());
	EXPECT_TRUE(
		(es == std::set<std::pair<int, int>>({{0, 1}, {1, 2}, {2, 3}})));
	EXPECT_TRUE((graph_gen_result_valid(g, 4, 3, false, false)));
}

TEST(graph_test, add_edges_from_directedness_mismatch) {
	tgen::register_gen();

	EXPECT_THROW_TGEN_PREFIX(
		tgen::graph(3, 2).add_edges_from(tgen::graph::value(3, {{0, 1}}, true)),
		"wgraph: graphs must have the same is_directed value");
}

TEST(graph_test, operator_plus_directedness_mismatch) {
	tgen::register_gen();

	auto a = tgen::graph(3, 1).gen();
	auto b = tgen::graph(2, 0, true).gen();

	EXPECT_THROW_TGEN_PREFIX(
		a + b, "wgraph: value: graphs must have the same is_directed value");
}

TEST(graph_test, print_edge_list) {
	tgen::register_gen();

	auto g = tgen::graph::value(
		4, std::vector<std::pair<int, int>>({{0, 1}, {0, 2}, {0, 3}}));

	EXPECT_TRUE((std::ostringstream() << g).str() == "0 1\n0 2\n0 3\n");

	auto ga = g;
	EXPECT_TRUE((std::ostringstream() << ga.add_1()).str() ==
				"1 2\n1 3\n1 4\n");

	auto gb = g;
	EXPECT_TRUE(((std::ostringstream() << gb.print_nm()).str() ==
				 "4 3\n0 1\n0 2\n0 3\n"));
}

TEST(graph_test, print_vertex_weights) {
	tgen::register_gen();

	auto g = tgen::graph::value(
				 3, std::vector<std::pair<int, int>>({{0, 1}, {1, 2}}))
				 .set_vertex_weights(std::vector<int>({5, 6, 7}));

	EXPECT_TRUE((std::ostringstream() << g).str() == "5 6 7\n0 1\n1 2\n");

	auto ga = g;
	EXPECT_TRUE(
		((std::ostringstream() << ga.add_1()).str() == "5 6 7\n1 2\n2 3\n"));

	auto gb = g;
	EXPECT_TRUE(((std::ostringstream() << gb.print_nm()).str() ==
				 "3 2\n5 6 7\n0 1\n1 2\n"));
}

TEST(graph_test, print_weighted_edge_list) {
	tgen::register_gen();

	auto g = tgen::graph::value(
				 3, std::vector<std::pair<int, int>>({{0, 1}, {1, 2}}))
				 .set_edge_weights(std::vector<int>({5, 6}));

	EXPECT_TRUE((std::ostringstream() << g).str() == "0 1 5\n1 2 6\n");

	auto ga = g;
	EXPECT_TRUE((std::ostringstream() << ga.add_1()).str() == "1 2 5\n2 3 6\n");
}

TEST(graph_test, to_std) {
	tgen::register_gen();

	auto g = tgen::graph(6, 8).gen();
	auto t = g.to_std();
	EXPECT_TRUE((graph_gen_result_valid(g, 6, 8, false, false)));
	EXPECT_TRUE((t == std::tuple(6, 8, g.adj())));
}

TEST(graph_test, gen_uniform_labeled_graphs_small) {
	tgen::register_gen();

	expect_generator_uniform(tgen::graph(3, 2), 3);
	expect_generator_uniform(tgen::graph(6, 3), 455);
}

TEST(graph_test, weight_type_aliases) {
	tgen::register_gen();

	EXPECT_TRUE((std::is_same_v<tgen::graph, tgen::wgraph<int, int>>));
	EXPECT_TRUE(
		(std::is_same_v<tgen::vgraph<double>, tgen::wgraph<double, int>>));
	EXPECT_TRUE(
		(std::is_same_v<tgen::egraph<unsigned>, tgen::wgraph<int, unsigned>>));
}

TEST(graph_test, vgraph_gen_and_vertex_weights) {
	tgen::register_gen();

	auto g = tgen::vgraph<float>(5, 6).gen();

	EXPECT_TRUE((graph_gen_result_valid(g, 5, 6, false, false)));

	auto h = g.set_vertex_weights(std::vector<float>{1, 2, 3, 4, 5});

	EXPECT_TRUE(h.vertex_weights().has_value() and
				h.vertex_weights()->size() == 5u);
}

TEST(graph_test, egraph_gen_and_edge_weights) {
	tgen::register_gen();

	auto g = tgen::egraph<unsigned>(4, 3).gen();
	std::vector<unsigned> ew(g.m(), 42u);
	auto h = g.set_edge_weights(ew);
	EXPECT_TRUE(h.edge_weights().has_value() and *h.edge_weights() == ew);
	EXPECT_TRUE((graph_gen_result_valid(h, 4, 3, false, false)));
}

TEST(graph_test, wgraph_both_weight_dimensions) {
	tgen::register_gen();

	auto g = tgen::wgraph<short, bool>(5, 4).gen();
	auto gv = g.set_vertex_weights(std::vector<short>{1, 2, 3, 4, 5});
	auto ge = g.set_edge_weights(std::vector<bool>{true, false, true, true});

	EXPECT_TRUE(gv.vertex_weights().has_value() and
				ge.edge_weights().has_value());
	EXPECT_TRUE((graph_gen_result_valid(gv, 5, 4, false, false)));
	EXPECT_TRUE((graph_gen_result_valid(ge, 5, 4, false, false)));
}

TEST(graph_test, gen_exhaustive_all_directed_and_self_loop_modes) {
	tgen::register_gen();

	for (bool directed : {false, true}) {
		for (bool self_loops : {false, true}) {
			for (int n = 2; n <= 10; ++n) {
				const long long mx = max_graph_edges(n, directed, self_loops);
				for (int m = 0; m <= static_cast<int>(mx); ++m) {
					auto g = tgen::graph(n, m, directed, self_loops).gen();
					EXPECT_TRUE((
						graph_gen_result_valid(g, n, m, directed, self_loops)));
				}
			}
		}
	}
}

TEST(graph_test, single_vertex_all_directed_self_loop_modes) {
	tgen::register_gen();

	for (bool directed : {false, true}) {
		for (bool self_loops : {false, true}) {
			auto g = tgen::graph(1, 0, directed, self_loops).gen();
			EXPECT_TRUE(
				(graph_gen_result_valid(g, 1, 0, directed, self_loops)));
		}
	}

	for (bool directed : {false, true}) {
		auto g = tgen::graph(1, 1, directed, true).gen();
		EXPECT_TRUE((graph_gen_result_valid(g, 1, 1, directed, true)));
	}

	for (bool directed : {false, true}) {
		EXPECT_THROW_TGEN_PREFIX(tgen::graph(1, 1, directed, false).gen(),
								 "wgraph: not enough edges to generate");
	}
}

TEST(graph_test, gen_undirected_with_self_loops_full_includes_all_diagonals) {
	tgen::register_gen();

	const int n = 5;
	const int m = max_graph_edges(n, false, true);
	auto g = tgen::graph(n, m, false, true).gen();

	for (int i = 0; i < n; ++i) {
		EXPECT_TRUE((std::find(g.edges().begin(), g.edges().end(),
							   std::pair(i, i)) != g.edges().end()));
	}
	EXPECT_TRUE((graph_gen_result_valid(g, n, m, false, true)));
}

TEST(graph_test, gen_directed_with_self_loops_full_includes_all_ordered_pairs) {
	tgen::register_gen();

	const int n = 4;
	const int m = n * n;
	auto g = tgen::graph(n, m, true, true).gen();
	std::set<std::pair<int, int>> expect;
	for (int u = 0; u < n; ++u)
		for (int v = 0; v < n; ++v)
			expect.insert({u, v});

	for (auto e : g.edges())
		EXPECT_TRUE(expect.erase(e) == 1u);
	EXPECT_TRUE(expect.empty());
	EXPECT_TRUE((graph_gen_result_valid(g, n, m, true, true)));
}

TEST(graph_test, from_tree) {
	tgen::register_gen();

	auto t = tgen::tree(7).gen();
	auto g = tgen::graph::value(t);

	EXPECT_EQ(g.n(), t.n());
	EXPECT_EQ(g.m(), static_cast<int>(t.edges().size()));
	EXPECT_FALSE(g.is_directed());
	EXPECT_EQ(g.edges(), t.edges());
	EXPECT_TRUE(is_connected_undirected(g));
}

TEST(graph_test, from_tree_preserves_weights) {
	tgen::register_gen();

	auto t = tgen::etree<int>(4).gen().set_edge_weights({1, 2, 3});
	auto g = tgen::egraph<int>::value(t);

	ASSERT_TRUE(g.edge_weights().has_value());
	EXPECT_EQ(*g.edge_weights(), *t.edge_weights());
}

TEST(graph_test, tree_graph_roundtrip) {
	tgen::register_gen();

	auto t0 = tgen::tree(5).gen();
	auto g = tgen::graph::value(t0);
	auto t = tgen::tree::value(g);

	EXPECT_EQ(t.n(), t0.n());
	EXPECT_EQ(std::set(t.edges().begin(), t.edges().end()),
			  std::set(t0.edges().begin(), t0.edges().end()));
}

#include "benchmark.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

volatile uint64_t sink = 0;

struct BenchmarkScale {
	bool smoke = false;

	int runs() const { return smoke ? 3 : 7; }

	int n_graph() const { return smoke ? 10'000 : 1'000'000; }
	int n_tree() const { return smoke ? 10'000 : 1'000'000; }
	int n_list() const { return smoke ? 100'000 : 5'000'000; }
	int n_perm() const { return smoke ? 100'000 : 5'000'000; }
	int n_geom() const { return smoke ? 5'000 : 1'000'000; }
	long long geom_max() const { return smoke ? 50'000 : 3'000'000; }
	int n_bipartite_side() const { return smoke ? 100 : 1'000; }
	int m_bipartite() const { return smoke ? 5'000 : 500'000; }

	int partition_n() const { return smoke ? 50'000 : 5'050'000; }
	int partition_fixed_n() const { return smoke ? 500'000 : 58'000'000; }
	int partition_bounded_n() const { return smoke ? 400 : 4'200; }
	int partition_fast_k() const { return smoke ? 300'000 : 3'000'000; }
};

template <typename G> void consume_graph_like(const G &g) {
	sink += g.n() + g.m();
	for (auto &e : g.edges())
		sink += e.first + e.second;
}

void consume_graph(const tgen::graph::value &g) { consume_graph_like(g); }

void consume_tree(const tgen::tree::value &t) {
	sink += t.n() + t.edges().size();
	for (auto &e : t.edges())
		sink += e.first + e.second;
}

void consume_polygon(
	const std::vector<tgen::geometry::point<long long>> &poly) {
	for (auto &p : poly)
		sink += p.x() + p.y();
}

void consume_list(const tgen::list<int>::value &list) {
	for (int i = 0; i < list.size(); ++i)
		sink += list[i];
}

void consume_str(const tgen::str::value &s) {
	for (int i = 0; i < s.size(); ++i)
		sink += s[i];
}

void consume_partition(const std::vector<int> &part) {
	for (int x : part)
		sink += x;
}

void consume_partition(const std::vector<uint64_t> &part) {
	for (uint64_t x : part)
		sink += x;
}

std::vector<benchmark::CaseResult> run_all(const BenchmarkScale &scale) {
	int n = scale.n_graph();
	int n_tree = scale.n_tree();
	int runs = scale.runs();
	std::vector<benchmark::CaseResult> results;

	auto run = [&](const std::string &name, const std::string &suffix,
				   const std::string &params, const std::string &doc, auto fn) {
		results.push_back(
			benchmark::run_case(name, suffix, params, doc, fn, runs));
	};

	run("tgen::graph::get_connected", " (m=n)",
		scale.smoke ? "n=1e4, m=1e4" : "n=1e6, m=1e6",
		"tgen::wgraph::get_connected",
		[=] { consume_graph(tgen::graph(n, n).get_connected()); });

	run("tgen::graph::get_connected", " (m=2n)",
		scale.smoke ? "n=1e4, m=2e4" : "n=1e6, m=2e6",
		"tgen::wgraph::get_connected",
		[=] { consume_graph(tgen::graph(n, 2 * n).get_connected()); });

	run("tgen::graph::get_acyclic", "",
		scale.smoke ? "n=1e4, m=1e4" : "n=1e6, m=1e6",
		"tgen::wgraph::get_acyclic",
		[=] { consume_graph(tgen::graph(n, n, true).get_acyclic()); });

	run("tgen::graph::gen", "", scale.smoke ? "n=1e4, m=1e4" : "n=1e6, m=1e6",
		"tgen::wgraph::gen", [=] { consume_graph(tgen::graph(n, n).gen()); });

	run("tgen::graph::gen", " (directed)",
		scale.smoke ? "n=1e4, m=1e4, directed" : "n=1e6, m=1e6, directed",
		"tgen::wgraph::gen",
		[=] { consume_graph(tgen::graph(n, n, true).gen()); });

	int n1 = scale.n_bipartite_side();
	int m_bip = scale.m_bipartite();
	run("tgen::graph::gen_bipartite", "",
		scale.smoke ? "n1=1e2, n2=1e2, m=5e3" : "n1=1e3, n2=1e3, m=5e5",
		"tgen::wgraph::gen_bipartite",
		[=] { consume_graph(tgen::graph::gen_bipartite(n1, n1, m_bip)); });

	run("tgen::graph::gen_skewed", " (m=n)",
		scale.smoke ? "n=1e4, m=1e4, elongation=1e2, spread=2"
					: "n=1e6, m=1e6, elongation=1e2, spread=2",
		"tgen::wgraph::gen_skewed",
		[=] { consume_graph(tgen::graph::gen_skewed(n, n, 100, 2)); });

	int spread_2n = scale.smoke ? 4 : 6;
	run("tgen::graph::gen_skewed", " (m=2n)",
		scale.smoke ? "n=1e4, m=2e4, elongation=1e2, spread=4"
					: "n=1e6, m=2e6, elongation=1e2, spread=6",
		"tgen::wgraph::gen_skewed", [=] {
			consume_graph(tgen::graph::gen_skewed(n, 2 * n, 100, spread_2n));
		});

	int skew_worst_m = scale.smoke ? 2 * n - 3 : 1'999'997;
	run("tgen::graph::gen_skewed", " (distinct worst)",
		scale.smoke ? "n=1e4, m=2n-3, elongation=1e2, spread=2"
					: "n=1e6, m=2n-3, elongation=1e2, spread=2",
		"tgen::wgraph::gen_skewed", [=] {
			consume_graph(tgen::graph::gen_skewed(n, skew_worst_m, 100, 2));
		});

	run("tgen::tree::gen", "", scale.smoke ? "n=1e4" : "n=1e6",
		"tgen::wtree::gen", [=] { consume_tree(tgen::tree(n_tree).gen()); });

	run("tgen::tree::gen_skewed", "",
		scale.smoke ? "n=1e4, elongation=1e2" : "n=1e6, elongation=1e2",
		"tgen::wtree::gen_skewed",
		[=] { consume_tree(tgen::tree::gen_skewed(n_tree, 100)); });

	run("tgen::list<int>::gen", " (all_different)",
		scale.smoke ? "n=1e5, value_left=1, value_right=2e5"
					: "n=1e6, value_left=1, value_right=2e6",
		"tgen::list::gen", [=] {
			consume_list(tgen::list<int>(scale.n_list(), 1, 2 * scale.n_list())
							 .all_different()
							 .gen());
		});

	run("tgen::list<int>::gen", "",
		scale.smoke ? "n=5e5, value_left=1, value_right=1e7"
					: "n=5e6, value_left=1, value_right=1e7",
		"tgen::list::gen", [=] {
			consume_list(
				tgen::list<int>(scale.n_list(), 1, 10 * scale.n_list()).gen());
		});

	run("tgen::permutation::gen", "", scale.smoke ? "n=5e5" : "n=5e6",
		"tgen::permutation::gen",
		[=] { sink += tgen::permutation(scale.n_perm()).gen().size(); });

	// Each OR branch produces 4 characters; r repetitions yield 4*r characters.
	int regex_reps = scale.smoke ? 25'000 : 2'500'000;
	run("tgen::str::gen", " (regex)",
		scale.smoke
			? "pattern=(([1-9][0-9]{3}|[A-F]{4})|(ab|cd){2}){r}, len=1e5"
			: "pattern=(([1-9][0-9]{3}|[A-F]{4})|(ab|cd){2}){r}, len=1e7",
		"tgen::str::gen", [=] {
			consume_str(tgen::str("(([1-9][0-9]{3}|[A-F]{4})|(ab|cd){2}){%d}",
								  regex_reps)
							.gen());
		});

	int ng = scale.n_geom();
	long long gmax = scale.geom_max();
	run("tgen::geometry::random_simple_polygon", "",
		scale.smoke ? "n=5e3, min=0, max=5e4, strict=false"
					: "n=1e6, min=0, max=3e6, strict=false",
		"tgen::geometry::random_simple_polygon", [=] {
			consume_polygon(
				tgen::geometry::random_simple_polygon(ng, 0, gmax, false));
		});

	run("tgen::geometry::random_points_general_position", "",
		scale.smoke ? "n=5e3, min=0, max=5e4" : "n=1e6, min=0, max=3e6",
		"tgen::geometry::random_points_general_position", [=] {
			consume_polygon(
				tgen::geometry::random_points_general_position(ng, 0, gmax));
		});

	run("tgen::geometry::random_convex_polygon", "",
		scale.smoke ? "n=5e3, min=0, max=5e4, strict=false"
					: "n=1e6, min=0, max=3e6, strict=false",
		"tgen::geometry::random_convex_polygon", [=] {
			consume_polygon(
				tgen::geometry::random_convex_polygon(ng, 0, gmax, false));
		});

	auto polygon_points =
		tgen::geometry::random_points_general_position(ng, 0, gmax);
	run("tgen::geometry::random_simple_polygon_through_points", "",
		scale.smoke ? "n=5e3" : "n=1e6",
		"tgen::geometry::random_simple_polygon_through_points",
		[&polygon_points] {
			consume_polygon(
				tgen::geometry::random_simple_polygon_through_points(
					polygon_points));
		});

	run("tgen::math::gen_partition", "", scale.smoke ? "n=5e4" : "n=5.05e6",
		"tgen::math::gen_partition", [=] {
			consume_partition(tgen::math::gen_partition(scale.partition_n()));
		});

	run("tgen::math::gen_partition_fixed_size", "",
		scale.smoke ? "n=5e5, k=10, part_left=0" : "n=5.8e7, k=10, part_left=0",
		"tgen::math::gen_partition_fixed_size", [=] {
			consume_partition(tgen::math::gen_partition_fixed_size(
				scale.partition_fixed_n(), 10));
		});

	run("tgen::math::gen_partition_fixed_size", " (bounded)",
		scale.smoke ? "n=400, k=400, part_left=0, part_right=20"
					: "n=4200, k=4200, part_left=0, part_right=20",
		"tgen::math::gen_partition_fixed_size", [=] {
			int bn = scale.partition_bounded_n();
			consume_partition(
				tgen::math::gen_partition_fixed_size(bn, bn, 0, 20));
		});

	run("tgen::math::gen_partition_fixed_size_fast", "",
		scale.smoke ? "n=1e18, k=3e5, part_left=0"
					: "n=1e18, k=3e6, part_left=0",
		"tgen::math::gen_partition_fixed_size_fast", [=] {
			consume_partition(tgen::math::gen_partition_fixed_size_fast(
				1'000'000'000'000'000'000ULL, scale.partition_fast_k()));
		});

	return results;
}

bool run_check(const std::string &baseline, const std::string &current,
			   double threshold) {
	std::string cmd = "python3 docs/benchmark_check.py --baseline " + baseline +
					  " --current " + current + " --threshold " +
					  std::to_string(threshold);
	return std::system(cmd.c_str()) == 0;
}

void usage(const char *prog) {
	std::cerr << "Usage: " << prog
			  << " [--json PATH] [--smoke] [--update-baseline PATH]\n"
				 "       [--check BASELINE [--threshold R]]\n"
			  << "  Default json: docs/benchmark_results.json\n"
			  << "  Run 'make doc' to render HTML with doc links.\n";
}

} // namespace

int main(int argc, char **argv) {
	std::string json_path = "docs/benchmark_results.json";
	bool smoke = false;
	bool update_baseline = false;
	std::string check_baseline;
	double threshold = 2.0;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--json" and i + 1 < argc)
			json_path = argv[++i];
		else if (arg == "--smoke")
			smoke = true;
		else if (arg == "--update-baseline" and i + 1 < argc) {
			update_baseline = true;
			json_path = argv[++i];
		} else if (arg == "--check" and i + 1 < argc)
			check_baseline = argv[++i];
		else if (arg == "--threshold" and i + 1 < argc)
			threshold = std::stod(argv[++i]);
		else if (arg == "--help" or arg == "-h") {
			usage(argv[0]);
			return 0;
		} else {
			std::cerr << "Unknown argument: " << arg << '\n';
			usage(argv[0]);
			return 1;
		}
	}

	tgen::register_gen(42);

	BenchmarkScale scale;
	scale.smoke = smoke;

	benchmark::Report report;
	report.generated_at = benchmark::iso_timestamp();
	report.compiler = __VERSION__;
	report.flags = "-std=c++17 -O2";
	report.hostname = benchmark::hostname();
	const auto start = benchmark::clock::now();
	report.results = run_all(scale);
	const double total_ms = benchmark::elapsed_ms(start);

	try {
		benchmark::write_json(report, json_path);
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return 1;
	}

	std::cout << "Wrote " << json_path << '\n';
	std::cout << "Benchmarks took " << total_ms / 1000.0 << " s\n";

	if (!check_baseline.empty() and
		!run_check(check_baseline, json_path, threshold))
		return 1;

	return 0;
}

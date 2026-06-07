#include "benchmark.h"

#include <iostream>
#include <string>

namespace {

volatile uint64_t sink = 0;

template <typename G> void consume_graph_like(const G &g) {
	sink += static_cast<uint64_t>(g.n()) + static_cast<uint64_t>(g.m());
	for (const auto &e : g.edges())
		sink +=
			static_cast<uint64_t>(e.first) + static_cast<uint64_t>(e.second);
}

void consume_graph(const tgen::graph::value &g) { consume_graph_like(g); }

void consume_tree(const tgen::tree::value &t) {
	sink +=
		static_cast<uint64_t>(t.n()) + static_cast<uint64_t>(t.edges().size());
	for (const auto &e : t.edges())
		sink +=
			static_cast<uint64_t>(e.first) + static_cast<uint64_t>(e.second);
}

void consume_polygon(
	const std::vector<tgen::geometry::point<long long>> &poly) {
	for (const auto &p : poly)
		sink += static_cast<uint64_t>(p.x()) + static_cast<uint64_t>(p.y());
}

void consume_list(const tgen::list<int>::value &list) {
	for (int i = 0; i < list.size(); ++i)
		sink += static_cast<uint64_t>(list[i]);
}

std::vector<benchmark::CaseResult> run_all() {
	std::vector<benchmark::CaseResult> results;

	results.push_back(benchmark::run_case(
		"tgen::graph::get_connected", " (m=n)", "n=1e6, m=1e6",
		"tgen::wgraph::get_connected", [] {
			consume_graph(tgen::graph(1'000'000, 1'000'000).get_connected());
		}));

	results.push_back(benchmark::run_case(
		"tgen::graph::get_connected", " (m=2n)", "n=1e6, m=2e6",
		"tgen::wgraph::get_connected", [] {
			consume_graph(tgen::graph(1'000'000, 2'000'000).get_connected());
		}));

	results.push_back(benchmark::run_case(
		"tgen::graph::gen", "", "n=1e6, m=1e6", "tgen::wgraph::gen",
		[] { consume_graph(tgen::graph(1'000'000, 1'000'000).gen()); }));

	results.push_back(
		benchmark::run_case("tgen::graph::gen_skewed", " (m=n)",
							"n=1e6, m=1e6, elongation=1e2, spread=2",
							"tgen::wgraph::gen_skewed", [] {
								consume_graph(tgen::graph::gen_skewed(
									1'000'000, 1'000'000, 100, 2));
							}));

	// spread=6: spread=2 cannot reach m=2n at n=1e6.
	results.push_back(
		benchmark::run_case("tgen::graph::gen_skewed", " (m=2n)",
							"n=1e6, m=2e6, elongation=1e2, spread=6",
							"tgen::wgraph::gen_skewed", [] {
								consume_graph(tgen::graph::gen_skewed(
									1'000'000, 2'000'000, 100, 6));
							}));

	// spread=2, path-like tree: at most n-2 distinct ancestor edges (~m=2n-3).
	results.push_back(
		benchmark::run_case("tgen::graph::gen_skewed", " (distinct worst)",
							"n=1e6, m=2n-3, elongation=1e2, spread=2",
							"tgen::wgraph::gen_skewed", [] {
								consume_graph(tgen::graph::gen_skewed(
									1'000'000, 1'999'997, 100, 2));
							}));

	results.push_back(
		benchmark::run_case("tgen::tree::gen", "", "n=1e6", "tgen::wtree::gen",
							[] { consume_tree(tgen::tree(1'000'000).gen()); }));

	results.push_back(benchmark::run_case(
		"tgen::tree::gen_skewed", "", "n=1e6, elongation=1e2",
		"tgen::wtree::gen_skewed",
		[] { consume_tree(tgen::tree::gen_skewed(1'000'000, 100)); }));

	results.push_back(benchmark::run_case(
		"tgen::list<int>::gen", " (all_different)",
		"n=1e6, value_left=1, value_right=2e6", "tgen::list::gen", [] {
			consume_list(
				tgen::list<int>(1'000'000, 1, 2'000'000).all_different().gen());
		}));

	results.push_back(benchmark::run_case(
		"tgen::geometry::random_simple_polygon", "", "n=1e6, min=0, max=3e6",
		"tgen::geometry::random_simple_polygon", [] {
			consume_polygon(
				tgen::geometry::random_simple_polygon(1'000'000, 0, 3'000'000));
		}));

	results.push_back(benchmark::run_case(
		"tgen::geometry::random_points_general_position", "",
		"n=1e6, min=0, max=3e6",
		"tgen::geometry::random_points_general_position", [] {
			consume_polygon(tgen::geometry::random_points_general_position(
				1'000'000, 0, 3'000'000));
		}));

	results.push_back(benchmark::run_case(
		"tgen::geometry::random_convex_polygon", "", "n=1e6, min=0, max=3e6",
		"tgen::geometry::random_convex_polygon", [] {
			consume_polygon(
				tgen::geometry::random_convex_polygon(1'000'000, 0, 3'000'000));
		}));

	const auto polygon_points =
		tgen::geometry::random_points_general_position(1'000'000, 0, 3'000'000);
	results.push_back(benchmark::run_case(
		"tgen::geometry::random_simple_polygon_through_points", "", "n=1e6",
		"tgen::geometry::random_simple_polygon_through_points",
		[&polygon_points] {
			consume_polygon(
				tgen::geometry::random_simple_polygon_through_points(
					polygon_points));
		}));

	return results;
}

void usage(const char *prog) {
	std::cerr << "Usage: " << prog << " [--json PATH]\n"
			  << "  Default: docs/benchmark_results.json\n"
			  << "  Run 'make doc' to render HTML with doc links.\n";
}

} // namespace

int main(int argc, char **argv) {
	std::string json_path = "docs/benchmark_results.json";

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--json" and i + 1 < argc)
			json_path = argv[++i];
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

	benchmark::Report report;
	report.generated_at = benchmark::iso_timestamp();
	report.compiler = __VERSION__;
	report.flags = "-std=c++17 -O2";
	report.hostname = benchmark::hostname();
	const auto start = benchmark::clock::now();
	report.results = run_all();
	const double total_ms = benchmark::elapsed_ms(start);

	try {
		benchmark::write_json(report, json_path);
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return 1;
	}

	std::cout << "Wrote " << json_path << '\n';
	std::cout << "Benchmarks took " << total_ms / 1000.0 << " s\n";
	return 0;
}

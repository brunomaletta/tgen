#pragma once

#include "../single_include/tgen.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace benchmark {

using clock = std::chrono::steady_clock;

struct CaseResult {
	std::string name;
	std::string name_suffix;
	std::string params;
	std::string doc_symbol;
	std::vector<double> runs_ms;
	double median_ms = 0;
};

struct Report {
	std::string generated_at;
	std::string compiler;
	std::string flags;
	std::string hostname;
	std::vector<CaseResult> results;
};

inline double elapsed_ms(clock::time_point start) {
	return std::chrono::duration<double, std::milli>(clock::now() - start)
		.count();
}

inline double median(std::vector<double> values) {
	if (values.empty())
		return 0;
	std::sort(values.begin(), values.end());
	return values[values.size() / 2];
}

template <typename Fn>
CaseResult run_case(const std::string &name, const std::string &name_suffix,
					const std::string &params, const std::string &doc_symbol,
					Fn fn, int num_runs = 7) {
	CaseResult result{name, name_suffix, params, doc_symbol, {}, 0};

	std::cout << "Benchmarking " << name << name_suffix << "...\n"
			  << std::flush;

	fn(); // warmup

	for (int i = 0; i < num_runs; ++i) {
		auto start = clock::now();
		fn();
		result.runs_ms.push_back(elapsed_ms(start));
	}

	result.median_ms = median(result.runs_ms);
	return result;
}

inline std::string json_escape(const std::string &s) {
	std::string out;
	out.reserve(s.size());
	for (char c : s) {
		switch (c) {
		case '\\':
			out += "\\\\";
			break;
		case '"':
			out += "\\\"";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		default:
			out += c;
		}
	}
	return out;
}

inline std::string format_ms(double ms) {
	return std::to_string(static_cast<long long>(std::llround(ms)));
}

inline std::string iso_timestamp() {
	std::time_t now = std::time(nullptr);
	std::tm tm_buf{};
#if defined(_WIN32)
	gmtime_s(&tm_buf, &now);
#else
	gmtime_r(&now, &tm_buf);
#endif
	char buf[32];
	std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
	return buf;
}

inline std::string hostname() {
	char buf[256];
	if (gethostname(buf, sizeof(buf)) != 0)
		return "unknown";
	return buf;
}

inline void write_json(const Report &report, const std::string &path) {
	std::ofstream out(path);
	if (!out)
		throw std::runtime_error("benchmark: cannot write " + path);

	out << "{\n";
	out << "  \"generated_at\": \"" << json_escape(report.generated_at)
		<< "\",\n";
	out << "  \"compiler\": \"" << json_escape(report.compiler) << "\",\n";
	out << "  \"flags\": \"" << json_escape(report.flags) << "\",\n";
	out << "  \"hostname\": \"" << json_escape(report.hostname) << "\",\n";
	out << "  \"results\": [\n";

	for (size_t i = 0; i < report.results.size(); ++i) {
		const auto &r = report.results[i];
		out << "    {\n";
		out << "      \"name\": \"" << json_escape(r.name) << "\",\n";
		out << "      \"name_suffix\": \"" << json_escape(r.name_suffix)
			<< "\",\n";
		out << "      \"params\": \"" << json_escape(r.params) << "\",\n";
		if (!r.doc_symbol.empty())
			out << "      \"doc_symbol\": \"" << json_escape(r.doc_symbol)
				<< "\",\n";
		out << "      \"median_ms\": " << format_ms(r.median_ms) << ",\n";
		out << "      \"runs_ms\": [";
		for (size_t j = 0; j < r.runs_ms.size(); ++j) {
			if (j)
				out << ", ";
			out << format_ms(r.runs_ms[j]);
		}
		out << "]\n";
		out << "    }";
		if (i + 1 < report.results.size())
			out << ",";
		out << "\n";
	}

	out << "  ]\n";
	out << "}\n";
}

} // namespace benchmark

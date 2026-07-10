.DELETE_ON_ERROR:
.DEFAULT_GOAL := help

SRC_DIR       := single_include
DOC_BUILD_DIR := docs/build
DOC_HTML_DIR  := $(DOC_BUILD_DIR)
DOC_INDEX     := $(abspath $(DOC_HTML_DIR)/index.html)
DOC_SRC_DIR   := docs
THEME_DIR     := docs/doxygen-awesome-css
LLM_BASE_URL  ?= https://brunomaletta.github.io/tgen

# Extra argv for runnable examples, e.g. make graph ARGS='--seed 1'   (also works with graph-asan, etc.)
ARGS ?=

# Parallel builds by default (override: make NPROCS=1)
NPROCS ?= $(shell nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
MAKEFLAGS += -j$(NPROCS)

# Optional compiler cache (unset CCACHE= to disable)
CCACHE := $(shell command -v ccache 2>/dev/null)

BUILD_ROOT := build
GCC_OBJDIR   := $(BUILD_ROOT)/gcc
CLANG_OBJDIR := $(BUILD_ROOT)/clang
ASAN_OBJDIR  := $(BUILD_ROOT)/asan
STRESS_OBJDIR := $(BUILD_ROOT)/stress

# Example binaries: release vs sanitizer trees so ASAN=1 cannot reuse a release binary
EXAMPLE_BINDIR := $(BUILD_ROOT)/examples
EXAMPLE_REL    := $(EXAMPLE_BINDIR)/rel
EXAMPLE_SAN    := $(EXAMPLE_BINDIR)/asan
# Set ASAN=1 for sanitized examples, e.g. make graph ASAN=1  or  make graph-asan
EXAMPLE_USE_SAN := $(filter 1 yes true on ON,$(ASAN))
EXAMPLE_OUT     = $(if $(strip $(EXAMPLE_USE_SAN)),$(EXAMPLE_SAN),$(EXAMPLE_REL))

TEST_SRCS     := $(sort $(wildcard tests/*.cpp))
TEST_HEADERS  := $(sort $(wildcard tests/*.h))
TGEN_HEADER   := $(SRC_DIR)/tgen.h

GCC_OBJS   := $(patsubst tests/%.cpp,$(GCC_OBJDIR)/%.o,$(TEST_SRCS))
CLANG_OBJS := $(patsubst tests/%.cpp,$(CLANG_OBJDIR)/%.o,$(TEST_SRCS))
ASAN_OBJS  := $(patsubst tests/%.cpp,$(ASAN_OBJDIR)/%.o,$(TEST_SRCS))
STRESS_OBJS := $(patsubst tests/%.cpp,$(STRESS_OBJDIR)/%.o,$(TEST_SRCS))

CXX_GCC   := $(if $(CCACHE),ccache )g++
CXX_CLANG := $(if $(CCACHE),ccache )clang++
CXX_ASAN  := $(CXX_GCC)

CXXFLAGS_COMMON := -std=c++17 -Wall -Wshadow -pthread
CXXFLAGS_GCC    := $(CXXFLAGS_COMMON) -O2
CXXFLAGS_CLANG  := $(CXXFLAGS_COMMON) -O2
CXXFLAGS_ASAN   := $(CXXFLAGS_COMMON) -g -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer
CXXFLAGS_STRESS := $(CXXFLAGS_GCC) -DTGEN_STRESSTEST

GTEST_LIBS := -lgtest -lgtest_main
GTEST_ARGS ?=
WERROR_FLAG := $(if $(filter 1 yes true on,$(WERROR)),-Werror,)

# Run gtest binary $(1); on failure print failed case names.
define RUN_GTEST
	bash -c 'set -o pipefail; log="$(BUILD_ROOT)/test_last.log"; mkdir -p "$(BUILD_ROOT)"; $(1) $(GTEST_ARGS) 2>&1 | tee "$$log"; status=$$?; if [ $$status -ne 0 ]; then printf "\n%s\n" "Failed test(s):" >&2; grep -E "^\[  FAILED  \] [A-Za-z0-9_]+\.[A-Za-z0-9_]+" "$$log" | sed "s/^\[  FAILED  \] /  /; s/ ([0-9]* ms)$$//" | sort -u >&2; printf "\n%s\n" "Re-run one: $(1) --gtest_filter=Suite.TestName" >&2; exit $$status; fi'
endef

# $(1)=install name  $(2)=source under examples/  $(3)=compiler (CXX_GCC or CXX_CLANG)
define EXAMPLE_LINK_RULE
$(EXAMPLE_REL)/$(1): examples/$(2) $(TGEN_HEADER) | $(EXAMPLE_REL)
	$(3) $(CXXFLAGS_COMMON) -O2 -I $(SRC_DIR) $$< -o $$@
$(EXAMPLE_SAN)/$(1): examples/$(2) $(TGEN_HEADER) | $(EXAMPLE_SAN)
	$(3) $(CXXFLAGS_ASAN) -I $(SRC_DIR) $$< -o $$@
endef

define EXAMPLE_DEBUG_RULE
$(EXAMPLE_REL)/$(1): examples/$(2) $(TGEN_HEADER) | $(EXAMPLE_REL)
	$(3) $(CXXFLAGS_COMMON) -g -I $(SRC_DIR) $$< -o $$@
$(EXAMPLE_SAN)/$(1): examples/$(2) $(TGEN_HEADER) | $(EXAMPLE_SAN)
	$(3) $(CXXFLAGS_ASAN) -I $(SRC_DIR) $$< -o $$@
endef

$(eval $(call EXAMPLE_LINK_RULE,graph,graph.cpp,$(CXX_CLANG)))
$(eval $(call EXAMPLE_LINK_RULE,geometry,geometry.cpp,$(CXX_GCC)))
$(eval $(call EXAMPLE_LINK_RULE,sample,all.cpp,$(CXX_GCC)))
$(eval $(call EXAMPLE_LINK_RULE,range_queries,range_queries.cpp,$(CXX_GCC)))
$(eval $(call EXAMPLE_DEBUG_RULE,sample_debug,all.cpp,$(CXX_GCC)))

GEOMETRY_HTML := $(BUILD_ROOT)/geometry.html
GEOMETRY_OUT  := $(BUILD_ROOT)/geometry.out

BENCHMARK_DIR  := $(BUILD_ROOT)/benchmarks
BENCHMARK_BIN  := $(BENCHMARK_DIR)/run
BENCHMARK_JSON := docs/benchmark_results.json
BENCHMARK_HTML := $(DOC_BUILD_DIR)/benchmark_include.html
BENCHMARK_CXXFLAGS := -std=c++17 -O2
BENCHMARK_LOCAL_BASELINE := benchmarks/local_baseline.json
BENCHMARK_LOCAL_CURRENT  := /tmp/tgen_bench_local.json
BENCHMARK_BASELINE_WORKTREE := $(BUILD_ROOT)/benchmark_baseline_worktree
BENCHMARK_THRESHOLD ?= 2.0

.PHONY: all doc doc-prepare llms doc-rebuild clean-doc opendoc lint lint-check test test_clang test_asan stresstest \
	sample sample_debug range_queries graph geometry benchmark \
	benchmark-baseline-local benchmark-baseline-local-auto benchmark-baseline-ci benchmark-check-local \
	benchmark-check check cloc help print-% %-asan

clean:
	-rm -rf $(BUILD_ROOT)

$(EXAMPLE_REL) $(EXAMPLE_SAN):
	mkdir -p $@

$(GCC_OBJDIR) $(CLANG_OBJDIR) $(ASAN_OBJDIR) $(STRESS_OBJDIR) $(BENCHMARK_DIR):
	mkdir -p $@

# Per-TU objects + dependency files for incremental builds
$(GCC_OBJDIR)/%.o: tests/%.cpp $(TGEN_HEADER) $(TEST_HEADERS) | $(GCC_OBJDIR)
	$(CXX_GCC) $(CXXFLAGS_GCC) $(WERROR_FLAG) -MMD -MP -c $< -o $@

$(CLANG_OBJDIR)/%.o: tests/%.cpp $(TGEN_HEADER) $(TEST_HEADERS) | $(CLANG_OBJDIR)
	$(CXX_CLANG) $(CXXFLAGS_CLANG) $(WERROR_FLAG) -MMD -MP -c $< -o $@

$(ASAN_OBJDIR)/%.o: tests/%.cpp $(TGEN_HEADER) $(TEST_HEADERS) | $(ASAN_OBJDIR)
	$(CXX_ASAN) $(CXXFLAGS_ASAN) $(WERROR_FLAG) -MMD -MP -c $< -o $@

$(STRESS_OBJDIR)/%.o: tests/%.cpp $(TGEN_HEADER) $(TEST_HEADERS) | $(STRESS_OBJDIR)
	$(CXX_GCC) $(CXXFLAGS_STRESS) $(WERROR_FLAG) -MMD -MP -c $< -o $@

$(GCC_OBJDIR)/test: $(GCC_OBJS)
	$(CXX_GCC) $(CXXFLAGS_GCC) $(WERROR_FLAG) $^ $(GTEST_LIBS) -o $@

$(CLANG_OBJDIR)/test: $(CLANG_OBJS)
	$(CXX_CLANG) $(CXXFLAGS_CLANG) $(WERROR_FLAG) $^ $(GTEST_LIBS) -o $@

$(ASAN_OBJDIR)/test: $(ASAN_OBJS)
	$(CXX_ASAN) $(CXXFLAGS_ASAN) $(WERROR_FLAG) $^ $(GTEST_LIBS) -o $@

$(STRESS_OBJDIR)/test: $(STRESS_OBJS)
	$(CXX_GCC) $(CXXFLAGS_STRESS) $(WERROR_FLAG) $^ $(GTEST_LIBS) -o $@

-include $(GCC_OBJS:.o=.d)
-include $(CLANG_OBJS:.o=.d)
-include $(ASAN_OBJS:.o=.d)
-include $(STRESS_OBJS:.o=.d)

test: $(GCC_OBJDIR)/test
	$(call RUN_GTEST,$(GCC_OBJDIR)/test)
	@rm -f $(GCC_OBJDIR)/test

test_clang: $(CLANG_OBJDIR)/test
	$(call RUN_GTEST,$(CLANG_OBJDIR)/test)
	@rm -f $(CLANG_OBJDIR)/test

test_asan: $(ASAN_OBJDIR)/test
	$(call RUN_GTEST,$(ASAN_OBJDIR)/test)
	@rm -f $(ASAN_OBJDIR)/test

stresstest: $(STRESS_OBJDIR)/test
	@bash -c '\
	log="$(BUILD_ROOT)/stresstest_last.log"; \
	seed=$$(( (RANDOM << 45) ^ (RANDOM << 30) ^ (RANDOM << 15) ^ RANDOM )); \
	iteration=0; \
	printf "[stresstest] starting at seed %s...\n" "$$seed"; \
	while true; do \
		printf "[stresstest] running iteration %s (seed %s)...\n" \
			"$$iteration" "$$seed"; \
		TGEN_STRESS_SEED="$$seed" "$(STRESS_OBJDIR)/test" $(GTEST_ARGS) \
			>"$$log" 2>&1; \
		status=$$?; \
		if [ $$status -ne 0 ]; then \
			printf "\n[stresstest] failed at seed=%s\n" "$$seed" >&2; \
			printf "\n%s\n" "Test output:" >&2; \
			cat "$$log" >&2; \
			failed_re="^\[  FAILED  \] [A-Za-z0-9_]+\.[A-Za-z0-9_]+"; \
			failed_tests=$$(grep -E "$$failed_re" "$$log" \
				| sed "s/^\[  FAILED  \] //; s/ ([0-9]* ms)$$//" \
				| sort -u); \
			printf "\n%s\n" "Failed test(s):" >&2; \
			printf "%s\n" "$$failed_tests" | sed "s/^/  /" >&2; \
			first_failed=$$(printf "%s\n" "$$failed_tests" | sed -n "1p"); \
			printf "\n%s\n" "Reproduce command:" >&2; \
			printf "  TGEN_STRESS_SEED=%s %s --gtest_filter=%s\n" \
				"$$seed" "$(STRESS_OBJDIR)/test" "$$first_failed" >&2; \
			exit $$status; \
		fi; \
		iteration=$$((iteration + 1)); \
		seed=$$((seed + 1)); \
	done'

$(BENCHMARK_BIN): benchmarks/main.cpp benchmarks/benchmark.h $(TGEN_HEADER) | $(BENCHMARK_DIR)
	$(CXX_GCC) $(BENCHMARK_CXXFLAGS) -I $(SRC_DIR) benchmarks/main.cpp -o $@

benchmark: $(BENCHMARK_BIN)
	$< --json $(BENCHMARK_JSON)
	@echo "Wrote $(BENCHMARK_JSON). Run 'make doc' to render $(BENCHMARK_HTML)."

benchmark-baseline-local: $(BENCHMARK_BIN)
	$< --update-baseline $(BENCHMARK_LOCAL_BASELINE)
	@echo "Wrote $(BENCHMARK_LOCAL_BASELINE) (gitignored)."

benchmark-baseline-local-auto:
	@if [ -f "$(BENCHMARK_LOCAL_BASELINE)" ]; then \
		printf '%s\n' "Using existing $(BENCHMARK_LOCAL_BASELINE)."; \
		exit 0; \
	fi; \
	bash -c '\
	set -eu; \
	wt="$(BENCHMARK_BASELINE_WORKTREE)"; \
	baseline="$(abspath $(BENCHMARK_LOCAL_BASELINE))"; \
	printf "%s\n" "Missing $(BENCHMARK_LOCAL_BASELINE)."; \
	printf "%s\n" "Building clean benchmark baseline from HEAD..."; \
	mkdir -p "$(BUILD_ROOT)"; \
	git worktree remove --force "$$wt" >/dev/null 2>&1 || true; \
	git worktree add --detach "$$wt" HEAD >/dev/null; \
	cleanup() { \
		git worktree remove --force "$$wt" >/dev/null 2>&1 || true; \
	}; \
	trap cleanup EXIT; \
	$(MAKE) -C "$$wt" benchmark-baseline-local; \
	cp "$$wt/$(BENCHMARK_LOCAL_BASELINE)" "$$baseline"; \
	printf "%s\n" "Wrote $(BENCHMARK_LOCAL_BASELINE) from clean HEAD."'

benchmark-baseline-ci: $(BENCHMARK_BIN)
	$< --smoke --update-baseline benchmarks/ci_baseline.json
	@echo "Wrote benchmarks/ci_baseline.json."

benchmark-check-local: $(BENCHMARK_BIN) benchmark-baseline-local-auto
	@bash -c '\
	set -u; \
	set -o pipefail; \
	out="$(BUILD_ROOT)/benchmark_check_local.log"; \
	printf "%s\n" "Running local benchmark check (this can take a while)..."; \
	if "$(BENCHMARK_BIN)" --check "$(BENCHMARK_LOCAL_BASELINE)" \
		--threshold "$(BENCHMARK_THRESHOLD)" \
		--json "$(BENCHMARK_LOCAL_CURRENT)" 2>&1 | tee "$$out"; then \
		exit 0; \
	fi; \
	printf "\n%s\n" "Benchmark changed significantly."; \
	if [ -r /dev/tty ] && [ -w /dev/tty ]; then \
		printf "%s" "Was this performance change intentional? [y/N] " \
			>/dev/tty; \
		read answer </dev/tty; \
		case "$$answer" in \
			y|Y|yes|YES) \
				printf "%s\n" "Continuing with acknowledged benchmark change."; \
				printf "%s\n" \
					"Refresh local baseline with: make benchmark-baseline-local"; \
				exit 0; \
				;; \
		esac; \
	fi; \
	printf "%s\n" "Benchmark check failed."; \
	printf "%s%s\n" \
		"Investigate the regression or run make benchmark-baseline-local " \
		"if intentional."; \
	exit 1'

# CI: smoke run (crash test) + smoke regression vs ci_baseline.json (ubuntu-latest timings).
benchmark-check: $(BENCHMARK_BIN)
	$< --smoke --json /tmp/tgen_bench_smoke.json
	$< --smoke --check benchmarks/ci_baseline.json --threshold 2.0 --json /tmp/tgen_bench_ci.json

# Pre-commit gate: formatting, tests, local benchmark regression.
check:
	@printf '%s\n' 'Running pre-commit checks (lint, test, benchmark)...' ''
	@$(MAKE) lint-check || { printf '\n%s\n' 'Fix: make lint'; exit 1; }
	@$(MAKE) test WERROR=1 || { printf '\n%s\n' 'Fix: compiler warnings (-Werror) or failed test(s) above'; exit 1; }
	@$(MAKE) benchmark-check-local || { \
		printf '\n%s\n' \
			'Fix: investigate regression, or run make benchmark-baseline-local'; \
		exit 1; \
	}
	@printf '\n%s\n' 'All checks passed — ok to commit.'

# XML-only Doxygen pass + llms markdown. Benchmark HTML is rendered in `doc` (needs XML).
doc-prepare:
	mkdir -p $(DOC_HTML_DIR)
	@printf '%s\n' '<div id="benchmark-results"></div>' > $(BENCHMARK_HTML)
	cd $(DOC_SRC_DIR) && { \
	printf '%s\n' '@INCLUDE = Doxyfile' 'GENERATE_HTML=NO' 'GENERATE_XML=YES' \
		'GENERATE_LATEX=NO' 'GENERATE_DOCBOOK=NO' \
		'OUTPUT_DIRECTORY=build' 'XML_OUTPUT=xml' \
		'NUM_PROC_THREADS = $(NPROCS)' 'DOT_NUM_THREADS = $(NPROCS)'; \
	} | doxygen -
	python3 $(DOC_SRC_DIR)/llms_gen.py \
		--xml $(DOC_BUILD_DIR)/xml \
		--out $(DOC_HTML_DIR) \
		--base-url '$(LLM_BASE_URL)'

llms: doc-prepare

# Incremental by default (no rm -rf); use `make doc-rebuild` or `make clean-doc doc` for a full wipe.
doc:
	@doc_start=$$(date +%s); \
	$(MAKE) doc-prepare && \
	cp $(THEME_DIR)/*.css $(THEME_DIR)/*.js $(DOC_HTML_DIR)/ && \
	cp $(DOC_SRC_DIR)/custom.css $(DOC_SRC_DIR)/header.html $(DOC_SRC_DIR)/layout.xml \
		$(DOC_SRC_DIR)/tgen_logo_white.svg $(DOC_SRC_DIR)/tgen_logo_black.svg \
		$(DOC_SRC_DIR)/tgen_logo_white_small.svg $(DOC_SRC_DIR)/tgen_logo_black_small.svg \
		$(DOC_SRC_DIR)/favicon.svg $(DOC_HTML_DIR)/ && \
	touch $(DOC_HTML_DIR)/.nojekyll && \
	python3 $(DOC_SRC_DIR)/benchmark_render.py \
		--json $(BENCHMARK_JSON) \
		--xml $(DOC_BUILD_DIR)/xml \
		--out $(BENCHMARK_HTML) && \
	rm -rf $(DOC_BUILD_DIR)/xml && \
	cd $(DOC_SRC_DIR) && { \
	printf '%s\n' '@INCLUDE = Doxyfile' 'NUM_PROC_THREADS = $(NPROCS)' 'DOT_NUM_THREADS = $(NPROCS)'; \
	} | doxygen - && \
	doc_secs=$$(($$(date +%s) - doc_start)); \
	printf 'Doc built in %ds.\n' "$$doc_secs"

# Two separate sub-makes (not `$(MAKE) clean-doc doc`): `MAKEFLAGS += -j$(NPROCS)`
# forces the sub-make parallel, so passing both goals at once lets clean-doc's
# `rm -rf docs/build` race the doc recipe writing into docs/build. The `&&`
# guarantees the wipe finishes before the build starts.
doc-rebuild:
	$(MAKE) clean-doc && $(MAKE) doc

clean-doc:
	rm -rf $(DOC_BUILD_DIR)

opendoc:
	@test -f '$(DOC_INDEX)' || { printf '%s\n' "opendoc: missing $(DOC_INDEX) - run 'make doc' first." >&2; exit 1; }
	@{ \
	if command -v xdg-open >/dev/null 2>&1; then \
		( xdg-open '$(DOC_INDEX)' </dev/null >/dev/null 2>&1 & ); \
	elif command -v google-chrome-stable >/dev/null 2>&1; then \
		( google-chrome-stable 'file://$(DOC_INDEX)' </dev/null >/dev/null 2>&1 & ); \
	elif command -v google-chrome >/dev/null 2>&1; then \
		( google-chrome 'file://$(DOC_INDEX)' </dev/null >/dev/null 2>&1 & ); \
	elif command -v chromium >/dev/null 2>&1; then \
		( chromium 'file://$(DOC_INDEX)' </dev/null >/dev/null 2>&1 & ); \
	elif command -v chromium-browser >/dev/null 2>&1; then \
		( chromium-browser 'file://$(DOC_INDEX)' </dev/null >/dev/null 2>&1 & ); \
	else \
		printf '%s\n' "opendoc: no xdg-open or Chrome/Chromium on PATH. Open in a browser:" >&2; \
		printf '%s\n' "  file://$(DOC_INDEX)" >&2; \
		exit 1; \
	fi; \
	}

lint:
	find $(SRC_DIR) examples tests benchmarks \( -name '*.h' -o -name '*.cpp' \) -print0 | xargs -0 clang-format -i

lint-check:
	@echo "Checking formatting..."
	@find $(SRC_DIR) examples tests benchmarks \( -name '*.h' -o -name '*.cpp' \) -print0 | \
	xargs -0 clang-format --dry-run --Werror || \
	( echo ""; echo "Run 'make lint' to fix formatting"; exit 1 )
	@echo "Formatting check passed"

sample: $(EXAMPLE_OUT)/sample
	$< $(ARGS)

graph: $(EXAMPLE_OUT)/graph
	$< $(ARGS)

geometry: $(EXAMPLE_OUT)/geometry
	@mkdir -p $(BUILD_ROOT)
	$< $(ARGS) | tee $(GEOMETRY_OUT)
	python3 examples/geometry_plot.py -o $(GEOMETRY_HTML) < $(GEOMETRY_OUT)
	@echo "Wrote $(GEOMETRY_HTML)"

range_queries: $(EXAMPLE_OUT)/range_queries

sample_debug: $(EXAMPLE_OUT)/sample_debug

# Shorthand: graph-asan -> make graph ASAN=1 (jobserver preserved; ARGS is forwarded automatically)
%-asan:
	+@$(MAKE) $(@:-asan=) ASAN=1

help:
	@echo "Tests:     make test | test_clang | test_asan | stresstest"
	@echo "Examples:  make graph | geometry | sample | range_queries | sample_debug"
	@echo " + geometry writes $(GEOMETRY_HTML) (stdout + plot)"
	@echo " + argv:   make graph ARGS='…'   (sample, graph, graph-asan, …)"
	@echo " + ASan:   make graph ASAN=1   or   make graph-asan   (same for sample-asan, …)"
	@echo "           Binaries: build/examples/rel/… and build/examples/asan/…"
	@echo "Benchmark: make benchmark | benchmark-baseline-local | benchmark-baseline-ci"
	@echo "           make benchmark-check-local | benchmark-check"
	@echo " + benchmark writes $(BENCHMARK_JSON); make doc renders $(BENCHMARK_HTML)"
	@echo "Checks:    make check   (lint + test WERROR=1 + local benchmark check)"
	@echo "Docs:      make doc | doc-rebuild | opendoc | clean-doc | llms"
	@echo "           doc also builds llms; doc-rebuild = clean + doc"
	@echo "Other:     make lint | lint-check | clean | cloc | print-CXXFLAGS_COMMON"

print-%:
	@echo '$* = $($*)'

cloc: clean
	cloc --by-file single_include examples tests docs/custom.css docs/header.html

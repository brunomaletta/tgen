.DELETE_ON_ERROR:

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

CXX_GCC   := $(if $(CCACHE),ccache )g++
CXX_CLANG := $(if $(CCACHE),ccache )clang++
CXX_ASAN  := $(CXX_GCC)

CXXFLAGS_COMMON := -std=c++17 -Wall -Wshadow -pthread
CXXFLAGS_GCC    := $(CXXFLAGS_COMMON) -O2
CXXFLAGS_CLANG  := $(CXXFLAGS_COMMON) -O2
CXXFLAGS_ASAN   := $(CXXFLAGS_COMMON) -g -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer

GTEST_LIBS := -lgtest -lgtest_main

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
$(eval $(call EXAMPLE_LINK_RULE,unordered,unordered_set.cpp,$(CXX_GCC)))
$(eval $(call EXAMPLE_LINK_RULE,range_queries,range_queries.cpp,$(CXX_GCC)))
$(eval $(call EXAMPLE_LINK_RULE,unordered_clang,unordered_set.cpp,$(CXX_CLANG)))
$(eval $(call EXAMPLE_DEBUG_RULE,sample_debug,all.cpp,$(CXX_GCC)))

GEOMETRY_HTML := $(BUILD_ROOT)/geometry.html
GEOMETRY_OUT  := $(BUILD_ROOT)/geometry.out

.PHONY: all doc llms doc-rebuild clean-doc opendoc lint lint-check test test_clang test_asan \
	sample sample_debug unordered range_queries unordered_clang graph geometry cloc \
	help print-% %-asan

all: lint doc test

clean:
	-rm -rf $(BUILD_ROOT)

$(EXAMPLE_REL) $(EXAMPLE_SAN):
	mkdir -p $@

$(GCC_OBJDIR) $(CLANG_OBJDIR) $(ASAN_OBJDIR):
	mkdir -p $@

# Per-TU objects + dependency files for incremental builds
$(GCC_OBJDIR)/%.o: tests/%.cpp $(TGEN_HEADER) $(TEST_HEADERS) | $(GCC_OBJDIR)
	$(CXX_GCC) $(CXXFLAGS_GCC) -MMD -MP -c $< -o $@

$(CLANG_OBJDIR)/%.o: tests/%.cpp $(TGEN_HEADER) $(TEST_HEADERS) | $(CLANG_OBJDIR)
	$(CXX_CLANG) $(CXXFLAGS_CLANG) -MMD -MP -c $< -o $@

$(ASAN_OBJDIR)/%.o: tests/%.cpp $(TGEN_HEADER) $(TEST_HEADERS) | $(ASAN_OBJDIR)
	$(CXX_ASAN) $(CXXFLAGS_ASAN) -MMD -MP -c $< -o $@

$(GCC_OBJDIR)/test: $(GCC_OBJS)
	$(CXX_GCC) $(CXXFLAGS_GCC) $^ $(GTEST_LIBS) -o $@

$(CLANG_OBJDIR)/test: $(CLANG_OBJS)
	$(CXX_CLANG) $(CXXFLAGS_CLANG) $^ $(GTEST_LIBS) -o $@

$(ASAN_OBJDIR)/test: $(ASAN_OBJS)
	$(CXX_ASAN) $(CXXFLAGS_ASAN) $^ $(GTEST_LIBS) -o $@

-include $(GCC_OBJS:.o=.d)
-include $(CLANG_OBJS:.o=.d)
-include $(ASAN_OBJS:.o=.d)

test: $(GCC_OBJDIR)/test
	$<
	@rm -f $<

test_clang: $(CLANG_OBJDIR)/test
	$<
	@rm -f $<

test_asan: $(ASAN_OBJDIR)/test
	-$<
	@rm -f $<

# Incremental by default (no rm -rf); use `make doc-rebuild` or `make clean-doc doc` for a full wipe.
doc:
	cd $(DOC_SRC_DIR) && { \
	printf '%s\n' '@INCLUDE = Doxyfile' 'NUM_PROC_THREADS = $(NPROCS)' 'DOT_NUM_THREADS = $(NPROCS)'; \
	} | doxygen -

	# copy theme assets into html root
	cp $(THEME_DIR)/*.css $(THEME_DIR)/*.js $(DOC_HTML_DIR)/

	# project assets
	cp $(DOC_SRC_DIR)/custom.css $(DOC_SRC_DIR)/header.html $(DOC_SRC_DIR)/layout.xml \
		$(DOC_SRC_DIR)/tgen_logo_white.svg $(DOC_SRC_DIR)/tgen_logo_black.svg \
		$(DOC_SRC_DIR)/tgen_logo_white_small.svg $(DOC_SRC_DIR)/tgen_logo_black_small.svg \
		$(DOC_SRC_DIR)/favicon.svg $(DOC_HTML_DIR)/

	# GitHub Pages safety
	touch $(DOC_HTML_DIR)/.nojekyll

	$(MAKE) llms

# LLM-friendly markdown docs: a second, XML-only Doxygen run + a python converter.
# Leaves the HTML build untouched. Output: $(DOC_HTML_DIR)/llms.txt and $(DOC_HTML_DIR)/llms/*.md
llms:
	mkdir -p $(DOC_HTML_DIR)
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
	rm -rf $(DOC_BUILD_DIR)/xml

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
	find $(SRC_DIR) examples tests \( -name '*.h' -o -name '*.cpp' \) -print0 | xargs -0 clang-format -i

lint-check:
	@echo "Checking formatting..."
	@find $(SRC_DIR) examples tests \( -name '*.h' -o -name '*.cpp' \) -print0 | \
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

unordered: $(EXAMPLE_OUT)/unordered

range_queries: $(EXAMPLE_OUT)/range_queries

unordered_clang: $(EXAMPLE_OUT)/unordered_clang

sample_debug: $(EXAMPLE_OUT)/sample_debug

# Shorthand: graph-asan -> make graph ASAN=1 (jobserver preserved; ARGS is forwarded automatically)
%-asan:
	+@$(MAKE) $(@:-asan=) ASAN=1

help:
	@echo "Tests:     make test | test_clang | test_asan"
	@echo "Examples:  make graph | geometry | sample | unordered | range_queries | unordered_clang | sample_debug"
	@echo " + geometry writes $(GEOMETRY_HTML) (stdout + plot)"
	@echo " + argv:   make graph ARGS='…'   (sample, graph, graph-asan, …)"
	@echo " + ASan:   make graph ASAN=1   or   make graph-asan   (same for sample-asan, unordered-asan, …)"
	@echo "           Binaries: build/examples/rel/… and build/examples/asan/… (see ASAN=1)"
	@echo "Docs:      make doc | doc-rebuild | opendoc | clean-doc | llms   (doc also builds llms; doc-rebuild = clean + doc)"
	@echo "Other:     make lint | lint-check | clean | cloc | print-CXXFLAGS_COMMON"

print-%:
	@echo '$* = $($*)'

cloc: clean
	cloc --by-file single_include examples tests docs/custom.css docs/header.html

SRC_DIR       := single_include
DOC_BUILD_DIR := docs/build
DOC_HTML_DIR  := $(DOC_BUILD_DIR)
DOC_SRC_DIR   := docs
THEME_DIR     := docs/doxygen-awesome-css

all: lint doc test

clean:
	-.rm -f test sample sample_debug

sample:
	@g++ -std=c++17 examples/all.cpp -I $(SRC_DIR) -o sample
	@-./sample
	@rm -f sample

.PHONY: doc clean-doc

doc: clean-doc
	cd $(DOC_SRC_DIR) && doxygen Doxyfile

	# copy theme assets into html root
	cp $(THEME_DIR)/*.css $(DOC_HTML_DIR)/
	cp $(THEME_DIR)/*.js  $(DOC_HTML_DIR)/

	# project assets
	cp $(DOC_SRC_DIR)/custom.css $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/header.html $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/layout.xml $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/tgen_logo_white.svg $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/tgen_logo_black.svg $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/tgen_logo_white_small.svg $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/tgen_logo_black_small.svg $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/favicon.svg $(DOC_HTML_DIR)/

	# GitHub Pages safety
	touch $(DOC_HTML_DIR)/.nojekyll

clean-doc:
	rm -rf $(DOC_BUILD_DIR)

opendoc:
	google-chrome docs/build/index.html > /dev/null 2>&1 &

lint:
	find $(SRC_DIR) examples tests \( -name '*.h' -o -name '*.cpp' \) -print0 | xargs -0 clang-format -i

lint-check:
	@echo "Checking formatting..."
	@find $(SRC_DIR) examples tests \( -name '*.h' -o -name '*.cpp' \) -print0 | \
	xargs -0 clang-format --dry-run --Werror || \
	( echo ""; echo "Run 'make lint' to fix formatting"; exit 1 )
	@echo "Formatting check passed"

test: single_include/* tests/*
	g++ -std=c++17 tests/*.cpp -lgtest -lgtest_main -pthread -o test -Wall -Wshadow -O2
	./test
	rm -f test

testas:
	g++ -std=c++17 tests/*.cpp -lgtest -lgtest_main -pthread -o test -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -g -Wall -Wshadow
	-./test
	rm -f test

debug:
	g++ -g examples/all.cpp -o examples/sample_debug

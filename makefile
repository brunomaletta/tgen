all: lint doc test

a:
	@g++ -std=c++17 a.cpp -I src -o a -O2
	@-./a
	@rm -f a

debug:
	g++ -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -g -Wall -Wshadow -std=c++17 a.cpp -I src -o a

DOC_BUILD_DIR := docs/build
DOC_HTML_DIR  := $(DOC_BUILD_DIR)
DOC_SRC_DIR   := docs
THEME_DIR     := docs/doxygen-awesome-css

.PHONY: doc clean-doc

doc: clean-doc
	cd $(DOC_SRC_DIR) && doxygen Doxyfile

	@# ensure directory exists even if doxygen changes settings
	mkdir -p $(DOC_HTML_DIR)

	# copy theme assets into html root
	cp $(THEME_DIR)/*.css $(DOC_HTML_DIR)/
	cp $(THEME_DIR)/*.js  $(DOC_HTML_DIR)/

	# project assets
	cp $(DOC_SRC_DIR)/custom.css $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/header.html $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/layout.xml $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/tgen_white.svg $(DOC_HTML_DIR)/
	cp $(DOC_SRC_DIR)/tgen_white_small.svg $(DOC_HTML_DIR)/

	# GitHub Pages safety
	touch $(DOC_HTML_DIR)/.nojekyll

clean-doc:
	rm -rf $(DOC_BUILD_DIR)

opendoc:
	google-chrome docs/build/index.html > /dev/null 2>&1 &

lint:
	find a.cpp src tests \( -name '*.h' -o -name '*.cpp' \) -print0 | xargs -0 clang-format -i

lint-check:
	@echo "Checking formatting..."
	@find a.cpp src tests \( -name '*.h' -o -name '*.cpp' \) -print0 | \
	xargs -0 clang-format --dry-run --Werror || \
	( echo ""; echo "Run 'make lint' to fix formatting"; exit 1 )
	@echo "Formatting check passed"

test:
	g++ -std=c++17 tests/*.cpp -lgtest -lgtest_main -pthread -I src -o test
	./test
	rm -f test

testas:
	g++ -std=c++17 tests/*.cpp -lgtest -lgtest_main -pthread -I src -o test -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -g -Wall -Wshadow
	-./test
	rm -f test

clean:
	rm -f a

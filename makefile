all: lint doc test

a:
	g++ -std=c++17 a.cpp -I src -o a -O2
	-./a
	rm -r a

debug:
	g++ -fsanitize=address,undefined -fno-omit-frame-pointer -g -Wall -Wshadow -std=c++17 -Wno-unused-result -Wno-sign-compare -Wno-char-subscripts a.cpp -I src -o a

doc:
	doxygen docs/Doxyfile

opendoc:
	open docs/html/index.html > /dev/null 2>&1 &

dropdoc:
	git checkout -- docs

lint:
	find a.cpp src/* tests/*.cpp -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i

test:
	g++ -std=c++17 tests/*.cpp -lgtest -lgtest_main -pthread -I src -o test
	-./test
	rm -r test

clean:
	rm -r a

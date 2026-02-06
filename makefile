all:
	g++ -std=c++17 a.cpp -I src -o a -O2
	./a
	rm -rf a

as:
	g++ -fsanitize=address,undefined -fno-omit-frame-pointer -g -Wall -Wshadow -std=c++17 -Wno-unused-result -Wno-sign-compare -Wno-char-subscripts a.cpp -I src -o a

doc:
	doxygen docs/Doxyfile

docs: doc

opendoc:
	open docs/html/index.html > /dev/null 2>&1 &

lint:
	find a.cpp src/* -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i

test:
	g++ -std=c++17 tests/test.cpp -lgtest -lgtest_main -o test
	./test
	rm test

clean:
	rm -rf a

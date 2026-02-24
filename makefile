all: lint doc test

a:
	@g++ -std=c++17 a.cpp -I src -o a -O2
	@-./a
	@rm -f a

debug:
	g++ -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -g -Wall -Wshadow -std=c++17 a.cpp -I src -o a

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
	rm -f test

testas:
	g++ -std=c++17 tests/*.cpp -lgtest -lgtest_main -pthread -I src -o test -fsanitize=address,undefined,float-cast-overflow -fno-omit-frame-pointer -g -Wall -Wshadow
	-./test
	rm -f test

clean:
	rm -f a

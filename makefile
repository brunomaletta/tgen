all:
	g++ a.cpp -I src -o a

as:
	g++ -fsanitize=address,undefined -fno-omit-frame-pointer -g -Wall -Wshadow -std=c++17 -Wno-unused-result -Wno-sign-compare -Wno-char-subscripts a.cpp -I src -o a

clean:
	rm a

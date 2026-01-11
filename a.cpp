#include "tgen.h"

#include <iostream>

using namespace std;

int main(int argc, char** argv) {
	tgen::registerGen(argc, argv);

	int max_c = tgen::opt<int>("c");

	int a = tgen::rnd.next(0, max_c+1);
	int b = tgen::rnd.next(0, max_c+1);

	cout << a << " " << b << endl;
}

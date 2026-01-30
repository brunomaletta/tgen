#include "tgen.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char** argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n");

	cout << tgen::next(1, 10) << endl;

	vector<int> v(n);
	iota(v.begin(), v.end(), 0);

	tgen::shuffle(v.begin(), v.end());
	for (int i : v) cout << i << " ";
	cout << endl;

	cout << tgen::any(v.begin(), v.end()) << endl;

	cout << tgen::opt<string>("x", "empty") << endl;
}

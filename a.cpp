#include "tgen.h"

#include <iostream>
#include <vector>

using namespace std;

int main(int argc, char** argv) {
	tgen::register_gen(argc, argv);

	int n = tgen::opt<int>("n");

	cout << tgen::rnd.next(1, 10) << endl;

	vector<int> v(n);
	iota(v.begin(), v.end(), 0);

	tgen::rnd.shuffle(v.begin(), v.end());
	for (int i : v) cout << i << " ";
	cout << endl;

	cout << tgen::rnd.any(v.begin(), v.end()) << endl;
}

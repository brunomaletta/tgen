#include "../single_include/tgen.h"

#include <bits/stdc++.h>

using namespace std;
using namespace tgen;

int main() {
	register_gen();

	cout << tgen::pair<int>(1, 10).leq().gen_list(5).separator('\n') << endl;
}
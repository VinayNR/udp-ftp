#include <iostream>
#include <cstdlib> // for std::strtoul
#include <cstring> // for strlen

using namespace std;

int main() {
    char x = 'A';

    do {
        cout << "Hi" << endl;
        cin >> x;
    } while (x == 'A');

    return 0;
}

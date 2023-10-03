#include <iostream>

using namespace std;

int main() {
    const char *a, *b;
    a = new char[10];
    b = new char[10];
    strcpy(b, "hi");
    cout << a << " : " << b << endl;
    cout << sizeof(a) << " : " << sizeof(b) << endl;
    return 0;
}
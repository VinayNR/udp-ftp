#include <iostream>

using namespace std;

void func(char * b) {
    cout << strlen(b) << endl;
}

int main() {
    char a[100];
    cout << strlen(a) << endl;
    func(a);
    return 0;
}
#include <iostream>

using namespace std;

int main() {
    char * a;
    a = new char[3];

    strcpy(a, "ls");
    a[strlen(a)] = '\0';

    cout << a << " : " << strlen(a) << endl;

    
    
    return 0;
}

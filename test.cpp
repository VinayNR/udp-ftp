#include <iostream>
#include <cstdlib> // for std::strtoul
#include <cstring> // for strlen

int main() {
    const char* str = "12345"; // Replace with your character array

    // Use std::strtoul to convert the character array to uint32_t
    char* endptr;
    uint32_t result = std::strtoul(str, &endptr, 10); // 10 specifies base 10

    // Check for conversion errors
    if (*endptr != '\0') {
        std::cerr << "Conversion error: Not a valid uint32_t." << std::endl;
    } else {
        std::cout << "Converted value: " << result << std::endl;
    }

    return 0;
}

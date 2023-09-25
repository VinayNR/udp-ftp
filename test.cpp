#include <iostream>
#include <fstream>

void read(const char* filename, char*& response) {
    std::ifstream filestream(filename, std::ios::binary | std::ios::ate);

    if (!filestream.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        response = nullptr;
        return;
    }

    int fileSize = filestream.tellg();
    filestream.seekg(0, std::ios::beg);

    // Create a buffer with room for the file content and null terminator
    response = new char[fileSize + 1];

    if (!filestream.read(response, fileSize)) {
        std::cerr << "Error reading file: " << filename << std::endl;
        delete[] response;
        response = nullptr;
        return;
    }

    // Null-terminate the string
    response[fileSize] = '\0';

    filestream.close();
}

int main() {
    char* response = nullptr;
    read("test.txt", response);

    if (response != nullptr) {
        std::cout << "File Size: " << strlen(response) << " bytes" << std::endl;
        std::cout << "File Content: " << response << std::endl;
        delete[] response;
    }

    return 0;
}

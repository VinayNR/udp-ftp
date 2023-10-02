#include "file_ops.h"

#include <dirent.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

void readCurrentDirectory(const char * directory, char *& contents) {
    DIR * dir = opendir(directory);
    
    if (dir) {
        vector<string> fileNames;

        struct dirent* entry;
        while ((entry = readdir(dir))) {
            if (entry->d_type == DT_REG) { // Check if it's a regular file
                fileNames.push_back(entry->d_name);
            }
        }

        closedir(dir);

        // Create a char buffer and concatenate the file names
        string concatenatedFileNames;
        for (const string& fileName : fileNames) {
            concatenatedFileNames += fileName + '\n';
        }

        // Copy the concatenated file names to a char buffer
        contents = new char[concatenatedFileNames.size() + 1];
        memset(contents, 0, concatenatedFileNames.size() + 1);

        strcpy(contents, concatenatedFileNames.c_str());
    }
    else {
        contents = new char[2];
        memset(contents, 0, 2);
        strcpy(contents, " ");
    }
}

int getFile(const char * filename, char *& fileContents) {
    cout << "In get file" << endl;
    ifstream filestream (filename, ios::binary | ios::ate);
    if (!filestream.is_open()) {
        cout << "Unable to open file" << endl;
        return 1;
    }

    int fileSize = filestream.tellg();
    cout << "File size: " << fileSize << endl;
    filestream.seekg(0, ios::beg);

    fileContents = new char[fileSize + 1];
    memset(fileContents, 0, fileSize + 1);

    char contents[fileSize+1];

    cout << "sizeof: " << strlen(fileContents) << endl;

    if (!filestream.read(contents, fileSize)) {
        cout << "Unable to read file" << endl;
        return 1;
    }

    std::streamsize bytesRead = filestream.gcount();
    cout << "Bytes read: " << bytesRead << endl;
    cout << "File contents: " << strlen(contents) << endl;

    filestream.close();
    return 0;
}

int putFile(const char * filename, char * fileContents) {
    ofstream outFile(filename);
    if (!outFile.is_open()) {
        return 1;
    }

    // Write the contents to the file
    outFile << fileContents; 

    if (outFile.fail()) {
        return 1;
    }

    outFile.close();
    return 0;
}

int deleteFile(const char * filename) {
    return remove(filename);
}
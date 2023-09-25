#include "file_ops.h"

#include <dirent.h>
#include <vector>
#include <string>
#include <fstream>

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
        memset(contents, 0, sizeof(contents));

        strcpy(contents, concatenatedFileNames.c_str());

        // null terminate
        contents[strlen(contents)] = '\0';
    }
    else {
        contents = new char[2];
        strcpy(contents, " ");
    }
}

int getFile(const char * filename, char *& fileContents) {
    ifstream filestream (filename, ios::binary | ios::ate);
    if (!filestream.is_open()) {
        return 1;
    }

    int fileSize = filestream.tellg();
    filestream.seekg(0, ios::beg);

    fileContents = new char[fileSize + 1];
    memset(fileContents, 0, sizeof(fileContents));

    if (!filestream.read(fileContents, fileSize)) {
        return 1;
    }

    // Null-terminate the string
    fileContents[fileSize] = '\0';

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
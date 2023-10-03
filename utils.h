#ifndef UTILS_H
#define UTILS_H

#include <vector>

uint32_t djb2_hash(const char*, int);

bool validateChecksum(const char *, int, uint32_t);

std::vector<int> getAllDelimiterPos(const char*, char);

// template <typename T>
void deleteAndNullifyPointer(char *&, bool);

#endif

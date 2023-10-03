#ifndef FILE_OPS_H
#define FILE_OPS_H

void readCurrentDirectory(const char *, char *&);

int getFile(const char *, char *&);

int putFile(const char *, char *, int);

int deleteFile(const char *);

#endif
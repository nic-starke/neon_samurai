#pragma once
#include <stddef.h>
void*	 memcpy(void*, const void*, size_t);
void*	 memset(void*, int, size_t);
int		 memcmp(const void*, const void*, size_t);
void*	 memmove(void*, const void*, size_t);
size_t strlen(const char*);
char*	 strcpy(char*, const char*);
char*	 strncpy(char*, const char*, size_t);
int		 strcmp(const char*, const char*);
int		 strncmp(const char*, const char*, size_t);
char*	 strchr(const char*, int);
char*	 strtok(char*, const char*);
int		 strcasecmp(const char*, const char*);
int		 strncasecmp(const char*, const char*, size_t);

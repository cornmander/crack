// Runtime support
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#ifndef _runtime_Util_h_
#define _runtime_Util_h_

#include <stdint.h>

namespace crack { namespace runtime {

char* strerror(void);

int float_str(double, char* buf, unsigned int size);
unsigned int rand(unsigned int low, unsigned int high);
int crk_puts(char *str);
int crk_putc(char byte);
void crk_die(const char *message);
void printfloat(float val);
void printint(int val);
void printint64(int64_t val);
void printuint64(uint64_t val);

int is_file(const char *path);
bool fileExists(const char *path);
int setNonBlocking(int fd, int val);
int setUtimes(const char *path, int64_t atv_usecs, int64_t mtv_usecs, bool now);

char *crk_iconv(unsigned int targetCharSize, const char *to, const char *from, char *string, unsigned int len, unsigned int *convertedLen);

}}

#endif // _runtime_Util_h_

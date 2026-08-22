#pragma once
/* Analysis-only stand-in: avr-libc's pgmspace.h uses inline asm with numeric
	 register clobbers that clang's AVR target rejects. Plain dereferences let
	 the analyzer see through the accesses, which is what we want here. */
#include <stddef.h>
#include <stdint.h>
#define PROGMEM
#define PGM_P									const char*
#define PGM_VOID_P						const void*
#define PSTR(s)								(s)
#define pgm_read_byte(p)			(*(const uint8_t*)(p))
#define pgm_read_byte_near(p) (*(const uint8_t*)(p))
#define pgm_read_byte_far(p)	(*(const uint8_t*)(uintptr_t)(p))
#define pgm_read_word(p)			(*(const uint16_t*)(p))
#define pgm_read_word_near(p) (*(const uint16_t*)(p))
#define pgm_read_word_far(p)	(*(const uint16_t*)(uintptr_t)(p))
#define pgm_read_dword(p)			(*(const uint32_t*)(p))
#define pgm_read_ptr(p)				(*(void* const*)(p))
size_t strlen_P(const char*);
char*	 strcpy_P(char*, const char*);
char*	 strncpy_P(char*, const char*, size_t);
int		 strcmp_P(const char*, const char*);
int		 strncmp_P(const char*, const char*, size_t);
int		 strcasecmp_P(const char*, const char*);
void*	 memcpy_P(void*, const void*, size_t);
int		 snprintf_P(char*, size_t, const char*, ...);
int		 sprintf_P(char*, const char*, ...);
int		 printf_P(const char*, ...);

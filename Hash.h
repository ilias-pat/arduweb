#ifndef HASH_H
#define HASH_H

#include <avr/pgmspace.h>

#define hash_function		hash_djb2
#define hash_function_pgm	hash_djb2_pgm

unsigned long hash_djb2( const char* str );
unsigned long hash_djb2_pgm( const prog_char* str );

#endif

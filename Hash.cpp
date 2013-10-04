#include "Hash.h"

unsigned long hash_djb2( const char* str )
{
    unsigned long hash = 5381;
    int c;

    while( c = *str++ )
        hash = ( ( hash << 5 ) + hash ) + c; /* hash * 33 + c */

    return hash;
}

unsigned long hash_djb2_pgm( const prog_char* str )
{
    unsigned long hash = 5381;
    int c;

    while( c = ( char )pgm_read_byte( str++ ) )
        hash = ( ( hash << 5 ) + hash ) + c; /* hash * 33 + c */

    return hash;
}
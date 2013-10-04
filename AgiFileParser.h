#ifndef AGI_FILE_PARSER_H
#define AGI_FILE_PARSER_H

#include <Client.h>
#include <SD.h>
#include "Hash.h"

#define AGI_COMMAND_SIZE	( 0x0A )
#define AGI_START_DELIM_A	( '$' )
#define AGI_START_DELIM_B	( '(' )
#define AGI_END_DELIM		( ')' )

typedef const char* (*agi_command_cb)(const char* command );

typedef struct
{
	unsigned long id;
	agi_command_cb callback;
} agi_command_entry;

	//agi_command_entry agi_command_entry_##command PROGMEM = { hash_function_pgm( PSTR( #command ) ), callback };
#define DEFINE_AGI_COMMAND( command, callback ) \
	const prog_char agi_command_str_##command[] PROGMEM = #command; \
        unsigned long agi_command_key_##command = hash_function_pgm( agi_command_str_##command ); \
	agi_command_entry agi_command_entry_##command = { agi_command_key_##command, callback };

#define BEGIN_AGI_COMMAND_TABLE \
	agi_command_entry* agi_command_table[] PROGMEM = {

#define AGI_COMMAND( command ) \
	&agi_command_entry_##command,

#define END_AGI_COMMAND_TABLE	\
	NULL };

class AgiFileParser
{
public:
	static void process( File& file, Stream& stream );

private:
	static agi_command_cb getCommandCallback( const char* command );
	
	static char buffer[AGI_COMMAND_SIZE];
	static int index;
	static int state;
};

#endif

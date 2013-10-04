#include "AgiFileParser.h"
#include <avr/pgmspace.h>

extern agi_command_entry* agi_command_table[];

#define WAIT_START_DELIM_A	( 0x00 )
#define WAIT_START_DELIM_B	( 0x01 )
#define WAIT_END_DELIM		( 0x02 )

char AgiFileParser::buffer[AGI_COMMAND_SIZE];
int AgiFileParser::index = 0;
int AgiFileParser::state = WAIT_START_DELIM_A;

void AgiFileParser::process( File& file, Stream& stream )
{
	state = WAIT_START_DELIM_A;
	index = 0;

	while( file && file.available( ) )
	{
		char c = file.read( );

		switch( state )
		{
		case WAIT_START_DELIM_A:
			if( c == AGI_START_DELIM_A )
				state = WAIT_START_DELIM_B;
			else
				stream.write( c );
		break;

		case WAIT_START_DELIM_B:
			if( c == AGI_START_DELIM_B )
				state = WAIT_END_DELIM;
			else
			{
				stream.write( AGI_START_DELIM_A );
				stream.write( c );
				state = WAIT_START_DELIM_A;
			}
		break;

		case WAIT_END_DELIM:
			if( c != AGI_END_DELIM )
				buffer[index++] = c;
			else
			{
				buffer[index] = '\0';
				index = 0;

				agi_command_cb callback = ( agi_command_cb )getCommandCallback( buffer );
				if( callback )
				{
					const char* str = callback( buffer );
					stream.write( ( uint8_t* )str, strlen( str ) );
				}
				state = WAIT_START_DELIM_A;
			}
		break;
		}
	}

	stream.flush( );
}

agi_command_cb AgiFileParser::getCommandCallback( const char* command )
{
	unsigned long id = hash_function( command );
	int index = 0;
	
	agi_command_entry* entry = ( agi_command_entry* )pgm_read_word( &agi_command_table[index] );
	while( entry )
	{
		//if( pgm_read_word( &entry->id ) == id )
		//	return ( agi_command_cb )pgm_read_word( entry->callback );
		
                //entry = ( agi_command_entry* )pgm_read_word( &agi_command_table[++index] );
                if( entry->id == id )
			return ( agi_command_cb )( entry->callback );
                
		entry = ( agi_command_entry* )pgm_read_word( &agi_command_table[++index] );
	}

	return 0;
}

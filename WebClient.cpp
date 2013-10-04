#include "WebClient.h"
#include <avr/pgmspace.h>

#define PROCESS_CONTINUE					( 0x00 )
#define PROCESS_ERROR						( 0x01 )
#define PROCESS_DONE						( 0x02 )

#define STATE_READ_METHOD					( 0x00 )
#define STATE_READ_URL						( 0x01 )
#define STATE_READ_VERSION					( 0x02 )
#define STATE_READ_STATUS					( 0x03 )
#define STATE_READ_STATUS_STR				( 0x04 )
#define STATE_READ_OPTION_KEY				( 0x05 )
#define STATE_READ_OPTION_CONTENT_LENGTH	( 0x06 )
#define STATE_READ_OPTION_MIME_TYPE			( 0x07 )
#define STATE_DISCARD_OPTION_VALUE    ( 0x08 )

WebClient::WebClient( void )
	: EthernetClient( ), mBufferIndex( 0 ), mParserState( STATE_READ_METHOD ), mCurrentLineIsEmpty( true )
{
	memset( mBuffer, 0, WEB_CLIENT_BUFFER_SIZE );
}

WebClient::WebClient( const EthernetClient& client ) 
	: EthernetClient( client ), mBufferIndex( 0 ), mParserState( STATE_READ_METHOD ), mCurrentLineIsEmpty( true )
{
	memset( mBuffer, 0, WEB_CLIENT_BUFFER_SIZE );
}

boolean WebClient::readHTTPReqHeader( struct HTTPReqHeader& header )
{
	int result = PROCESS_CONTINUE;
	mBufferIndex = 0;
	mParserState = STATE_READ_METHOD; // first element of http req.
	Serial.println("req data:");
	while( connected( ) && available( ) && result == PROCESS_CONTINUE )
	{
		char c = read( );
                Serial.print( c );
		result = processHTTPReqHeaderByte( header, c );
	}
	
	if( result != PROCESS_DONE )
	{
		// TODO: flush rx buffer ?
		EthernetClient::flush( );
		return false;
	}

	return true;
}

boolean WebClient::writeHTTPReqHeader( const struct HTTPReqHeader& header )
{
	mBufferIndex = 0;
	
	switch( header.method )
	{
	case HTTP_METHOD_HEAD:
	default:
		write_pgm( PSTR( "HEAD " ) );
		break;
		
	case HTTP_METHOD_GET:
		write_pgm( PSTR( "GET " ) );
		break;
		
	case HTTP_METHOD_POST:
		write_pgm( PSTR( "POST " ) );
		break;
	}
	
	print( header.url );
	
	switch( header.version )
	{	
	case HTTP_VER_1_0:
		write_pgm( PSTR( " HTTP/1.0" ) );
		break;
	
	case HTTP_VER_1_1:
	default:
		write_pgm( PSTR( " HTTP/1.1" ) );
		break;		
	}
	
	if( header.content_length > 0 )
	{
		write_pgm( PSTR( "Content-length: " ) );
		println( header.content_length );
	}

	println( );
	
        flush( );
        
	return true;
}

boolean WebClient::readHTTPRespHeader( struct HTTPRespHeader& header )
{
	int result = PROCESS_CONTINUE;
	mBufferIndex = 0;
	mParserState = STATE_READ_VERSION; // first element of http resp.
	
	while( connected( ) && available( ) && result == PROCESS_CONTINUE )
	{
		char c = read( );
		result = processHTTPRespHeaderByte( header, c );
	}
	
	if( result != PROCESS_DONE )
	{
		// TODO: flush rx buffer ?
		EthernetClient::flush( );
		return false;
	}

	return true;

}

boolean WebClient::writeHTTPRespHeader( const struct HTTPRespHeader& header )
{
	mBufferIndex = 0;
	
	switch( header.version )
	{
	case HTTP_VER_1_0:
		write_pgm( PSTR( "HTTP/1.0 " ) );
		break;
		
	case HTTP_VER_1_1:
	default:	
		write_pgm( PSTR( "HTTP/1.1 " ) );
		break;
	}
	
	switch( header.status )
	{
	case HTTP_STATUS_200_OK:
		write_pgm( PSTR( "200 OK\r\n" ) );
		write_pgm( PSTR( "Pragma: no-cache\r\n" ) );                
		break;
			
	case HTTP_STATUS_400_BAD_REQ:
	default:
		write_pgm( PSTR( "400 Bad Request\r\n" ) );
		write_pgm( PSTR( "Connection: close\r\n" ) );             
		break;
			
	case HTTP_STATUS_404_NOT_FOUND:
		write_pgm( PSTR( "404 Not Found\r\n" ) );
		break;
	}

	if( header.mime_type )
	{
		write_pgm( PSTR( "Content-type: " ) );
		println( header.mime_type );
	}

	if( header.content_length > 0 )
	{
		write_pgm( PSTR( "Content-length: " ) );
		println( header.content_length );
	}

	println( );

	return true;
}

size_t WebClient::write_pgm( const prog_char* str )
{
	const prog_char* ptr = str;
	char c = ( char )pgm_read_byte( ptr++ );
	
	while( c != '\0' )
	{
		write( ( uint8_t )c );
		c = ( char )pgm_read_byte( ptr++ );
	}

	return ( ptr - str - 1 );
}

size_t WebClient::write( uint8_t c )
{
        Serial.print( '[' );
        Serial.print( (char)c ); 
        Serial.print( ']' );        
	mBuffer[mBufferIndex++] = c;
	if( mBufferIndex >= WEB_CLIENT_BUFFER_SIZE )
	{
		EthernetClient::write( ( const uint8_t* )mBuffer, mBufferIndex );
		mBufferIndex = 0;
	}
	return 1;
}


void WebClient::flush( void )
{
	if( mBufferIndex )
	{
		EthernetClient::write( ( const uint8_t* )mBuffer, mBufferIndex );
		mBufferIndex = 0;
	}

	// flush rx buffer.
	EthernetClient::flush( );
}

void WebClient::stop( void )
{
	flush( );
	EthernetClient::stop( );
}
/*
WebClient& WebClient::operator=( const EthernetClient& client )
{
    *this = client;
    return *this;
}
*/
int WebClient::processHTTPReqHeaderByte( struct HTTPReqHeader& header, char c )
{
	mBuffer[mBufferIndex++] = ( uint8_t )c;
	
	// buffer has overflown. can't process request.
	if( mBufferIndex > WEB_CLIENT_BUFFER_SIZE )
		return PROCESS_ERROR;
	
	// we 've reached the end of the header.
	if( c == '\n' && mCurrentLineIsEmpty )
		return PROCESS_DONE;
		
	switch( mParserState )
	{
	case STATE_READ_METHOD:
		if( c == ' ' )
		{
			mBuffer[--mBufferIndex]  = '\0';
			mBufferIndex = 0;
			header.method = getHTTPMethodFromStr( ( const char* )mBuffer );
			mParserState = STATE_READ_URL;
		}
		break;

	case STATE_READ_URL:
		if( c == ' ' )
		{
			mBuffer[--mBufferIndex]  = '\0';
			mBufferIndex = 0;

			if( strlen( ( const char* )mBuffer ) <= MAX_HTTP_URL_SIZE )
			{
				strcpy( header.url, ( const char* )mBuffer );
				mParserState = STATE_READ_VERSION;
			}
			else
			{
				return PROCESS_ERROR;
			}
		}
		break;

	case STATE_READ_VERSION:
		if( c == '\n' )
		{
			mBuffer[--mBufferIndex]  = '\0';
			mBufferIndex = 0;
			header.version = getHTTPVersionFromStr( ( const char* )mBuffer );
			mParserState = STATE_READ_OPTION_KEY;
		}
		break;

	case STATE_READ_OPTION_KEY:
		if( c == ' ' )
		{
			mBuffer[--mBufferIndex]  = '\0';
			mBufferIndex = 0;

			if( strstr( ( const char* )mBuffer, "Content-Length:" ) )
			{
				mParserState = STATE_READ_OPTION_CONTENT_LENGTH;
			}
                        else
                        {
                                mParserState = STATE_DISCARD_OPTION_VALUE; 
                        }
		}
		break;

	case STATE_READ_OPTION_CONTENT_LENGTH:
		if( c == '\n' )
		{
			 // exclude '\r'.
			mBuffer[--mBufferIndex]  = '\0';
			mBufferIndex = 0;
			header.content_length = atoi( ( const char* )mBuffer );
			mParserState = STATE_READ_OPTION_KEY;
		}
		break;

        case STATE_DISCARD_OPTION_VALUE:
                mBufferIndex = 0;
                if( c == '\n' )
                    mParserState = STATE_READ_OPTION_KEY;
                break;
	}

	if( c == '\n' )
	{
		// starting a new line
		mCurrentLineIsEmpty = true;
	} 
	else if( c != '\r' ) 
	{
		// got a character on the current line
		mCurrentLineIsEmpty = false;
	}

	return PROCESS_CONTINUE;
}

int WebClient::processHTTPRespHeaderByte( struct HTTPRespHeader& header, char c )
{
	mBuffer[mBufferIndex++] = ( uint8_t )c;
	
	// buffer has overflown. can't process request.
	if( mBufferIndex > WEB_CLIENT_BUFFER_SIZE )
		return PROCESS_ERROR;
	
	// we 've reached the end of the header.
	if( c == '\n' && mCurrentLineIsEmpty )
		return PROCESS_DONE;
		
	switch( mParserState )
	{
	case STATE_READ_VERSION:
		if( c == ' ' )
		{
			mBuffer[--mBufferIndex]  = '\0';
			mBufferIndex = 0;
			header.version = getHTTPVersionFromStr( ( const char* )mBuffer );
			mParserState = STATE_READ_STATUS;
		}
		break;
		
	case STATE_READ_STATUS:
		if( c == ' ' )
		{
			mBuffer[--mBufferIndex]  = '\0';
			mBufferIndex = 0;
			header.status = getHTTPStatusFromStr( ( const char* )mBuffer );
			mParserState = STATE_READ_STATUS_STR;
		}
		break;

	case STATE_READ_STATUS_STR:
		if( c == '\n' )
		{
			// do not store status str.
			mBufferIndex = 0;
			mParserState = STATE_READ_OPTION_KEY;
		}
		break;

	case STATE_READ_OPTION_KEY:
		if( c == ' ' )
		{
			mBuffer[--mBufferIndex]  = '\0';
			mBufferIndex = 0;

			if( strstr( ( const char* )mBuffer, "Content-Length:" ) )
			{
				mParserState = STATE_READ_OPTION_CONTENT_LENGTH;
			}
			else if( strstr( ( const char* )mBuffer, "Content-Type:" ) )
			{
				mParserState = STATE_READ_OPTION_MIME_TYPE;
			}
                        else
                        {
                                mParserState = STATE_DISCARD_OPTION_VALUE;
                        }
		}
		break;

	case STATE_READ_OPTION_CONTENT_LENGTH:
		if( c == '\n' )
		{
			 // exclude '\r'.
			mBuffer[--mBufferIndex]  = '\0';
			mBufferIndex = 0;
			header.content_length = atoi( ( const char* )mBuffer );
			mParserState = STATE_READ_OPTION_KEY;
		}
		break;
		
	case STATE_READ_OPTION_MIME_TYPE:
		if( c == '\n' )
		{
			 // exclude '\r'.
			mBuffer[--mBufferIndex]  = '\0';
			mBufferIndex = 0;

			if( strlen( ( const char* )mBuffer ) <= MAX_HTTP_MIME_TYPE_SIZE )
			{
				strcpy( header.mime_type, ( const char* )mBuffer );
				mParserState = STATE_READ_OPTION_KEY;
			}
			else
			{
				return PROCESS_ERROR;
			}
		}
		break;

        case STATE_DISCARD_OPTION_VALUE:
                mBufferIndex = 0;
                if( c == '\n' )
                    mParserState = STATE_READ_OPTION_KEY;
                break;	
	}

	if( c == '\n' )
	{
		// starting a new line
		mCurrentLineIsEmpty = true;
	} 
	else if( c != '\r' ) 
	{
		// got a character on the current line
		mCurrentLineIsEmpty = false;
	}

	return PROCESS_CONTINUE;
}

int WebClient::getHTTPMethodFromStr( const char* str )
{
	int result = HTTP_METHOD_INVALID;
	
	if( strstr( str, "GET" ) )
		result = HTTP_METHOD_GET;
	else if( strstr( str, "HEAD" ) )
		result = HTTP_METHOD_HEAD;
	else if( strstr( str, "POST" ) )
		result = HTTP_METHOD_POST;
	
	return result;
}

int WebClient::getHTTPVersionFromStr( const char* str )
{
	int result = HTTP_VER_INVALID;

	if( strstr( str, "HTTP/1.0" ) )
		result = HTTP_VER_1_0;
	else if( strstr( ( const char* )mBuffer, "HTTP/1.1" ) )
		result = HTTP_VER_1_1;
	
	return result;
}

int WebClient::getHTTPStatusFromStr( const char* str )
{
	int result = HTTP_STATUS_INVALID;
	
	if( strstr( str, "200" ) )
		result = HTTP_STATUS_200_OK;
	else if( strstr( str, "400" ) )
		result = HTTP_STATUS_400_BAD_REQ;
	else if( strstr( str, "404" ) )
		result = HTTP_STATUS_404_NOT_FOUND;
	
	return result;
}

int WebClient::decodeUrl( char *str )
{
	char* src = str;
        char* dest = str;
        char* end = str + strlen( str );
	int len = strlen( str );
	char c1;
	char c2;
	
    while(  src < end )
    {
        if( ( c1 = *str++ ) == '+' )
		{
            c1 = ' ';
		}
        else if( ( c1 == '%' ) && ( src + 2 <= end ) &&
                 ( isxdigit( c1 = *str++ ) ) && 
				 ( isxdigit (c2 = *str++ ) ) )
        {
            c1  = ( c1 <= '9' ) ? ( ( c1 - '0' ) * 16 ) : ( ( toupper( c1 ) - 'A' + 10 ) * 16 );
            c1 += ( c2 <= '9' ) ? ( c2 - '0' ) : ( toupper( c2 ) - 'A' + 10 );
        }
        *dest++ = c1;
    }
	
	*dest = '\0';
    
	return ( dest - str );
}

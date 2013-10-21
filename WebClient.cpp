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
#define STATE_DISCARD_OPTION_VALUE			( 0x08 )

WebClient::WebClient( void )
	: EthernetClient( ), mBufferIndex( 0 )
{
	memset( mBuffer, 0, WEB_CLIENT_BUFFER_SIZE );
}

WebClient::WebClient( const EthernetClient& client ) 
	: EthernetClient( client ), mBufferIndex( 0 )
{
	memset( mBuffer, 0, WEB_CLIENT_BUFFER_SIZE );
}

boolean WebClient::writeHTTPRequest( int method, const char* url, const unsigned char* data, int size )
{
    if( !method || !url )
        return false;

    // write HTTP header.
    switch( method )
    {
    case HTTP_METHOD_GET: write( "GET" ); break;   
    case HTTP_METHOD_POST: write( "POST" ); break;
    default:    
	case HTTP_METHOD_HEAD: write( "HEAD" ); break;
    }	
    write( ' ' );
    write( url );
    write( ' ' );
    write( "HTTP/1.0" );
    write( "\r\n" );
    write( "Content-Length: " );
    print( size );
    write( "\r\n" );
    write( "\r\n" );

    // write HTTP body.
    if( method == HTTP_METHOD_POST &&
        data && size > 0 )
    {
        write( data, size );
    }

    return true;
}

boolean WebClient::readHTTPRequest( int* method, char* url, char* data, int* size, int max_size )
{
    int state = STATE_READ_METHOD;
    const char* word;
    
    if( !method || !url )
        return false;
    
    // init data
    *method = '\0';
    *url = '\0';
    *data = '\0';
    *size = 0;
    
    // read header.
    while( word = getNextWordUntilNewLine( ) )
    {
        // this is an empty line.
        // end of header.
        if( !strlen( word ) )
            break;
      
		switch( state )
		{
		case STATE_READ_METHOD:
			*method = getHTTPMethodFromStr( word );
			if( *method != HTTP_METHOD_INVALID )
				state = STATE_READ_URL;
			break;

		case STATE_READ_URL:
			// TODO: urlDecode( )
			strcpy( url, word );
			state = STATE_READ_VERSION;
			break;
				
		case STATE_READ_VERSION:
			if( getHTTPVersionFromStr( word ) != HTTP_VER_INVALID )
				state = STATE_READ_OPTION_KEY;
			break;

		case STATE_READ_OPTION_KEY:
			if( strstr( word, "Content-Length:" ) )
				state = STATE_READ_OPTION_CONTENT_LENGTH;
			break;

		case STATE_READ_OPTION_CONTENT_LENGTH:
			if( size ) 
				*size = atoi( word );
			state = STATE_READ_OPTION_KEY;
			break;
		}
    }
    
    // read body if any.
    if( data && size )
    {
        if( *size > max_size )
            *size = max_size;
        
        int len = 0;
        while( available( ) && len < *size )
            data[len++] = read( );
    }
    
    flush( );
    
    if( *method == HTTP_METHOD_INVALID )
        return false;

    return true; 
}

boolean WebClient::writeHTTPResponse( int status, const char* mime_type, const unsigned char* data, int size )
{
    if( !status )
        return false;
    
    // write HTTP header.
    write( "HTTP/1.0" );
    write( ' ' );
    switch( status )
    {
    case 200: write( "200 OK" ); break;   
    default:
    case 400: write( "400 Bad Request" ); break;
    case 404: write( "404 Not Found" ); break;
    }
    write( "\r\n" );
    if( mime_type )
    {
        write( "Content-Type:" );
        write( ' ' );
        write( mime_type );
        write( "\r\n" );        
    }
    write( "Content-Length:" );
    write( ' ' );    
    print( size );
    write( "\r\n" );
    write( "\r\n" );
    
    // write HTTP body
    if( data && size > 0 )
    {
        write( data, size );  
    }
    
    return true;
}

boolean WebClient::readHTTPResponse( int* status, char* mime_type, unsigned char* data, int* size, int max_size )
{
    int state = STATE_READ_VERSION;
    const char* word;
    
    if( !status )
        return false;
    
    // init data
    *status = HTTP_STATUS_INVALID;
    *mime_type = '\0';
    *data = '\0';
    *size = 0;
    
    // read header.
    while( word = getNextWordUntilNewLine( ) )
    {
        // this is an empty line.
        // end of header.
        if( !strlen( word ) )
            break;
                
		switch( state )
		{
		case STATE_READ_VERSION:
            if( getHTTPVersionFromStr( word ) != HTTP_VER_INVALID )
                state = STATE_READ_STATUS;
            break;
		
		case STATE_READ_STATUS:
            *status = getHTTPStatusFromStr( word );
            state = STATE_READ_OPTION_KEY;
            break;

		case STATE_READ_OPTION_KEY:
            if( strstr( word, "Content-Length:" ) )
                state = STATE_READ_OPTION_CONTENT_LENGTH;
            else if( strstr( word, "Content-Type:" ) )
                state = STATE_READ_OPTION_MIME_TYPE;
            break;

		case STATE_READ_OPTION_CONTENT_LENGTH:
            if( size ) 
                *size = atoi( word );
			state = STATE_READ_OPTION_KEY;
	    break;
		
		case STATE_READ_OPTION_MIME_TYPE:
            if( mime_type )
                strcpy( mime_type, word );
            state = STATE_READ_OPTION_KEY;
            break;
        }
    }
    
    // read body if any.
    if( data && size )
    {
        if( *size > max_size )
            *size = max_size;
        
        int len = 0;
        while( available( ) && len < *size )
            data[len++] = read( );
    }
    
    flush( );
    
    if( *status == HTTP_STATUS_INVALID )
        return false;

    return true;   
}

boolean WebClient::waitForResponse( int timeout )
{
    unsigned long now = millis( );

    while( !available( ) )
    {
        if( millis( ) - now >= ( unsigned long )timeout )
            return false;
    }
    
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

const char* WebClient::getNextWordUntilNewLine( void )
{
    char* buffer = ( char* )mBuffer;
    
    // init buffer
    *buffer = '\0';
    
    while( available( ) )
    {
        char c = read( );
        
        if( c == '\n' )
        {
            *buffer = '\0';
            break;
        }
        else if( c == ' ' || c == ';' )
        {
            *buffer = '\0';
            if( strlen( ( const char* )mBuffer ) )
                break;   
        }
        else if( c != '\r' ) // do not store '\r'
        {
            *buffer++ = ( uint8_t )c;
        }
    }
    
    return ( const char* )mBuffer;
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

#include <SD.h>
#include "WebServer.h"
#include "WebClient.h"
//#include "AgiFileParser.h"
#include <avr/pgmspace.h>

#define DEFAULT_DOC	"index.htm"

typedef struct 
{
	const char* fileExtStr;
	const char* mimeStr;
} mime_type_entry;


#define DEFINE_MIME_TYPE( mimeType, mimeStr ) \
        prog_char mime_type_ext_str##mimeType[] PROGMEM = #mimeType; \
        prog_char mime_type_str##mimeType[] PROGMEM = mimeStr; \
		mime_type_entry mime_##mimeType PROGMEM = { mime_type_ext_str##mimeType, mime_type_str##mimeType };

#define BEGIN_MIME_TYPE_TABLE \
	mime_type_entry* mime_type_table[] PROGMEM = {

#define MIME_TYPE( mimeType ) \
	&mime_##mimeType,

#define END_MIME_TYPE_TABLE	\
	NULL };

DEFINE_MIME_TYPE( raw, "text/raw" )
DEFINE_MIME_TYPE( htm, "text/html" )
DEFINE_MIME_TYPE( css, "text/css" )
DEFINE_MIME_TYPE( jpg, "image/jpg" )
DEFINE_MIME_TYPE( png, "image/png" )
DEFINE_MIME_TYPE( gif, "image/gif" )
DEFINE_MIME_TYPE( js,  "text/javascript" )
DEFINE_MIME_TYPE( jsn, "application/json" )

BEGIN_MIME_TYPE_TABLE
	MIME_TYPE( raw )	
	MIME_TYPE( htm )
	MIME_TYPE( css )
	MIME_TYPE( jpg )
	MIME_TYPE( png )
	MIME_TYPE( gif )
	MIME_TYPE( js )
	MIME_TYPE( jsn )
END_MIME_TYPE_TABLE

WebServer::WebServer( uint16_t port, const char* htdocs )
	: Server( port )//, TxBufferIndex( 0 )
{
//	strcpy( HtDocs, htdocs );
	
	// remove trailing '/'
//	char* ptr = ( char* )strrchr( HtDocs, '/' );
//	if( ptr != HtDocs )
//		*ptr = '\0';
}

WebServer::~WebServer( void )
{
}
	
boolean WebServer::begin( void )
{
    Server.begin( );
    
	if( SD.begin( 4 ) )// && SD.exists( ( char* )HtDocs ) )
		return true;
		
	return false;
}

void WebServer::process( void )
{
    WebClient client = WebClient( Server.available( ) );

	if( !client || !client.connected( ) )
		return;

	int method;
	char url[20];
	int size;
	char data[20];
	if( !client.readHTTPRequest( &method, url, data, &size, 20 ) )
	{
		client.writeHTTPResponse( 400, "text/html", (uint8_t*)"<h1>400 Bad Request</h1>\r\n", 27 );
		char* command = getCommandFromUrl( url );
		if( command )
			*(command-1) = '\0';

		char* url_filename = strrchr( url, '/' );

		if( url_filename && ( *( ++url_filename ) == '\0' ) )
                    strcpy( ( char* )url_filename, DEFAULT_DOC );
                    
                Serial.print( "Filename: " );
                Serial.println( url_filename );                
                Serial.print( "Data: " );
                for( int i=0; i<size; i++ )
                    Serial.print( (char)data[i] );                
	}
	else
	{
		
	}

	//PrintRequest( request );
	client.stop( );           
}

const char* WebServer::getFilenameFromUrl( const char* url )
{
	const char* filename = strrchr( url, '/' );
	if( !filename )
		return 0;
		
	return ++filename;
}

char* WebServer::getCommandFromUrl( const char* url )
{
	return 0;
}

const char* WebServer::getExtFromFilename( const char* filename )
{
	const char* ext = strrchr( filename, '.' );
	if( !ext )
		return 0;
		
	return ++ext;
}

void WebServer::getMimeFromExt( const char* ext, char* mime )
{
	int index = 0;
	boolean found = false;

	mime_type_entry* entry = ( mime_type_entry* )pgm_read_word( &mime_type_table[index] );
	while( entry && !found )
	{
		char buffer[5];
		strcpy_P(buffer, (PGM_P)pgm_read_word( &entry->fileExtStr ) );

		if( strcmp( ext, buffer ) == 0 )
		{
			strcpy_P( mime, (PGM_P)pgm_read_word( &entry->mimeStr ) );
			found = true;
		}

		entry = ( mime_type_entry* )pgm_read_word( &mime_type_table[++index] );
	}
	
	if( !found )
		*mime = '\0';
}

boolean fileExists( char* filepath )
{
	return SD.exists( filepath );
}


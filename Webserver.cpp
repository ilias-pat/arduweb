#include <SD.h>
#include "WebServer.h"
#include "WebClient.h"
#include "AgiFileParser.h"
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
//#define DEFINE_MIME_TYPE( mimeType, mimeStr ) \
//		mime_type_entry mime_##mimeType PROGMEM = { #mimeType, mimeStr };


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

/*
void PrintRequest( struct HTTPRequest& req )
{
  cout << ( "Version = " );
  cout << ( req.version ) << endl;
  cout << ( "Method = " );
  cout << ( req.method ) << endl;
  cout << ( "URL = " );
  cout << ( req.url ) << endl;  
  cout << ( "URL Filenam = " );
  if( req.url_filename )
	cout << ( req.url_filename ) << endl; 
  else
	cout << (int)0 << endl;
  cout << ( "Data size = " );
  cout << ( req.data_size ) << endl;  
  cout << ( "DATA = " );
  cout << ( req.data ) << endl;    
}
*/

prog_char http200Ok[] PROGMEM = "HTTP/1.0 200 OK\r\nPragma: no-cache\r\n";
prog_char http404NotFound[] PROGMEM = "HTTP/1.0 404 Not Found\r\n";
prog_char http400BadRequest[] PROGMEM = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n";
//prog_char httpServerStr[] PROGMEM = "Server: WebServer/0.0 (Arduino)\r\n";

WebServer::WebServer( uint16_t port, const char* htdocs )
	: Server( port )//, TxBufferIndex( 0 )
{
	//memset( RxBuffer, 0, RX_BUFFER_SIZE );
	//memset( TxBuffer, 0, TX_BUFFER_SIZE );
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
	
	struct HTTPReqHeader reqHeader;

	boolean result = client.readHTTPReqHeader( reqHeader );
        Serial.println( result ? "PARSE ok" : "PARSE NOK" );
	if( result )
	{
                Serial.println( "HTTP Req received" );
                Serial.print( "Version: " );
                Serial.println( reqHeader.version );
                Serial.print( "Method: " );
                Serial.println( reqHeader.method );                
                Serial.print( "URL: " );
                Serial.println( reqHeader.url );
                Serial.print( "Len: " );
                Serial.println( reqHeader.content_length );          

                char* url_filename = strrchr( reqHeader.url, '/' );

		if( url_filename && ( *( ++url_filename ) == '\0' ) )
                    strcpy( ( char* )url_filename, DEFAULT_DOC );

                Serial.print( "Filename: " );
                Serial.println( url_filename );                    
	}
	else
	{
		client.write_pgm( http400BadRequest );
		client.write_pgm( PSTR( "Content-type: text/html\r\n\r\n" ) );
		client.write_pgm( PSTR( "<h1>400 Bad Request</h1>\r\n" ) );
	}

	//PrintRequest( request );
	client.stop( );

        static int time = millis( );
        
        if( time - millis( ) >= 5000 )
        {
            time = millis( );
            
            WebClient cl;
            Serial.println("SENDING REQUEST");            
            if( cl.connect( IPAddress( 192, 168, 1, 230 ), 80 ) )
            {
                Serial.println("connected");
                 // Make a HTTP request:
                struct HTTPReqHeader h;
                h.version = HTTP_VER_1_0;
                h.method = HTTP_METHOD_GET;
                h.url[0] = '/';
                h.url[1] = '\0';
                h.content_length = 0;
                
                cl.writeHTTPReqHeader( h );
                
                struct HTTPRespHeader respH;
                if( cl.readHTTPRespHeader( respH ) )
                {
                    Serial.println( "HTTP RESP received" );
                    Serial.print( "Version: " );
                    Serial.println( respH.version );
                    Serial.print( "Status: " );
                    Serial.println( respH.status );                
                    Serial.print( "MIME: " );
                    Serial.println( respH.mime_type );
                    Serial.print( "Len: " );
                    Serial.println( respH.content_length );
                }
                else
                {
                    Serial.println( "resp parse failed" );
                }
            }
            else
            {
                Serial.println( "connect failed" );
            }
            cl.stop( );
        }
            
}

const char* WebServer::getFilenameFromUrl( const char* url )
{
	const char* filename = strrchr( url, '/' );
	if( !filename )
		return 0;
		
	return ++filename;
}

const char* WebServer::getCommandFromUrl( const char* url )
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

boolean fileExists( const char* filepath )
{

}

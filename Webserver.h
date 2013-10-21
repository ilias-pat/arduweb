#ifndef WEB_SERVER_H
#define WEB_SERVER_H

//#include "AgiFileParser.h"
#include <EthernetServer.h>
#include <SD.h>
#include <avr/pgmspace.h>

// Size of receive buffer in bytes.
//#define MAX_HTTP_URL_SIZE               50
//#define MAX_HTTP_DATA_SIZE              50
//#define RX_BUFFER_SIZE			32
//#define TX_BUFFER_SIZE			32
#define HTDOCS_FILEPATH_SIZE	        64

class WebServer
{
public:
	WebServer( uint16_t port, const char* htdocs );
	~WebServer( void );
	
	boolean begin( void );
	void process( void );

private:
	// 1, is it a command
	// 1a. yes, goto 2
	// 1b. no, goto 3
	// 2. deliver http command to user( command , data )
	// 3. does file exist?
	// 3a. yes goto 4
	// 3b. no goto 5
	// 4. Send 200 ok and file.
	// 5. Send 404 not found.
        
	const char* getFilenameFromUrl( const char* url );
	char* getCommandFromUrl( const char* url );
	const char* getExtFromFilename( const char* filename );
	void getMimeFromExt( const char* ext, char* mime );
	boolean fileExists( const char* filepath );
	
	int processGetCommand( const char* url_filename, const char* command );


	const prog_char* GetMimeTypeStrFromIndex( int index );
	int GetMimeTypeFromFilename( const char* filename );
	const prog_char* GetMimeTypeFromFilename1( const char* filename );

	EthernetServer Server;
//	char HtDocs[HTDOCS_FILEPATH_SIZE];

};
/*
struct HTTPRequest
{
	HTTPRequest( void )
          : version( -1 ), method( -1 ), url_filename( 0 ), data_size( 0 )
	{
                memset( url, 0, MAX_HTTP_URL_SIZE );
                memset( data, 0, MAX_HTTP_DATA_SIZE );
        }
        
	int version;
	int method;
	char url[MAX_HTTP_URL_SIZE];
	const char* url_filename;
	int data_size;
	char data[MAX_HTTP_DATA_SIZE];
};
*/
#endif

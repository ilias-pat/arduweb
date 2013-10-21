#ifndef WEB_CLIENT_H
#define WEB_CLIENT_H

#include <EthernetClient.h>
#include <avr/pgmspace.h>

#define WEB_CLIENT_BUFFER_SIZE		( 0x20 )

#define HTTP_VER_1_0					( 0x10 )
#define HTTP_VER_1_1					( 0x11 )
#define HTTP_VER_INVALID				( -1 )

#define HTTP_METHOD_HEAD				( 0x00 )
#define HTTP_METHOD_GET					( 0x01 )
#define HTTP_METHOD_POST				( 0x02 )
#define HTTP_METHOD_INVALID				( -1 )

#define HTTP_STATUS_200_OK				( 200 )
#define HTTP_STATUS_400_BAD_REQ			( 400 )
#define HTTP_STATUS_404_NOT_FOUND		( 404 )
#define HTTP_STATUS_INVALID				( -1 )

class WebClient : public EthernetClient
{
public:
	WebClient( void );
	WebClient( const EthernetClient& client );

    boolean writeHTTPReqHeader( int method, const char* url, int contentLen );
    boolean readHTTPReqHeader( int* method, char* url, int* contentLen );
    boolean writeHTTPRespHeader( int status, const char* mime_type, int contentLen );
    boolean readHTTPRespHeader( int* status, char* mime_type, int* contentLen );
    boolean waitForResponse( int timeout ); // in msecs
	size_t write_pgm( const prog_char* str );
	
private:
	int getHTTPMethodFromStr( const char* str );
	int getHTTPVersionFromStr( const char* str );
	int getHTTPStatusFromStr( const char* str );
	int decodeUrl( char *str );
    const char* getNextWordUntilNewLine( void );

	uint8_t mBuffer[WEB_CLIENT_BUFFER_SIZE+1];
	int mBufferIndex;
};

#endif

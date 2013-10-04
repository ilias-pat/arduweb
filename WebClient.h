#ifndef WEB_CLIENT_H
#define WEB_CLIENT_H

#include <EthernetClient.h>
#include <avr/pgmspace.h>

#define WEB_CLIENT_BUFFER_SIZE		( 0x20 )
#define MAX_HTTP_URL_SIZE               ( 0x20 )
#define MAX_HTTP_MIME_TYPE_SIZE         ( 0x14 )

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

struct HTTPReqHeader
{
	int method;
	char url[MAX_HTTP_URL_SIZE];
	int version;
	int content_length;
};

struct HTTPRespHeader
{
	int version;
	int status;
	char mime_type[MAX_HTTP_MIME_TYPE_SIZE];
	int content_length;
};

class WebClient : public EthernetClient
{
public:
	WebClient( void );
	WebClient( const EthernetClient& client );
	boolean readHTTPReqHeader( struct HTTPReqHeader& header );
	boolean writeHTTPReqHeader( const struct HTTPReqHeader& header );
	boolean readHTTPRespHeader( struct HTTPRespHeader& header );
	boolean writeHTTPRespHeader( const struct HTTPRespHeader& header );
	size_t write( uint8_t c );
	size_t write_pgm( const prog_char* str );
	void flush( void );
	void stop( void );
	
private:
	int processHTTPReqHeaderByte( struct HTTPReqHeader& header, char c );
	int processHTTPRespHeaderByte( struct HTTPRespHeader& header, char c );
	int getHTTPMethodFromStr( const char* str );
	int getHTTPVersionFromStr( const char* str );
	int getHTTPStatusFromStr( const char* str );
	int decodeUrl( char *str );

	int mParserState;
	bool mCurrentLineIsEmpty;
	uint8_t mBuffer[WEB_CLIENT_BUFFER_SIZE+1];
	int mBufferIndex;
};

#endif

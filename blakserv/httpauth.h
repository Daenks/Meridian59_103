#include <curl/curl.h>

#ifndef _HTTP_AUTH
#define _HTTP_AUTH

#define CURL_USER_AGENT "blakserv-agent/1.0"

#define HTTP_AUTH_URL "http://white.treyhome.com/m59auth"
#define HTTPS_AUTH_URL "https://white.treyhome.com/m59auth"

// SSL Options (Currently Disabled)
//#define CURL_USE_SSL
//#define CURL_SKIP_PEER_VERIFICATION
//#define CURL_SKIP_HOSTNAME_VERIFICATION

struct HttpResponse {
  char *rdata;
  size_t size;
};

void LoadCurl(void);
void AuthenticateHttp();
static size_t httpResponseToMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
void UnloadCurl(void);

#endif
#include "blakserv.h"

CURL *curl;
CURLcode res;

void LoadCurl(void)
{
   curl_global_init(CURL_GLOBAL_WIN32);
   lprintf("Loading cURL: %s",curl_version());
}

void AuthenticateHttp()
{
   curl = curl_easy_init();
   
   struct HttpResponse response;
 
   response.rdata = (char*)malloc(1);
   response.size = 0;

   if(curl) 
   {
      curl_easy_setopt(curl, CURLOPT_USERAGENT, CURL_USER_AGENT);
      
      #ifdef CURL_USE_SSL
      curl_easy_setopt(curl, CURLOPT_URL, HTTPS_AUTH_URL);
      
      #ifdef CURL_SKIP_PEER_VERIFICATION
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      #endif CURL_SKIP_PEER_VERIFICATION
      
      #ifdef CURL_SKIP_HOSTNAME_VERIFICATION
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      #endif CURL_SKIP_HOSTNAME_VERIFICATION
      
      #else // No CURL_USE_SSL
            curl_easy_setopt(curl, CURLOPT_URL, HTTP_AUTH_URL);
      #endif CURL_USE_SSL
      
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpResponseToMemoryCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
      
      res = curl_easy_perform(curl);
    
      if(res != CURLE_OK)
      {
         eprintf("HTTP authentication failed: %s\n",
            curl_easy_strerror(res));
      }
      else
      {
         // got a response, process it
         lprintf("Http authentication: received server response.\n");
      }
   }
   
   if(response.rdata)
    free(response.rdata);
   
   curl_easy_cleanup(curl);
}

static size_t httpResponseToMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct HttpResponse *response = (struct HttpResponse *)userp;
 
  response->rdata = (char*) realloc(response->rdata, response->size + realsize + 1);
  if(response->rdata == NULL) {
    /* out of memory! */ 
    eprintf("Http authentication failed: Not enough memory to store server response.\n");
    return 0;
  }
 
  memcpy(&(response->rdata[response->size]), contents, realsize);
  response->size += realsize;
  response->rdata[response->size] = 0;
 
  return realsize;
}

void UnloadCurl(void)
{
   curl_global_cleanup();
}
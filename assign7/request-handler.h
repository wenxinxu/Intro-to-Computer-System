/**
 * File: request-handler.h
 * -----------------------
 * Defines the HTTPRequestHandler class, which fully proxies and
 * services a single client request.  
 */

#ifndef _request_handler_
#define _request_handler_

#include <utility>
#include <string>
#include "blacklist.h"
#include "cache.h"
#include <mutex>
#include <semaphore.h>


class HTTPRequestHandler {
 public:
  HTTPRequestHandler();
  void serviceRequest(const std::pair<int, std::string>& connection) throw();
  void setProxy(unsigned short port);
  void clearCache();
  void setCacheMaxAge(long maxAge);
  unsigned short getPort(){return proxyNum_;}
  void forwardRequestToProxy(const HTTPRequest &request, const std::pair<int, std::string> &connection);

private:
    HTTPBlacklist blacklist;
    HTTPCache cache;
    std::string server_;
    unsigned short proxyNum_;
    size_t getHashcode(const HTTPRequest& request);
    void cacheHTTP(const HTTPRequest& request, const std::pair<int, std::string> &connection);
    std::mutex cacheLock[997];


};

#endif

/**
 * File: request-handler.cc
 * ------------------------
 * Provides the implementation for the HTTPRequestHandler class.
 */

#include "request-handler.h"
#include "response.h"
#include "ostreamlock.h"
#include <unistd.h>

#include "proxy.h"

#include <socket++/sockstream.h> // for sockbuf, iosockstream
#include "client-socket.h"
using namespace std;


HTTPRequestHandler::HTTPRequestHandler(){
  blacklist.addToBlacklist("blocked-domains.txt");

}
void HTTPRequestHandler::setProxy( unsigned short port) {

    this->proxyNum_ = port;
}
size_t HTTPRequestHandler::getHashcode(const HTTPRequest& request) {
  return cache.getHashcode(request);
}
void HTTPRequestHandler::forwardRequestToProxy(const HTTPRequest &request, const std::pair<int, std::string> &connection)
{

    int clientSocket  = createClientSocket(request.getServer(),request.getPort());
    if (clientSocket == -1) {
        string e = "Can not connect to  " + request.getURL();
        throw HTTPProxyException(e);
    }
  sockbuf sb(clientSocket);
  iosockstream  ss(&sb);
  ss << request;
  ss.flush();
  // get response

  sockbuf sb1(clientSocket);
  istream is1(&sb1);
  HTTPResponse response;
  response.ingestResponseHeader(is1);
  response.ingestPayload(is1);
  size_t hashCode =getHashcode(request);
  cacheLock[hashCode].lock();
  if(cache.shouldCache(request,response)) {
    try {
      cache.cacheEntry(request, response);
    }
    catch (HTTPCacheAccessException& e) {
      cerr<<oslock<<"cache inaccessible "<<e.what()<<endl<<osunlock;
    }
  }
  cacheLock[hashCode].unlock();
  // send back to client
  sockbuf sb2(connection.first);
    istream s2(&sb2);
  iosockstream ss2(&sb2);
  ss2 << response;
  ss2.flush();
  close(clientSocket);
}
void HTTPRequestHandler::serviceRequest(const std::pair<int, std::string> &connection) throw(){
  sockbuf sb(connection.first);
  istream s1(&sb);
  iosockstream ss(&sb);
  HTTPResponse response;
  HTTPRequest request;
  try {
    request.ingestRequestLine(s1);
  } 
  catch (HTTPBadRequestException& e){
      cerr<<oslock<<"Bad Request: "<<e.what()<<endl<<osunlock;
  }
  //check if it is in blacklist
  if(!blacklist.serverIsAllowed(request.getServer())) {
    response.setResponseCode(403);
    response.setProtocol("HTTP/1.0");
    response.setPayload("Forbidden Content");
    ss<<response;
    ss.flush();
    return;
  }
  //check cache
  
  request.ingestHeader(s1,connection.second);
  request.addRequestEntity(connection.second);
  HTTPRequest forward = request;

  request.ingestPayload(s1);

  size_t hashCode =getHashcode(request);
  cacheLock[hashCode].lock();
  if(cache.containsCacheEntry(request,response)) {
      cacheHTTP(request,connection);
  }
  else {
    if(cache.shouldCache(request,response)) {
      try {
        cache.cacheEntry(request, response);
      }
      catch (HTTPCacheAccessException& e) {
        cerr<<oslock<<"cache inaccessible "<<e.what()<<endl<<osunlock;
      }
    }
  }
  cacheLock[hashCode].unlock();

  //redirect from proxy

  forwardRequestToProxy(forward,connection);
}
void HTTPRequestHandler::cacheHTTP(const HTTPRequest &request,const std::pair<int, std::string> &connection) {

  HTTPResponse response;
    sockbuf sb2(connection.first);
    iosockstream ss(&sb2);
    ss << response;
    ss.flush();
}
// the following two methods needs to be completed 
// once you incorporate your HTTPCache into your HTTPRequestHandler
void HTTPRequestHandler::clearCache() {
    cache.clear();
}

void HTTPRequestHandler::setCacheMaxAge(long maxAge) {
    cache.setMaxAge(maxAge);
}

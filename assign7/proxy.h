/**
 * File: proxy.h
 * -------------
 * Defines the primary class that manages all HTTP proxying.  It's
 * primary responsibility is to accept all incoming connection requests
 * and to get them off the main thread as quickly as possible (by passing
 * the file descriptor to the scheduler).
 */

#ifndef _http_proxy_
#define _http_proxy_


#include "proxy-exception.h"
#include <string>
#include <utility>
#include "scheduler.h"

class HTTPProxy {
 public:

/**
 * Configures the proxy server to listen for incoming traffic on the
 * specified port.  If the specified port is in use, or the HTTP Proxy
 * server otherwise can't bind to it, then an exception is thrown.
 */
  HTTPProxy(int argc, char *argv[]) throw (HTTPProxyException);

/**
 * Returns the port number our proxy is listening to.
 */
  unsigned short getPortNumber() const { return portNumber; }
/**
 * Returns a proxy server
 */
    std::string getProxy() const{return server;}
/**
 * Waits for an HTTP request to come in, and does whatever it takes
 * to handle it.  Because acceptAndProxyRequest is assumed to be
 * called to handle a single request as opposed to all of them, we
 * assume any and all exceptions thrown within are just for that request,
 * so we further assume exceptions are handled internally and not
 * thrown.
 */
  void acceptAndProxyRequest() throw(HTTPProxyException);
  
 private:
  unsigned short portNumber;
  int listenfd;
  HTTPProxyScheduler scheduler;
  std::string server;

  /* private methods */
  void configureFromArgumentList(int argc, char *argv[]) throw (HTTPProxyException);
  void createServerSocket();
  void configureServerSocket() const;
  const char *getClientIPAddress(const struct sockaddr_in *clientAddr) const;
};

#endif

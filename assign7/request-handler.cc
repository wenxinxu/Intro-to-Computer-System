/**
 * File: request-handler.cc
 * ------------------------
 * Provides the implementation for the HTTPRequestHandler class.
 */

#include "request-handler.h"
#include "response.h"
#include "ostreamlock.h"
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>
#include <fstream>
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

int setupEpoll(int watchset, int firefoxFd, int googleFd) {
    struct epoll_event firefoxEvent;
    firefoxEvent.events = EPOLLIN;
    firefoxEvent.data.fd = firefoxFd;
    struct epoll_event googleEvent;
    googleEvent.events = EPOLLIN;
    googleEvent.data.fd = googleFd;
    epoll_ctl(watchset, EPOLL_CTL_ADD, firefoxFd, &firefoxEvent);
    epoll_ctl(watchset, EPOLL_CTL_ADD, googleFd, &googleEvent);
    return watchset;
}

int proxyPackets(iosockstream &fromStream, iosockstream &toStream, int fromFd, int toFd) {
   // cout << "proxy from " << fromFd << " to " << toFd << endl;
    char buffer[1];
    fromStream.read(buffer, 1);
    if(fromStream.gcount() == 0 ) return 0;
    toStream.write(buffer,1);
    int count = 1;
    while(true) {
        const int BUFFER_SIZE = 1024;

        char buffer[BUFFER_SIZE] = {'\0'};
        int num = fromStream.readsome(buffer, BUFFER_SIZE);
        if(num <= 0) {
            cout<<" breaking because read "<< num<<endl;
            break;
        }
        count += num;
        toStream.write(buffer, fromStream.gcount());
        toStream.flush();
        cout << "proxied " << fromStream.gcount() + 1 << " from " << fromFd << " to " << toFd << endl;

    }
    return count;
}

void HTTPRequestHandler::serviceRequest(const std::pair<int, std::string> &connection) throw(){

    sockbuf firefoxSockbuf(connection.first);
    int firefoxFd = connection.first;
    iosockstream firefoxStream(&firefoxSockbuf);
    HTTPRequest request;
    HTTPResponse response;
    try {
        request.ingestRequestLine(firefoxStream);
        request.ingestHeader(firefoxStream,connection.second);
    } catch (HTTPBadRequestException &e) {
        throw(e.what());
    }

    int googleFd = createClientSocket(request.getServer(), request.getPort());
    sockbuf googleSockBuf(googleFd);
    iosockstream googleStream(&googleSockBuf);
    firefoxStream << "HTTP/1.1 200 OK\r\n";
    firefoxStream << "\r\n";
    firefoxStream << flush;
    int watchset = epoll_create(1);
    setupEpoll(watchset, firefoxFd, googleFd);
    while (true) {
        struct epoll_event events[2];
        int numEvents = epoll_wait(watchset, events, 2, 30000);
        if (numEvents <= 0) break;
            int eventFd = events[0].data.fd;
            int count = 0;
            if (eventFd == googleFd) {
                count = proxyPackets(googleStream, firefoxStream, googleFd, firefoxFd);
            } else if (eventFd == firefoxFd) {
                count = proxyPackets(firefoxStream, googleStream, firefoxFd, googleFd);
            }
            if (count == 0) {
                break;
            }

    } 
    close(googleFd);
    close(firefoxFd);
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
// the following two methods needs to be completed 
// once you incorporate your HTTPCache into your HTTPRequestHandler
void HTTPRequestHandler::clearCache() {
    cache.clear();
}

void HTTPRequestHandler::setCacheMaxAge(long maxAge) {
    cache.setMaxAge(maxAge);
}

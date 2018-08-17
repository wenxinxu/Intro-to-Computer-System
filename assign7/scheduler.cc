/**
 * File: scheduler.cc
 * ------------------
 * Presents the implementation of the HTTPProxyScheduler class.
 */

#include "scheduler.h"
#include <utility>

using namespace std;
HTTPProxyScheduler::HTTPProxyScheduler():pool(64){

}
void HTTPProxyScheduler::setProxy(unsigned short port) {
  this->portNum = port;
}

void HTTPProxyScheduler::scheduleRequest(int connectionfd, const string& clientIPAddress) throw () {

  //  requestHandler.setProxy(getPort());
    //requestHandler.serviceRequest(make_pair(connectionfd,clientIPAddress));

    pool.schedule([connectionfd,clientIPAddress, this] () {
        const std::pair<int, std::string> &connection = make_pair(connectionfd, clientIPAddress);
        requestHandler.serviceRequest(connection);
    });

}



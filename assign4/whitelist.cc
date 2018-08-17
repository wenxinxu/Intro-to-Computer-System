#include <regex>

#define BOOST_NETWORK_ENABLE_HTTPS
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>

#include "whitelist.h"

using namespace std;
namespace network = boost::network;
namespace http = boost::network::http;
namespace uri = boost::network::uri;

void Whitelist::addHost(string url) {
    whitelistedHosts.push_back(url);
}

bool Whitelist::canAccess(string uriStr) {
    uri::uri uri(uriStr);
    if (!hostIsWhitelisted(uriStr)) return false;
    return !isDisallowedByRobots(uriStr);
}

// From https://stackoverflow.com/a/3418285/2079798
static void stringReplace(std::string& str, const std::string& from, const std::string& to) {
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

bool Whitelist::hostIsWhitelisted(std::string path) {
    uri::uri destination(path);
    for (string allowed : whitelistedHosts) {
        // Hack for getting around netlib uri bug where parsing breaks if the
        // proto is omitted
        bool ignoreProto = false;
        if (allowed.substr(0, 5) != "http:"
                && allowed.substr(0, 6) != "https:") {
            allowed = "http://" + allowed;
            ignoreProto = true;
        }

        uri::uri allowedUri(allowed);

        if (!ignoreProto && destination.scheme() != allowedUri.scheme()) {
            continue;
        }

        if (allowedUri.user_info().length()
                && allowedUri.user_info() != destination.user_info()) {
            continue;
        }

        string allowedHost = allowedUri.host();
        // Expand wildcards to a valid regex
        stringReplace(allowedHost, "*", ".*");
        regex hostReg("^" + allowedHost + "$");
        if (!regex_match(destination.host(), hostReg)) continue;

        if (allowedUri.port().length()
                && allowedUri.port() != destination.port()) {
            continue;
        }

        return true;
    }
    return false;
}

/* Fetch robotsUrl and return a list of disallowed URLs */
static vector<string> downloadRobotsTxt(string robotsUrl) {
    string contents;
    try {
        http::client::request req(robotsUrl);
        http::client::options options;
        options.always_verify_peer(false)   // Our certificate store is out of
                // date... doing this until I figure out how to fix it :(
            .timeout(20);
        http::client cl(options);
        http::client::response resp = cl.get(req);
        contents = body(resp);
    } catch (exception& e) {
        // If we failed to download robots.txt, assume it's not there. Don't
        // need to do anything because `contents` is already an empty string
    }
    vector<string> toReturn;
    regex disallowReg("Disallow:\\s+([^\\s]*)");
    smatch matches;
    while (regex_search(contents, matches, disallowReg)) {
        toReturn.push_back(matches[1].str());
        contents = matches.suffix().str();
    }
    return toReturn;
}

bool Whitelist::isDisallowedByRobots(string url) {
    uri::uri target(url);

    // Construct a string with the destination server as proto://host[:port]
    uri::uri robotsUrl;
    uri::builder(robotsUrl).scheme(target.scheme());
    robotsUrl.append(uri::authority(target));
    // Add robots.txt to path
    uri::builder(robotsUrl).path("/robots.txt");

    vector<string> robotsContents;
    disallowedLock.lock();
    if (disallowed.find(robotsUrl.string()) == disallowed.end()) {
        disallowedLock.unlock();
        robotsContents = downloadRobotsTxt(robotsUrl.string());
        disallowedLock.lock();
        disallowed.insert({robotsUrl.string(), robotsContents});
    } else {
        robotsContents = disallowed.at(robotsUrl.string());
    }
    disallowedLock.unlock();

    for (string& item : robotsContents) {
        if (target.path().substr(0, item.length()) == item) {
            return true;
        }
    }

    return false;
}

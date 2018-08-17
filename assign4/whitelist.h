#pragma once

#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>

class Whitelist {
public:
    /* Takes a string including proto and port number if relevant (e.g.
     * "https://stanford.edu"). The proto can be omitted to support both http
     * and https, and wildcarding works within the hostname (e.g.
     * "*.stanford.edu" works). This function is NOT thread-safe. */
    void addHost(std::string path);

    /* Checks if we should access a specific url. (Note that even if we've
     * whitelisted the domain, this may still return false, because the website
     * can tell us not to access this particular url via robots.txt.) This
     * function is thread-safe with other canAccess calls. */
    bool canAccess(std::string url);

private:
    std::vector<std::string> whitelistedHosts;
    std::unordered_map<std::string, std::vector<std::string>> disallowed;
    std::mutex disallowedLock;

    bool hostIsWhitelisted(std::string path);
    bool isDisallowedByRobots(std::string url);
};

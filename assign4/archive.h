#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "whitelist.h"
#include "index.h"
#include <semaphore.h>
#include "log.h"
#include "thread-pool.h"
class InternetArchive {
public:
    InternetArchive(const std::vector<std::string>& whitelistedHosts, bool quiet=false,
            bool memoryOnly=false);
    void download(const std::string seedUrl);
    void serve(unsigned short port);
    void dumpIndex();
    void downloadWeb(const std::string url);

private:
    Index index;
    Whitelist webList;
    std::unordered_set<std::string> hasurl;
    std::mutex docLock;
    std::mutex indexLock;
    Log logFile;
    ThreadPool pool;
};

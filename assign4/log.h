#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <sys/time.h>

#include "document.h"

class Log {
public:
    Log(bool quiet) : quiet(quiet) {}

    /* Note: these public functions are all thread safe. */

    void noteDocumentDownloadBegin(const Document& doc);

    // Note download skipped
    void noteDocumentAlreadyDownloaded(const Document& doc);
    void noteDocumentNotWhitelisted(const Document& doc);

    // Note failure (you can pass a string or an exception as an explanation)
    void noteDocumentDownloadFailure(const Document& doc, const std::string& why);
    void noteDocumentDownloadFailure(const Document& doc, const std::exception& e);

    void noteDocumentDownloadEnd(const Document& doc);

    void noteAllDownloadsComplete();

private:
    bool quiet;
    std::unordered_map<std::string, struct timeval> downloadBeginTimes;
    std::mutex downloadTimesLock;

    bool getSavedStartTime(const Document& doc, struct timeval *beginTime);
    std::string getElapsedString(const Document& doc);
};

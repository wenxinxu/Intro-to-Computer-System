#include <ostreamlock.h>

#include "log.h"

using namespace std;

static string getTimeHeading() {
    time_t rawtime;
    time(&rawtime);
    struct tm tm;
    localtime_r(&rawtime, &tm);
    char buf[128];
    strftime(buf, sizeof(buf), "[%d-%m-%Y %I:%M:%S] ", &tm);
    return buf;
}

void Log::noteDocumentDownloadBegin(const Document& doc) {
    if (quiet) return;
    struct timeval begin;
    gettimeofday(&begin, NULL);
    downloadTimesLock.lock();
    downloadBeginTimes.insert({doc.getUrl(), begin});
    downloadTimesLock.unlock();
    cout << oslock << getTimeHeading() << "Beginning download of "
        << doc.getUrl() << osunlock << endl;
}

void Log::noteDocumentAlreadyDownloaded(const Document& doc) {
    if (quiet) return;
    cout << oslock << getTimeHeading() << "Skipping download of "
        << doc.getUrl() << " (already downloaded)" << osunlock << endl;
}

void Log::noteDocumentNotWhitelisted(const Document& doc) {
    if (quiet) return;
    cout << oslock << getTimeHeading() << "Skipping download of "
        << doc.getUrl() << " (not whitelisted, or blocked by robots.txt)"
        << osunlock << endl;
}

void Log::noteDocumentDownloadFailure(const Document& doc, const string& why) {
    if (quiet) return;
    string elapsedStr = getElapsedString(doc);
    cout << oslock << getTimeHeading() << "Download of " << doc.getUrl()
        << " failed!" << elapsedStr << " Explanation:\n" << why << osunlock
        << endl;
}

void Log::noteDocumentDownloadFailure(const Document& doc, const exception& e) {
    if (quiet) return;
    noteDocumentDownloadFailure(doc, e.what());
}

void Log::noteDocumentDownloadEnd(const Document& doc) {
    if (quiet) return;
    string elapsedStr = getElapsedString(doc);
    cout << oslock << getTimeHeading() << "End download of "
        << doc.getUrl() << elapsedStr << osunlock << endl;
}

void Log::noteAllDownloadsComplete() {
    if (quiet) return;
    cout << oslock << getTimeHeading() << "All downloads complete!" << osunlock
        << endl;
}

/* Returns true and populates beginTime with saved start time if a document had
 * a saved start time, and removes the document from the internal map.
 * Otherwise, returns false. */
bool Log::getSavedStartTime(const Document& doc, struct timeval *beginTime) {
    lock_guard<mutex> lg(downloadTimesLock);
    if (downloadBeginTimes.find(doc.getUrl()) == downloadBeginTimes.end()) {
        return false;
    } else {
        *beginTime = downloadBeginTimes[doc.getUrl()];
        downloadBeginTimes.erase(doc.getUrl());
        return true;
    }
}

/* If doc exists in our timing map (i.e. noteDocumentDownloadBegin was
 * correctly called on it), this returns a string noting how many seconds have
 * passed. Otherwise, returns an empty string. */
string Log::getElapsedString(const Document& doc) {
    struct timeval begin, end;
    if (getSavedStartTime(doc, &begin)) {
        gettimeofday(&end, NULL);
        double elapsed = (end.tv_sec - begin.tv_sec)
            + ((end.tv_usec - begin.tv_usec) / 1000000.0);
        return " (" + to_string(elapsed) + " seconds)";
    }
    return "";
}

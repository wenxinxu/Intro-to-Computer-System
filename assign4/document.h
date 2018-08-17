/**
 * File: document.h
 * ----------------
 * Defines the interface for the Document class, which downloads a particular
 * URL from the internet and parses it if it is an HTML document. You may then
 * extract any links present in the document.
 */

#pragma once
#include <iostream>
#include <vector>

#include <myhtml/api.h>

class DownloadException: public std::runtime_error {
public:
    DownloadException(std::string msg) : std::runtime_error(msg.c_str()) {}
};

class ParseException: public std::runtime_error {
public:
    ParseException(std::string msg) : std::runtime_error(msg.c_str()) {}
};

class Document {
public:
    /* Creates a document for a particular URL, but doesn't download it until
     * download() is called. */
    Document(const std::string& url) : url(url) {}

    /* Creates a document that has already been downloaded. (This is used for
     * re-populating the index from archived files on disk.) */
    Document(const std::string& url, const std::string& contents,
            const std::string& contentType)
        : url(url), contents(contents), contentType(contentType) {}

    /* Attempts to download the document. If an error is encountered, throws
     * DownloadException. If the download is successful and the document is an
     * HTML file, parses the HTML to find outbound links. */
    void download();

    const std::string& getUrl() const { return url; }
    const std::string& getContents() const { return contents; }
    const std::string& getContentType() const { return contentType; }
    const std::vector<std::string>& getLinks() const { return links; }

private:
    std::string url;
    std::string contents;
    std::string contentType;
    std::vector<std::string> links;

    void parseHtml();
    void patchLinks(myhtml_tree_t* tree, const char *targetAttr);
};

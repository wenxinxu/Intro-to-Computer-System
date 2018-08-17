#include <algorithm>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <csignal>

#include <openssl/sha.h>
#include <boost/network/protocol/http/server.hpp>
#include <boost/network/uri.hpp>

#include "index.h"

using namespace std;
namespace uri = boost::network::uri;
namespace http = boost::network::http;
namespace utils = boost::network::utils;

const char *kIndexDirectory = "indexed-documents/";
const char *indexListFilename = "documents-list";
inline const char* getIndexHomepage();

static string generateFilename(string url);

Index::Index(bool memoryOnly, bool loadFromDisk) : memoryOnly(memoryOnly) {
    if (memoryOnly) return;

    int status = mkdir(kIndexDirectory, 0700);
    if (status != 0 && errno != EEXIST) {
        cout << status << endl;
        throw runtime_error(string("Error creating ") + kIndexDirectory
                + " directory!");
    }

    if (loadFromDisk) {
        // Load saved documents back into the index
        ifstream ifs;
        ifs.open(string(kIndexDirectory) + indexListFilename);
        string url;
        while ((ifs >> url)) {
            string contentType;
            getline(ifs, contentType);
            contentType = contentType.substr(1);    // get rid of tab delimeter
            ifstream contentStream;
            contentStream.open(kIndexDirectory + generateFilename(url),
                    ios::in | ios::binary);
            string contents((istreambuf_iterator<char>(contentStream)),
                    istreambuf_iterator<char>());
            contentStream.close();
            documents.insert({url, Document(url, contents, contentType)});
        }
    } else {
        // Clear index on disk; we're building a fresh one
        system((string("rm ") + kIndexDirectory + "*").c_str());  // I'm lazy :)
    }
}

void Index::addDocument(Document document) {
    documents.insert({document.getUrl(), document});
    if (memoryOnly) return;
    ofstream of;
    string targetFilename = kIndexDirectory
        + generateFilename(document.getUrl());
    of.open(targetFilename, ios::out | ios::binary | ios::trunc);
    of << document.getContents();
    of.close();
    dumpMetadata();     // not efficient, but this needs to get done :)
}

/* Write a list of files, along with metadata for each file (content type,
 * etc... but not the actual contents of the file) */
void Index::dumpMetadata() {
    if (memoryOnly) return;
    ofstream of;
    of.open(string(kIndexDirectory) + indexListFilename, ios::out | ios::trunc);
    for (auto p : documents) {
        of << p.first << "\t" << p.second.getContentType() << endl;
    }
    of.close();
}

void Index::dumpToTerminal() {
    // Sort downloaded documents by URL
    vector<Document> docs;
    for (auto p : documents) docs.push_back(p.second);
    sort(docs.begin(), docs.end(), [](const Document& a, const Document& b){
        return a.getUrl() > b.getUrl();
    });
    cerr << "-----------------------" << endl;
    cerr << "Contents of index:" << endl;
    for (Document& doc : docs) {
        cerr << "-----------------------" << endl
            << doc.getUrl() << endl
            << "-------" << endl;
        if (doc.getContentType().substr(0, 4) == "text") {
            cerr << doc.getContents().substr(0, 200)
                << (doc.getContents().length() > 200 ? "..." : "") << endl;
        } else {
            cerr << "<contents might not be text, skip printing to terminal>" << endl;
        }
    }
}

void Index::serveDocument(string url, http_server::connection_ptr connection,
        bool headersOnly) {
    if (url == "") {
        connection->set_status(http_server::connection::ok);
        vector<http_server::response_header> headers;
        headers.push_back({"Content-Type", "text/html"});
        connection->set_headers(boost::make_iterator_range(headers.begin(),
                    headers.end()));
        connection->write(string(getIndexHomepage()));
    } else if (documents.find(url) != documents.end()) {
        connection->set_status(http_server::connection::ok);
        Document& doc = documents.at(url);
        vector<http_server::response_header> headers;
        if (doc.getContentType() != "") {
            headers.push_back({"Content-Type", doc.getContentType()});
        }
        headers.push_back({"Content-Length",
                to_string(doc.getContents().length())});
        connection->set_headers(boost::make_iterator_range(headers.begin(),
                    headers.end()));
        connection->write(documents.at(url).getContents());
    } else {
        string notFoundMsg = "Error: document not found.";
        connection->set_status(http_server::connection::not_found);
        vector<http_server::response_header> headers;
        headers.push_back({"Content-Length", to_string(notFoundMsg.length())});
        connection->set_headers(boost::make_iterator_range(headers.begin(),
                    headers.end()));
        connection->write(notFoundMsg);
    }
}

/* Once Index::serve is called, this function is called by c++-netlib server to
 * handle a request. */
void Index::operator()(http_server::request const &request,
        http_server::connection_ptr connection) {
    http_server::string_type ip = source(request);
    cerr << "Incoming connection from " << ip << ": " << request.destination
        << endl;
    if (request.method == "HEAD" || request.method == "GET") {
        serveDocument(uri::decoded(request.destination).substr(1), connection,
                request.method == "HEAD");
    } else {
        connection->set_status(http_server::connection::not_supported);
        connection->write("Error: HTTP method not supported.\n");
    }
}

void Index::serve(unsigned short portNum) {
    try {
        char hostname[32];
        gethostname(hostname, sizeof(hostname));

        http_server::options options(*this);
        http_server server(options
                .thread_pool(make_shared<utils::thread_pool>(16))
                .address("0.0.0.0").port(to_string(portNum)));

        cerr << hostname << " listening on port " << portNum << "..." << endl;
        server.run();
    } catch (exception &e) {
        cerr << "Error running server:" << endl;
        cerr << e.what() << endl;
        exit(1);
    }
}

/* Given a URL, generates a filename to store that URL's contents */
static string generateFilename(string url) {
    ostringstream ss;

    // Prepend a SHA1 hash to avoid collisions between two distinct URLs that
    // become identical when we remove special characters
    unsigned char shaBuf[16];
    SHA1((const unsigned char*)url.c_str(), url.length(), shaBuf);
    ss << hex;
    for (size_t i = 0; i < 20; i++) {
        ss << setw(2) << setfill('0')
            << static_cast<unsigned int>(shaBuf[i]);
    }
    ss << dec;

    // Sanitize the string
    string illegalChars = "\\/:?\"'<>|";
    for (size_t i = 0; i < illegalChars.length(); i++) {
        replace(url.begin(), url.end(), illegalChars[i], '-');
    }

    // Abbreviate the string if it's very long
    const size_t halfLength = 20;
    if (url.length() > 2 * halfLength + 3) {
        url = url.substr(0, halfLength) + "..."
            + url.substr(url.length() - halfLength);
    }

    ss << "-" << url;
    return ss.str();
}

inline const char* getIndexHomepage() {
    return R"V0G0N(
<html>
    <head>
        <title>Search archive</title>
<style>
*{
    box-sizing: border-box;
}

html {
    background: linear-gradient(135deg, #ff9b61 0%,#bf5a13 100%);
}

html, body {
    margin: 0;
    width: 100%;
    height: 100%;
}

h1 {
    margin: 0;
}

.content {
    width: 600px;
    height: 300px;
    margin-left: auto;
    margin-right: auto;
    border-radius: 10px;
    background-color: #F1F8FF;
    box-shadow: 0px 3px 26px -8px rgba(0,0,0,0.75);
    position: relative;
    top: 50%;
    transform: translateY(-50%);
    padding: 20px;
    padding-top: 90px;
    text-align: center;
}

.search-area {
    width: auto;
    margin-left: auto;
    margin-right: auto;
    padding-top: 10px;
}
#query {
    width: 300px;
    font-size: 13pt;
}
.submit-btn {
    display: inline-block;
    cursor: pointer;
    margin-left: 10px;
    background-color: #FFAB00;
    border-radius: 3px;
    padding: 5px;
    position: relative;
    top: -3px;
}
</style>
    </head>
    <body>
        <div class="content">
            <h1>CS 110 Archive</h1>
            <div class="search-area">
                <input type="text" id="query" placeholder="https://www.stanford.edu" />
                <div class="submit-btn" onclick="window.location = '/' + document.getElementById('query').value">Go!</div>
            </div>
        </div>
    </body>
</html>
)V0G0N";
}

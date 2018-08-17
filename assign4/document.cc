/**
 * File: document.cc
 * -----------------
 * Provides implementation of the Document class.
 */

#define BOOST_NETWORK_ENABLE_HTTPS
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>
#include <myhtml/api.h>

#include "document.h"

using namespace std;
using namespace boost::network;
using namespace boost::network::http;

static std::string rewriteUrl(std::string baseUrl, myhtml_tree_node_t *domNode,
        const char *targetAttr);

void Document::download() {
    try {
        client::request req(url);
        client::options options;
        options.always_verify_peer(false)   // Our certificate store is out of
                // date... doing this until I figure out how to fix it :(
            .timeout(20);
        client cl(options);
        client::response resp = cl.get(req);
        contents = body(resp);
        // Save Content-Type header
        for (auto &pair : headers(resp)["Content-Type"]) {
            contentType = pair.second;
            break;
        }
    } catch (exception& e) {
        throw DownloadException(
                "Error downloading document from " + url + ":\n" + e.what());
    }

    // Parse body for links
    const std::string htmlMime = "text/html";
    if (contentType.substr(0, htmlMime.length()) == htmlMime) {
        try {
            parseHtml();
        } catch (ParseException& e) {
            // Do nothing. If we can't parse as HTML, maybe we just received an
            // incorrect mime type, and it's fine for us to say that this page
            // has no outbound links (it might just render weird if we're
            // wrong)
        }
    }
}

/* Parses the body of a document, looking for href/src attributes. (These could
 * be links to other web pages, stylesheets, images, etc.) The outgoing links
 * are rewritten to be absolute URLs, added to the object's `links` vector, and
 * then the HTML text stored in `contents` is overwritten to point to a link on
 * our archive server. */
void Document::parseHtml() {
    myhtml_t* myhtml = myhtml_create();
    if (myhtml == NULL) throw runtime_error("Failed to alloc MyHTML parser");
    if (myhtml_init(myhtml, MyHTML_OPTIONS_DEFAULT, 1, 0) != MyHTML_STATUS_OK) {
        throw runtime_error("Failed to initialize MyHTML parser");
    }
    myhtml_tree_t* tree = myhtml_tree_create();
    if (tree == NULL) throw runtime_error("Failed to alloc MyHTML tree");
    if (myhtml_tree_init(tree, myhtml) != MyHTML_STATUS_OK) {
        throw runtime_error("Failed to initialize MyHTML tree");
    }
    if (myhtml_parse(tree, MyENCODING_UTF_8, contents.c_str(),
                contents.length()) != MyHTML_STATUS_OK) {
        throw ParseException("Failed to parse document body as HTML!");
    }

    // Look for href/src attributes
    patchLinks(tree, "href");
    patchLinks(tree, "src");

    // The page has had links replaced. Dump the parse tree back to a string
    // and update `contents`
    mycore_string_raw_t str = {0};
    if (myhtml_serialization_tree_buffer(myhtml_tree_get_document(tree), &str)
            != MyHTML_STATUS_OK) {
        throw runtime_error("MyHTML failed to serialize parse tree back to a string");
    }
    contents = str.data;
    mycore_string_raw_destroy(&str, false);

    // release resources
    if (myhtml_tree_destroy(tree) != NULL) {
        throw runtime_error("MyHTML failed to free parse tree");
    }
    if (myhtml_destroy(myhtml) != NULL) {
        throw runtime_error("MyHTML failed to free myhtml object");
    }
}

void Document::patchLinks(myhtml_tree_t* tree, const char *targetAttr) {
    myhtml_collection_t *linkedAttrList = myhtml_get_nodes_by_attribute_key(
            tree, NULL, NULL, targetAttr, strlen(targetAttr), NULL);
    if (linkedAttrList == NULL) {
        throw runtime_error("MyHTML failed to alloc linkedAttrList");
    }
    for (size_t i = 0; i < linkedAttrList->length; i++) {
        myhtml_tree_node_t *node = linkedAttrList->list[i];
        std::string absoluteUrl;
        try {
            // Make each URL absolute to some hostname, and then prepend a
            // slash so that it becomes a pathname on our archive server
            absoluteUrl = rewriteUrl(this->url, node, targetAttr);
            // Remove fragments/hashes, since we don't actually want to request
            // those from the server
            if (absoluteUrl.find("#") != std::string::npos) {
                absoluteUrl = absoluteUrl.substr(0, absoluteUrl.find("#"));
            }
        } catch (ParseException& e) {
            // Skip this element; if there are issues with the link, we don't
            // need to follow it to a different page
            continue;
        }
        std::string archiveUrl = "/" + absoluteUrl;
        // Remove the old attribute
        myhtml_tree_attr_t* attr = myhtml_attribute_remove_by_key(
                node, targetAttr, strlen(targetAttr));
        if (attr == NULL) {
            throw runtime_error("MyHTML failed to alloc attr");
        }
        myhtml_attribute_free(tree, attr);
        // Add new attribute
        if (myhtml_attribute_add(node, targetAttr, strlen(targetAttr),
                    archiveUrl.c_str(), archiveUrl.length(),
                    MyENCODING_UTF_8) == NULL) {
            throw runtime_error("MyHTML failed to add new attr to node");
        }
        // Add to our list of outbound links for this doc
        links.push_back(absoluteUrl);
    }
    if (myhtml_collection_destroy(linkedAttrList) != NULL) {
        throw runtime_error("MyHTML failed to free resources for linkedAttrList");
    }
}

/* Rewrites a (potentially relative) URL into an absolute URL, containing a
 * scheme and host. domNode is the parse node containing the destination href
 * attribute, and baseUrl is the URL of the page that is linking to the
 * destination (i.e. if we encounter a relative link, we'll make it relative to
 * this URL). */
static std::string rewriteUrl(std::string baseUrl, myhtml_tree_node_t *domNode,
        const char* targetAttr) {
    // Find target attribute
    for (myhtml_tree_attr_t *attr = myhtml_node_attribute_first(domNode);
            attr != NULL; attr = myhtml_attribute_next(attr)) {
        if (strcmp(myhtml_attribute_key(attr, NULL), targetAttr) == 0) {
            if (myhtml_attribute_value(attr, NULL) == NULL) {
                throw ParseException("Empty attribute value!");
            }
            uri::uri u(myhtml_attribute_value(attr, NULL));
            // Check whether this is an absolute URL. I'd love to call netlib's
            // is_absolute function, but it's buggy and rejects some valid URLs
            // (such as Google Fonts URLs) :( The scheme function is buggy as
            // well, returning the first component of some relative URLs.
            if (u.host() != "") {
                if (u.scheme() != "http" && u.scheme() != "https") {
                    throw ParseException("Unsupported protocol");
                }
                return u.string();
            } else if (u.string().substr(0, 2) == "//") {
                // This is technically an absolute path, but netlib can't parse
                // the //host/path format. Manually build the URL, using the
                // protocol from baseUrl
                uri::uri rewritten;
                uri::builder(rewritten).scheme(uri::scheme(uri::uri(baseUrl)));
                rewritten.append(u.string().substr(2));
                return rewritten.string();
            } else {
                uri::uri basePath(baseUrl);
                if (u.string().substr(0, 1) == "/") {
                    // This must be an absolute path for this server
                    uri::uri rewritten;
                    uri::builder(rewritten).scheme(uri::scheme(basePath));
                    rewritten.append(uri::authority(basePath));
                    rewritten.append(u.string());
                    return rewritten.string();
                } else if (u.string().substr(0, 1) == "#") {
                    // This is a fragment/hash
                    if (baseUrl.find("#") == std::string::npos)
                        return baseUrl + u.string();
                    return baseUrl.substr(0, baseUrl.find("#")) + u.string();
                } else {
                    // This must be relative to the current file
                    if (uri::uri(baseUrl).path() == "")
                        return baseUrl + "/" + u.string();
                    return baseUrl.substr(0, baseUrl.rfind("/") + 1) + u.string();
                }
            }
            break;
        }
    }
    throw ParseException(std::string("Couldn't find ") + targetAttr
            + "attr in this dom node");
}

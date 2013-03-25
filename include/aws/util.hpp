/******************************************************************************
 * Copyright (c) 2013 SEOmoz
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

/******************************************************************************
 * Utilities needed by all
 *****************************************************************************/

#ifndef AWS__UTIL_HPP
#define AWS__UTIL_HPP

/* Suffice it to say, we need to talk to things. Thus, curl */
#include <curl/curl.h>
/* Some path manipulation */
#include <apathy/path.hpp>
/* Boost headers! */
#include <boost/algorithm/string.hpp>
/* You know, for hash functions */
#if defined(__APPLE__) && defined(__MACH__)
    #define COMMON_DIGEST_FOR_OPENSSL
    #include <CommonCrypto/CommonDigest.h>
    #include <CommonCrypto/CommonHMAC.h>
#else
    #include <openssl/hmac.h>
    #include <openssl/sha.h>
#endif

/* Standard includes */
#include <iostream>
#include <vector>
#include <map>

namespace AWS {
    namespace Curl {
        /* For brevity */
        typedef apathy::Path Path;

        /* The stuff of headers */
        typedef std::vector<std::string>           HeaderValues;
        typedef std::map<std::string,HeaderValues> Headers;

        /* Because curl-provided headers must be freed, wrapping this */
        struct Slist {
            /* Default constructor */
            Slist(): impl(NULL) {}

            /* Fill it with the contents of a headers object */
            Slist(const Headers& headers): impl(NULL) {
                typename Headers::const_iterator hit(headers.begin());
                typename HeaderValues::const_iterator vit;
                for (; hit != headers.end(); ++hit) {
                    for (vit = hit->second.begin();
                         vit != hit->second.end();
                         ++vit) {
                        append(hit->first + ": " + *vit);
                    }
                }
            }

            ~Slist() { curl_slist_free_all(impl); }

            /* Append to it */
            void append(const std::string& line) {
                impl = curl_slist_append(impl, line.c_str());
            }

            curl_slist* slist() { return impl; }
        private:
            curl_slist *impl;

            /* Private, unimplemented to prevent use */
            Slist(const Slist& other);
            const Slist& operator=(const Slist& other);
        };

        /* This is just a way to be able to make a nice wrapper around a curl
         * connection that takes care of all the initialization and so forth.
         * A curl connection is only capable of servicing one request at a
         * time */
        struct Connection {
            Connection()
                :curl(curl_easy_init())
                ,curl_error()
                ,request_headers()
                ,response_headers() {}

            Connection(const Connection& other)
                :curl(curl_easy_init())
                ,curl_error()
                ,request_headers()
                ,response_headers() {}

            ~Connection() {
                curl_easy_cleanup(curl);
            }

            const Connection& operator=(const Connection& other) {
                return *this;
            }

            /* Reset the request */
            void reset();

            /* Return how much data has been downloaded */
            std::size_t downloaded();

            /* Add a header to our request */
            void addHeader(const std::string& key, const std::string& value);

            /* Perform a GET request */
            template <typename T>
            long get(const std::string& host, const Path& path,
                const std::string& query, T& stream);

            /* Perform a PUT request */
            template <typename T, typename S>
            long put(const std::string& host, const Path& path,
                const std::string& query, T& istream, std::size_t size,
                S& ostream);

            /* Get the request headers */
            const Headers& get_request_headers() { return request_headers; }

            /* This is for use with curl when reading a response header. This
             * is not meant to be used, but it has to be public for curl */
            static std::size_t appendHeader_(void *ptr, std::size_t size,
                std::size_t nmemb, void *stream);

            /* This is for use with curl when reading response data and pumping
             * it out to a stream */
            template <typename T>
            static std::size_t appendData_(void* ptr, std::size_t size,
                std::size_t nmemb, void *stream);

            /* This is for use with curl when reading input data from a stream
             * and pumping it out to the server */
            template <typename T>
            static std::size_t readData_(void* ptr, std::size_t size,
                std::size_t nmemb, void *stream);
        private:
            CURL*   curl;
            char    curl_error[CURL_ERROR_SIZE];
            Headers request_headers;
            Headers response_headers;
        };
    }

    namespace Auth {
        /* For brevity */
        typedef AWS::Curl::Headers Headers;

        /* Return a string representative of the canonicalized headers */
        std::string canonicalizedAmzHeaders(const Headers& headers);

        /* Return a string representative of the canonicalized query */
        std::string canonicalizedQueryString(const std::string& query);

        /* Return a signature given the details of the request */
        std::string signature(const std::string& verb, const std::string& md5,
            const std::string& content_type, const std::string& date,
            const Headers& headers, const std::string& url,
            const std::string& secret_key);

        /* Return a date in the correct format */
        std::string date();

        /* Base-64 encoding */
        void b64_enblock(unsigned char in[3], unsigned char out[4], int len);
        void b64_encode(unsigned char *input, size_t inlen,
            unsigned char *output);

        static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    }
}

/******************************************************************************
 * Implementations
 *****************************************************************************/
inline void AWS::Curl::Connection::reset() {
    /* Reset the curl connection */
    curl_easy_reset(curl);
    /* At this point, just reset the request headers */
    request_headers.clear();
    response_headers.clear();
}

inline void AWS::Curl::Connection::addHeader(const std::string& key,
    const std::string& value) {
    request_headers[key].push_back(value);
}

template <typename T>
inline long AWS::Curl::Connection::get(const std::string& host,
    const Path& path, const std::string& query, T& stream) {
    /* Let's begin by putting together some headers */
    Slist slist(request_headers);

    /* With our headers together, we can begin to make a request */
    std::string url = "http://" + host + path.string();
    if (query != "") {
        url += "?" + query;
    }

    /* Set the urls, headers, verb, error buffer and file to write to */
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist.slist());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error);

    /* These came right out of the original code */
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);

    /* Now we'll set up to recieve the header */
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,
        AWS::Curl::Connection::appendHeader_);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, reinterpret_cast<void*>(this));
    /* And how we'll write the data */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        AWS::Curl::Connection::appendData_<T>);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(&stream));
    
    /* If there was an error, return something to indicate that */
    if (curl_easy_perform(curl) != CURLE_OK) {
        return -1;
    }

    /* Now check the status code and return it */
    long response;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
    return response;
}

template <typename T, typename S>
inline long AWS::Curl::Connection::put(const std::string& host,
    const Path& path, const std::string& query, T& istream, std::size_t size,
    S& ostream) {
    /* Let's begin by putting together some headers */
    Slist slist(request_headers);

    std::string url = "http://" + host + path.string();
    if (query != "") {
        url += "?" + query;
    }

    /* Set the url, headers, verb, erro buff, etc. */
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist.slist());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error);

    /* These came right out of the original code */
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 30);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);

    /* Now we'll set up to recieve the header */
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,
        AWS::Curl::Connection::appendHeader_);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, reinterpret_cast<void*>(this));
    /* And how we'll write the data. */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        AWS::Curl::Connection::appendData_<S>);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(&ostream));
    /* And how we'll read the data from the input stream */
    curl_easy_setopt(curl, CURLOPT_READFUNCTION,
        AWS::Curl::Connection::readData_<T>);
    curl_easy_setopt(curl, CURLOPT_READDATA, reinterpret_cast<void*>(&istream));
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, size);

    /* If there was an error, return something to say so */
    if (curl_easy_perform(curl) != CURLE_OK) {
        std::cerr << "Curl error: " << curl_error << std::endl;
        return -1;
    }

    /* Get and return the response */
    long response;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
    return response;
}

inline std::size_t AWS::Curl::Connection::downloaded() {
    double down;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &down);
    return static_cast<std::size_t>(down);
}

inline std::size_t AWS::Curl::Connection::appendHeader_(void *ptr, std::size_t size,
    std::size_t nmemb, void *stream) {
    /* Our user data is a Connection object */
    Connection* conn = reinterpret_cast<Connection*>(stream);
    /* Make a string object and strip off the last CR/LF */
    std::string line(reinterpret_cast<char*>(ptr), size * nmemb);
    line = line.substr(0, line.length() - 2);
    /* Find the ': ' in the line if there is one for our key, value */
    std::size_t pos = line.find(": ");
    if (pos != std::string::npos) {
        std::string key(line.substr(0, pos));
        std::string value(line.substr(pos + 2));
        conn->response_headers[key].push_back(value);
    }
    return size * nmemb;
}

template <typename T>
inline std::size_t AWS::Curl::Connection::appendData_(void* ptr,
    std::size_t size, std::size_t nmemb, void *stream) {
    (*reinterpret_cast<T*>(stream)) << std::string(
        reinterpret_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

template <typename T>
inline std::size_t AWS::Curl::Connection::readData_(void* ptr,
    std::size_t size, std::size_t nmemb, void *stream) {
    /* Apparently istream::read doesn't return the number of bytes read, and so
     * to find out how much has been read, we'll use the get pointer */
    T* istream = reinterpret_cast<T*>(stream);
    istream->read(reinterpret_cast<char*>(ptr), size * nmemb);
    return istream->gcount();
}

/******************************************************************************
 * Auth Implementation
 *****************************************************************************/
inline std::string AWS::Auth::canonicalizedAmzHeaders(const Headers& headers) {
    /* We're copying each header over, because we have to do some manipulation
     * of them. In particular, of the keys */
    std::string key;
    Headers canonical;
    typename Headers::const_iterator it(headers.begin());
    for (; it != headers.end(); ++it) {
        /* Lowercase each of the key names */
        key = boost::algorithm::to_lower_copy(it->first);

        /* Filter out all headers that belong to Amazon */
        if (key.find("x-amz-") == 0) {
            /* It's important to merge these, not replace */
            canonical[key].insert(
                canonical[key].end(), it->second.begin(), it->second.end());
        }
    }

    /* Now go through these lowercased headers in alphabetical order */
    std::string line;
    std::vector<std::string> lines;
    for (it = canonical.begin(); it != canonical.end(); ++it) {
        line = boost::algorithm::join(it->second, ",");
        /* Replace all the newlines in this, and then strip all the leading
         * and trailing whiespace */
        boost::algorithm::replace_all(line, "\n", " ");
        boost::algorithm::trim(line);
        lines.push_back(it->first + ":" + line);
    }
    return boost::algorithm::join(lines, "\n");
}

inline std::string AWS::Auth::date() {
    time_t rawtime;
    char datestr[31];
    struct tm timeinfo;

    /* Get the current time and format it correctly */
    time(&rawtime);
    std::strftime(datestr, 31, "%a, %d %b  %Y %H:%M:%S GMT",
        gmtime_r(&rawtime, &timeinfo));
    return std::string(datestr);
}

inline std::string AWS::Auth::signature(const std::string& verb,
    const std::string& md5, const std::string& content_type,
    const std::string& date, const Headers& headers, const std::string& url,
    const std::string& secret_key) {
    /* Generate the string that we have to sign */
    std::string toSign = verb + "\n" + md5 + "\n" + content_type + "\n"
        + date + "\n" + AWS::Auth::canonicalizedAmzHeaders(headers) + url;

    /* And now we'll begin the signing */
    unsigned char processed[20];
#if defined(__APPLE__) && defined(__MACH__)
    CCHmac(kCCHmacAlgSHA1,
        reinterpret_cast<const void*>(secret_key.c_str()), secret_key.length(),
        reinterpret_cast<const void*>(toSign.c_str()), toSign.length(),
        reinterpret_cast<void*>(processed));
#else
    #error "S3 signing not yet implemented"
    // Should look something like this, but until I can test it...
    std::memcpy(processed,
        HMAC(EVP_sha1(),
        secret_key.c_str(), secret_key.length(),
        toSign.c_str(), toSign.length(0)),
        NULL, NULL),
    20);
#endif
    /* Now we base64-encode the processed string */
    unsigned char b64processed[29];
    std::memset(b64processed, 0, 29);
    AWS::Auth::b64_encode(processed, 20, b64processed);
    // @Martin -- I'm a little unsure about this line
    return std::string(reinterpret_cast<char*>(b64processed));
}

inline void AWS::Auth::b64_enblock(
    unsigned char in[3], unsigned char out[4], int len) {
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | (len > 1 ? ((in[1] & 0xf0) >> 4) : 0 ) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

inline void AWS::Auth::b64_encode(
    unsigned char *input, size_t inlen, unsigned char *output) {
    size_t inindex = 0;
    while(inindex < inlen)
    {
        AWS::Auth::b64_enblock(input+inindex, output, inlen-inindex);
        //printf("%c%c%c -> %c%c%c%c\n", input[inindex], input[inindex+1], input[inindex+2], output[0], output[1], output[2], output[3]);
        inindex += 3;
        output += 4;
    }
}

#endif

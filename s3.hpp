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
* All things S3
*****************************************************************************/

/* Internal utilities */
#include "util.hpp"

/* We use apathy for all path manipulations */
#include <apathy/path.hpp>

/* Standard includes */
#include <iostream>
#include <cstdlib>
#include <string>
#include <locale>
#include <ctime>
#include <cmath>

namespace AWS {
    namespace S3 {
        /* Just to make it a little easier to refer to a path */
        typedef apathy::Path Path;

        /* For brevity */
        typedef AWS::Curl::Headers      Headers;
        typedef AWS::Curl::HeaderValues HeaderValues;

        /* Some backoff policies that can be used when retrying get/fetch
         * operations with the Connection object */
        namespace Backoff {
            /* Linear backoff. Given a number of tries, returns a*x + b */
            struct Linear {
                /* Default parameters */
                Linear(float a=5, float b=0): a(a), b(b) {}

                /* Return how much time to back off */
                float operator()(std::size_t tries) {
                    return a * tries + b;
                }

                float a; // Linear coefficient
                float b; // Constant expression
            };

            /* Exponential backoff. Given a number of tries, a ** tries + b */
            struct Exponential {
                /* Default parameters */
                Exponential(float a=2, float b=0): a(a), b(b) {}

                /* Return how much time to back off */
                float operator()(std::size_t tries) {
                    return std::pow(a, tries) + b;
                }

                float a; // Base
                float b; // Constant expression
            };
        }

        /* A S3 Connection object. When you connect, you provide all your
         * authentication credintials */
        struct Connection {
            /* Otherwise, you can provide them explicitly */
            Connection(
                const std::string& access_id,
                const std::string& secret_key)
                :access_id(access_id)
                ,secret_key(secret_key)
                ,user_agent("awscpp-bot") {}

            /* Download a S3 resource to a local file */
            template <typename T>
            bool get(const std::string& bucket, const Path& object,
                T& stream, std::size_t retries=5) const;

            /* Download a S3 resource to a string and return it */
            std::string get(const std::string& bucket, const Path& object,
                std::size_t retries=5) const;

            /* Post the contents of a stream to a location on S3 */
            template <typename T, typename S>
            bool put(const std::string& bucket, const Path& object,
                T& istream, std::size_t size, S& ostream=std::cerr,
                std::size_t retries=5);

            /* Post the contents of a string to a location on S3. This is /not/
             * a specialization of the template put method because here the
             * stream is a const ref */
            std::string put(const std::string& bucket, const Path& object,
                const std::string& stream, std::size_t retries=5);

            /* Do some S3 authentication y'all */
            bool auth(const std::string& url, const std::string& verb,
                const std::string& contentMD5, const Headers& headers) const;
        private:
            /* We need to know a little bit about the auth here */
            std::string access_id;
            std::string secret_key;
            std::string user_agent;
        };
    }
}

/******************************************************************************
 * Implementation of S3
 *****************************************************************************/
template <typename T>
inline bool AWS::S3::Connection::get(const std::string& bucket,
    const Path& object, T& stream, std::size_t retries) const {
    /* Check the original size of the file so that we can rewind if need be */
    std::streampos position = stream.tellp();

    /* Begin forming our request */
    std::string date = AWS::Auth::date();
    AWS::Curl::Connection curl;
    /* And compute our signature */
    std::string signature = AWS::Auth::signature("GET", "", "", date,
        curl.get_request_headers(), "/" + bucket + object.string(),
        secret_key);

    /* Begin our attempt to fetch */
    long response=0;
    for (std::size_t i = 0; (response != 200) && (i < retries); ++i) {
        stream.seekp(position);
        curl.reset();
        curl.addHeader("User-Agent", user_agent);
        curl.addHeader("Date", date);
        curl.addHeader("Authorization", "AWS " + access_id + ":" + signature);
        response = curl.get(
            bucket + ".s3.amazonaws.com", object.string(), "", stream);
    }

    if (response != 200) {
        std::cerr << curl.error() << std::endl;
    }
    
    stream.flush();
    return response == 200;
}

inline std::string AWS::S3::Connection::get(const std::string& bucket,
    const Path& object, std::size_t retries) const {
    std::ostringstream stream;
    if (get(bucket, object, stream, retries)) {
        return stream.str();
    } else {
        return "Error: " + stream.str();
    }
}

template <typename T, typename S>
inline bool AWS::S3::Connection::put(const std::string& bucket,
    const Path& object, T& istream, std::size_t size, S& ostream,
    std::size_t retries) {
    /* Check the original read position of the stream so we can seek back */
    std::streampos iposition = istream.tellg();
    std::streampos oposition = ostream.tellp();

    /* Begin forming our request */
    std::string date = AWS::Auth::date();
    AWS::Curl::Connection curl;
    /* Compute our signature */
    std::string signature = AWS::Auth::signature("PUT", "", "", date,
        curl.get_request_headers(), "/" + bucket + object.string(),
        secret_key);

    /* Begin our attempts to upload */
    long response = 0;
    for (std::size_t i = 0; (response != 200) && (i < retries); ++i) {
        istream.seekg(iposition);
        ostream.seekp(oposition);
        curl.reset();
        curl.addHeader("User-Agent", user_agent);
        curl.addHeader("Date", date);
        curl.addHeader("Authorization", "AWS " + access_id + ":" + signature);
        response = curl.put(
            bucket + ".s3.amazonaws.com", object.string(), "", istream, size,
            ostream);
    }

    ostream.flush();
    return response == 200;
}

inline std::string AWS::S3::Connection::put(const std::string& bucket,
    const Path& object, const std::string& input, std::size_t retries) {
    std::istringstream istream(input);
    std::ostringstream ostream;
    put(bucket, object, istream, input.size(), ostream, retries);
    return ostream.str();
}

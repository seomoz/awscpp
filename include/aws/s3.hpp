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

namespace AWS {
    namespace S3 {
        /* Just to make it a little easier to refer to a path */
        typedef apathy::Path Path;

        /* For brevity */
        typedef AWS::Curl::Headers      Headers;
        typedef AWS::Curl::HeaderValues HeaderValues;

        /* A S3 Connection object. When you connect, you provide all your
         * authentication credintials */
        struct Connection {
            /* By default, load the environment variables */
            Connection()
                :access_id(getenv("AWS_ACCESS_ID"))
                ,secret_key(getenv("AWS_SECRET_KEY"))
                ,curl() {}

            /* Otherwise, you can provide them explicitly */
            Connection(
                const std::string& access_id,
                const std::string& secret_key)
                :access_id(access_id)
                ,secret_key(secret_key)
                ,curl() {}

            /* Download a S3 resource to a local file */
            bool get(const std::string& bucket, const Path& object,
                const Path& local);

            /* Do some S3 authentication y'all */
            bool auth(const std::string& url, const std::string& verb,
                const std::string& contentMD5, const Headers& headers);
        private:
            /* We need to know a little bit about the auth here */
            std::string access_id;
            std::string secret_key;

            /* We also need to be able to make requests */
            AWS::Curl::Connection curl;

            /* Canonicalize the query string */
            std::string canonicalizedQueryString();
        };
    }
}

/******************************************************************************
 * Implementation of S3
 *****************************************************************************/
inline bool AWS::S3::Connection::get(const std::string& bucket,
    const Path& object, const Path& local) {
    /* Before we begin, make sure we can open the file */
    FILE* fp = fopen(local.string().c_str(), "w+");
    if (!fp) {
        std::cerr << "Error opening file" << std::endl;
        return false;
    }

    /* Begin forming our request */
    std::string date = AWS::Auth::date();
    curl.reset();
    curl.addHeader("User-Agent", "danbot");
    curl.addHeader("Date", date);

    /* Now we're ready to submit our 'GET' */
    std::string signature = AWS::Auth::signature("GET", "", "", date,
        curl.get_request_headers(), "/" + bucket + object.string(),
        secret_key);

    /* Lastly, we'll add our authorization header */
    curl.addHeader("Authorization", "AWS " + access_id + ":" + signature);
    long response = curl.get(
        bucket + ".s3.amazonaws.com", object.string(), "", fp);
    std::cout << "Response: " << response << std::endl;
    return response == 200;
}

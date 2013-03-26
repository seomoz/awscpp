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

#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include <apathy/path.hpp>
#include <boost/algorithm/string.hpp>

/* The things we need to actually test */
#include "aws.hpp"

using namespace apathy;

TEST_CASE("auth", "Auth module works as expected") {
    SECTION("headers", "Can correctly canonicalize headers") {
        AWS::Curl::Headers headers;
        /* Auth canonicalization shouldn't care about this header */
        headers["foo"].push_back("hello");
        /* Auth canonicalization shouldn't care about case */
        headers["X-AMZ-a"].push_back("hello");
        headers["x-AmZ-b"].push_back("hello");
        headers["x-amZ-z"].push_back("hello");
        headers["x-Amz-c"].push_back("hello");
        /* It also should allow for multiple values for a given header */
        headers["x-amz-a"].push_back("howdy");
        /* Lastly, it should take care of newlines and strip whitespace */
        headers["x-AmZ-x"].push_back("   hello\nhowdy\n");
        /* Now, we expect to see all of headers correctly formatted, in
         * alphabetical order, with multiple values joined by commas, and
         * all newlines replaced with single spaces and all leading and
         * trailing whitespace cleaned up */
        std::string pieces_[5] = {
            "x-amz-a:hello,howdy",
            "x-amz-b:hello",
            "x-amz-c:hello",
            "x-amz-x:hello howdy",
            "x-amz-z:hello"
        };
        std::vector<std::string> pieces(&pieces_[0], &pieces_[0]+5);
        std::string expected = boost::algorithm::join(pieces, "\n");
        REQUIRE(expected == AWS::Auth::canonicalizedAmzHeaders(headers));
    }

    SECTION("signature", "Can correctly generate a couple trial signatures") {
        AWS::Curl::Headers headers;

        /* It's difficult to say that this is correct, so we'll just try
         * several examples that are known to work */
        std::string signature = AWS::Auth::signature(
            "PUT", "1234567890", "text/html", "Tue, 26 Mar  2013 21:14:41 GMT",
            headers, "/foo", "this is a secret");
        REQUIRE(signature == "Y17QeAkX2ntLsrAH2+ZFW7+x/QU=");

        /* Let's change things up a bit and make sure it's correct and in
         * particular that it's not the same as above */
        signature = AWS::Auth::signature(
            "PUT", "1234567890", "text/html", "Tue, 26 Mar  2013 21:14:41 GMT",
            headers, "/foo", "this is not a secret");
        REQUIRE(signature == "10siG57hhLjp7RwUW8smhxrpSu4=");

        /* Different verb */
        signature = AWS::Auth::signature(
            "GET", "1234567890", "text/html", "Tue, 26 Mar  2013 21:14:41 GMT",
            headers, "/foo", "this is a secret");
        REQUIRE(signature == "aP4Esas6h2VMpsgxjB7xX3hQrHw=");

        /* Different md5 */
        signature = AWS::Auth::signature(
            "PUT", "12345abcde", "text/html", "Tue, 26 Mar  2013 21:14:41 GMT",
            headers, "/foo", "this is a secret");
        REQUIRE(signature == "TQa3VOQMZRcNAVuBNtWHFdQv2Ik=");

        /* Different content type */
        signature = AWS::Auth::signature(
            "PUT", "1234567890", "text/csv", "Tue, 26 Mar  2013 21:14:41 GMT",
            headers, "/foo", "this is a secret");
        REQUIRE(signature == "BgPP+sQXJUGReHQjXdz1/ldTdVA=");

        /* Different date */
        signature = AWS::Auth::signature(
            "PUT", "1234567890", "text/html", "Tue, 26 Feb  2013 21:14:41 GMT",
            headers, "/foo", "this is a secret");
        REQUIRE(signature == "lc/e1kq4Cp4G8nMHGmmzy4bnDwo=");

        /* Different path */
        signature = AWS::Auth::signature(
            "PUT", "1234567890", "text/html", "Tue, 26 Mar  2013 21:14:41 GMT",
            headers, "/bar", "this is a secret");
        REQUIRE(signature == "Ho4ASmVvjS2yeSmelF82UBOjKsY=");

        /* Let's add a few headers and try again, and make sure they differ */
        headers["x-amz-foo"].push_back("hello");
        headers["x-amz-bar"].push_back("whiz");
        signature = AWS::Auth::signature(
            "PUT", "1234567890", "text/html", "Tue, 26 Mar  2013 21:14:41 GMT",
            headers, "/foo", "this is a secret");
        REQUIRE(signature == "20MV2sdcxFPjZCWm25nCJ7gpQ5o=");
    }
}

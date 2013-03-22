#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include <apathy/path.hpp>
#include <boost/algorithm/string.hpp>

/* The things we need to actually test */
#include "aws/aws.hpp"

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
}

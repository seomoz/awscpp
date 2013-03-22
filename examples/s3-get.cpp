/* An example of fetching */

#include <aws/s3.hpp>

#include <iostream>

int main(int argc, char** argv) {
    /* By default, it will load your local environment variables */
    AWS::S3::Connection conn;

    /* Now get a particular file from S3 and save it locally */
    if (conn.get("urlschedulingtest", "/urls.monthly.0.0/urls.monthly.0.0.authority.lzoc.1363679153.manifest", "localpath")) {
        std::cout << "We fetched a page!" << std::endl;
        return 0;
    } else {
        /* Oh no! We failed! */
        std::cout << "We failed fetching a page!" << std::endl;
        return 1;
    }
}

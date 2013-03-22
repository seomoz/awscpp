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

/* An example of fetching */

#include <aws/s3.hpp>

#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    /* By default, it will load your local environment variables */
    AWS::S3::Connection conn;

    /* Now get a particular file from S3 and save it locally */
    std::ofstream out("localpath");
    if (conn.get("urlschedulingtest", "/urls.monthly.0.0/urls.monthly.0.0.authority.lzoc.1363679153.manifest", out)) {
        std::cout << "We fetched a page!" << std::endl;
        return 0;
    } else {
        /* Oh no! We failed! */
        std::cout << "We failed fetching a page!" << std::endl;
        return 1;
    }
}

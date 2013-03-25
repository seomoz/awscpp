AWS C++ Bindings
================
This will be a hopefully growing collection of C++ bindings to various AWS
services. I'm slowly porting features out of our existing C++ bindings into
something a little more usable and modular.

Structure
---------
There's a top-level namespace `AWS`, and each service is a namespace within
it. For example, we currently have `AWS::S3`, `AWS::Auth` and `AWS::Curl`.

S3
=====
To make use of S3, you first have to create a connection object. It will by
default read your environment for the variables `AWS_ACCESS_ID` and
`AWS_SECRET_KEY`.

GET
---
With a connection object, you can get objects out of a bucket:

```c++
#include <aws/aws.hpp>

...

// Any ostream will work here
std::ofstream out("local/download/path");

AWS::S3::Connection s3;
if (s3.get("bucket", "/object", out) {
    std::cout << "Success!" << std::endl;
} else {
    std::cout << "Failure" << std::endl;
}
```

It's designed to work with any `ostream`, but for convenience there's a way to
return the contents of an object as a string:

```c++
std::cout << s3.get("bucket", "/object") << std::endl;
```

PUT
---
`PUT` operations don't generally return responses, except on errors. Because
the function is templated to accept a generalized `istream` for input, you must
know the size ahead of time. You can optionally provide an `ostream` to take
any output, though it defaults to `std::cerr`:

```c++
apathy::Path localpath("local/path/to/upload")
std::ifstream istream(localpath.string());

if (s3.put("bucket", "/object", istream, istream.size())) {
    std::cout << "Success!" << std::endl;
} else {
    std::cout << "Failure" << std::endl;
}
```

Like `GET`, there's a string version as well for convenience:

```c++
s3.put("bucket", "/object", "Hello, world!");
```
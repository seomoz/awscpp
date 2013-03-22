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
`AWS_SECRET_KEY`. With this object in hand, you can get objects:

```c++
#include <aws/aws.hpp>

...

AWS::S3::Connection s3;
if (s3.get("bucket", "object", "local/download/path")) {
    std::cout << "Success!" << std::endl;
} else {
    std::cout << "Failure" << std::endl;
}
```

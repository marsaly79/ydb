GO_LIBRARY()

LICENSE(
    Apache-2.0 AND
    BSD-2-Clause AND
    BSD-3-Clause AND
    BSL-1.0 AND
    CC-BY-3.0 AND
    HPND AND
    MIT AND
    NCSA AND
    OpenSSL AND
    Zlib
)

SRCS(
    arrjson.go
    option.go
    reader.go
    writer.go
)

GO_TEST_SRCS(arrjson_test.go)

END()
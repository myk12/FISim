#find_external_library(
#  DEPENDENCY_NAME openssl
#  LIBRARY_NAME libcrypto.so
#)


build_lib(
    LIBNAME cybertwin
    SOURCE_FILES
        helper/cybertwin-helper.cc
        model/cybertwin-node.cc
        model/cybertwin-header.cc
        model/cybertwin-common.cc
        model/cybertwin.cc
        model/cybertwin-tag.cc
        model/cybertwin-client.cc
        model/cybertwin-multipath-controller.cc
        model/cybertwin-packet-tags.cc
        model/cybertwin-name-resolution-service.cc
        model/cybertwin-manager.cc
    HEADER_FILES
        helper/cybertwin-helper.h
        model/cybertwin-node.h
        model/cybertwin-header.h
        model/cybertwin-common.h
        model/cybertwin.h
        model/cybertwin-tag.h
        model/cybertwin-client.h
        model/cybertwin-multipath-controller.h
        model/cybertwin-packet-tags.h
        model/cybertwin-name-resolution-service.h
        model/cybertwin-manager.h
    LIBRARIES_TO_LINK ${libcore}
                        ${libapplications}
                        ${libinternet}
                        "crypto.so"

    TEST_SOURCES test/cybertwin-test-suite.cc
                 ${examples_as_tests_sources}
)
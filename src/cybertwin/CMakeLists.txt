build_lib(
    LIBNAME cybertwin
    SOURCE_FILES
        helper/cybertwin-bulk-client-helper.cc
        helper/cybertwin-edge-server-helper.cc
        helper/cybertwin-helper.cc
        model/cybertwin.cc
        model/cybertwin-bulk-client.cc
        model/cybertwin-edge-server.cc
        model/cybertwin-packet-header.cc
        model/cybertwin-name-resolution-service.cc
    HEADER_FILES
        helper/cybertwin-bulk-client-helper.h
        helper/cybertwin-edge-server-helper.h
        helper/cybertwin-helper.h
        model/cybertwin.h
        model/cybertwin-bulk-client.h
        model/cybertwin-edge-server.h
        model/cybertwin-packet-header.h
        model/cybertwin-name-resolution-service.h
    LIBRARIES_TO_LINK ${libcore}
                        ${libapplications}
                        ${libinternet}
    TEST_SOURCES test/cybertwin-test-suite.cc
                 ${examples_as_tests_sources}
)
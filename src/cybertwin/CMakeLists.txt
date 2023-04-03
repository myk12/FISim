build_lib(
    LIBNAME cybertwin
    SOURCE_FILES
        helper/cybertwin-helper.cc
        model/cybertwin-header.cc
        model/cybertwin-edge.cc
        model/cybertwin-common.cc
        model/cybertwin.cc
        model/cybertwin-cert.cc
        model/cybertwin-client.cc
    HEADER_FILES
        helper/cybertwin-helper.h
        model/cybertwin-header.h
        model/cybertwin-edge.h
        model/cybertwin-common.h
        model/cybertwin.h
        model/cybertwin-cert.h
        model/cybertwin-client.h
    LIBRARIES_TO_LINK ${libcore}
                        ${libapplications}
                        ${libinternet}
    TEST_SOURCES test/cybertwin-test-suite.cc
                 ${examples_as_tests_sources}
)
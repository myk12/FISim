build_lib(
    LIBNAME cybertwin
    SOURCE_FILES
        helper/cybertwin-helper.cc
        #helper/cybertwin-node-coreserver.cc
        #helper/cybertwin-node-edgeserver.cc
        #helper/cybertwin-node-endhost.cc
        model/cybertwin-header.cc
        model/cybertwin-edge.cc
        model/cybertwin-common.cc
        model/cybertwin.cc
        model/cybertwin-tag.cc
        model/cybertwin-client.cc
        model/cybertwin-multipath-controller.cc
        model/cybertwin-packet-tags.cc
        model/cybertwin-name-resolution-service.cc
    HEADER_FILES
        helper/cybertwin-helper.h
        #helper/cybertwin-node-coreserver.h
        #helper/cybertwin-node-edgeserver.h
        #helper/cybertwin-node-endhost.h
        model/cybertwin-header.h
        model/cybertwin-edge.h
        model/cybertwin-common.h
        model/cybertwin.h
        model/cybertwin-tag.h
        model/cybertwin-client.h
        model/cybertwin-multipath-controller.h
        model/cybertwin-packet-tags.h
        model/cybertwin-name-resolution-service.h
    LIBRARIES_TO_LINK ${libcore}
                        ${libapplications}
                        ${libinternet}
    TEST_SOURCES test/cybertwin-test-suite.cc
                 ${examples_as_tests_sources}
)
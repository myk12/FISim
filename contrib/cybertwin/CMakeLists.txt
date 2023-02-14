check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/cybertwin-examples-test-suite.cc
        )
endif()

set(source_files
    model/cybertwin.cc
    helper/cybertwin-helper.cc)

set(header_files
    model/cybertwin.h
    helper/cybertwin-helper.h)


build_lib(
    LIBNAME cybertwin
    SOURCE_FILES ${source_files}
    HEADER_FILES ${header_files}
    LIBRARIES_TO_LINK ${libcore}
                        ${libapplications}
                        ${libinternet}
    TEST_SOURCES test/cybertwin-test-suite.cc
                 ${examples_as_tests_sources}
)


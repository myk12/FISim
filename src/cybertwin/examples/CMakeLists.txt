build_lib_example(
    NAME cybertwin-example
    SOURCE_FILES cybertwin-example.cc
    LIBRARIES_TO_LINK ${libcybertwin}
                    ${libpoint-to-point}
)

build_lib_example(
    NAME cybertwin-multipath-transfer
    SOURCE_FILES cybertwin-multipath-datatransfer.cc
    LIBRARIES_TO_LINK ${libcybertwin}
                    ${libpoint-to-point}
                    ${libapplications}
                    ${libinternet}
                    ${libnetwork}
                    ${libcsma}
)
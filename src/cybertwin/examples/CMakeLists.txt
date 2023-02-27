build_lib_example(
    NAME cybertwin-example
    SOURCE_FILES cybertwin-example.cc
    LIBRARIES_TO_LINK ${libcybertwin}
                    ${libpoint-to-point}
)

build_lib_example(
    NAME cybertwin-name-resolution-example
    SOURCE_FILES cybertwin-name-resolution-service-example.cc
    LIBRARIES_TO_LINK ${libcybertwin}
                    ${libpoint-to-point}
)
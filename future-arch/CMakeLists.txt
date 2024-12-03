# Content Centric Networking
add_subdirectory(CCN)

# Cybertwin Network
add_subdirectory(Cybertwin)

# Test lte
build_exec(
    EXECNAME "lte-test"
    EXECNAME_PREFIX "lte"
    SOURCE_FILES "lte-test.cc"
    LIBRARIES_TO_LINK "${ns3-libs}"
    EXECUTABLE_DIRECTORY_PATH "lte-test/"
)

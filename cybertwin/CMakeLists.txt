set(target_prefix cybertwin)
set(source_files
    cybertwin.cc)

set(header_files
    cybertwin.h)


build_exec(
    EXECNAME "cybertwinv1"
    EXECNAME_PREFIX target_prefix
    SOURCE_FILES "version2.cc"
    LIBRARIES_TO_LINK "${ns3-libs}"
    EXECUTABLE_DIRECTORY_PATH "cybertwin/"
)

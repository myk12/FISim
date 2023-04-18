set(target_prefix cybertwin)
set(source_files
    version3.cc
    cybertwin-topology.cc)

set(header_files
    cybertwin-topology.h)


build_exec(
    EXECNAME "cybertwin"
    EXECNAME_PREFIX target_prefix
    SOURCE_FILES ${source_files}
    HEADER_FILES ${header_files}
    LIBRARIES_TO_LINK "${ns3-libs}"
    EXECUTABLE_DIRECTORY_PATH "cybertwinexec/"
)

set(ccn_target_prefix "ccn")
set(ccn_target_name "ccn")

set(cybertwin_target_prefix "cybertwin")
set(cybertwin_target_name "cybertwin")


set(ccn_source_files
    "CCN/simulator.cc"
)

set(cybertwin_source_files
    "Cybertwin/simulator.cc"
)

set(cybertwin_heders
    "Cybertwin/simulator.h"
)

build_exec(
    EXECNAME ${ccn_target_name}
    EXECNAME_PREFIX ${ccn_target_prefix}
    SOURCE_FILES ${ccn_source_files}
    LIBRARIES_TO_LINK "${ns3-libs}"
    EXECUTABLE_DIRECTORY_PATH "${ccn_target_prefix}exec/"
)

build_exec(
    EXECNAME ${cybertwin_target_name}
    EXECNAME_PREFIX ${cybertwin_target_prefix}
    SOURCE_FILES ${cybertwin_source_files}
    LIBRARIES_TO_LINK "${ns3-libs}"
                        yaml-cpp
    EXECUTABLE_DIRECTORY_PATH "${cybertwin_target_prefix}exec/"
)

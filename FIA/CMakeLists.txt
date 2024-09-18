set(target_prefix CCN)
set(target_name ${target_prefix}Sim)

set(source_files
    ${target_prefix}/simulator.cc
)

build_exec(
    EXECNAME ${target_name}
    EXECNAME_PREFIX ${target_prefix}
    SOURCE_FILES ${source_files}
    LIBRARIES_TO_LINK "${ns3-libs}"
    EXECUTABLE_DIRECTORY_PATH "${target_prefix}exec/"
)


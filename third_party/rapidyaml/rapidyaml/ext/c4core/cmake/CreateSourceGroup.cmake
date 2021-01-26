# create hierarchical source groups based on a dir tree
#
# EXAMPLE USAGE:
#
#    create_source_group("src" "${SRC_ROOT}" "${SRC_LIST}")
#
# Visual Studio usually has the equivalent to this:
#
#    create_source_group("Header Files" ${PROJ_SRC_DIR} "${PROJ_HEADERS}")
#    create_source_group("Source Files" ${PROJ_SRC_DIR} "${PROJ_SOURCES}")
#
# TODO: <jpmag> this was taken from a stack overflow answer. Need to find it
# and add a link here.

macro(create_source_group GroupPrefix RootDir ProjectSources)
  set(DirSources ${ProjectSources})
  foreach(Source ${DirSources})
    #message(STATUS "s=${Source}")
    string(REGEX REPLACE "${RootDir}" "" RelativePath "${Source}")
    #message(STATUS "  ${RelativePath}")
    string(REGEX REPLACE "[\\\\/][^\\\\/]*$" "" RelativePath "${RelativePath}")
    #message(STATUS "  ${RelativePath}")
    string(REGEX REPLACE "^[\\\\/]" "" RelativePath "${RelativePath}")
    #message(STATUS "  ${RelativePath}")
    string(REGEX REPLACE "/" "\\\\\\\\" RelativePath "${RelativePath}")
    #message(STATUS "  ${RelativePath}")
    source_group("${GroupPrefix}\\${RelativePath}" FILES ${Source})
    #message(STATUS "  ${Source}")
  endforeach(Source)
endmacro(create_source_group)

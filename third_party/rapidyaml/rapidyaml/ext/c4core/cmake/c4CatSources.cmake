if(NOT _c4CatSourcesIncluded)
set(_c4CatSourcesIncluded ON)


#------------------------------------------------------------------------------
# concatenate the source files to an output file, adding preprocessor adjustment
# for correct file/line reporting
function(c4_cat_sources files output umbrella_target)
    _c4_cat_sources_create_cat(cat)
    c4_to_full_path("${files}" full_files) # we must work with full paths
    c4_separate_list("${full_files}" sepfiles) # and use a string instead of a list
    c4_dbg("${_c4_prefix}: catting sources to ${output}")
    if(NOT EXISTS "${output}")
        # the cat command is executed at build time, but we need the output
        # file to exist to be able to create the target. so to bootstrap, just
        # run the command now
        c4_dbg("${_c4_prefix}: creating ${output} for the first time")
        execute_process(
            COMMAND ${cat} "${sepfiles}" "${output}"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            )
    else()
        c4_dbg("output exists: ${output}")
    endif()
    # add a custom command invoking our cat script for the input files
    add_custom_command(OUTPUT ${output}
        COMMAND ${cat} "${sepfiles}" "${output}"
        DEPENDS ${files}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "concatenating sources to ${output}")
    if(NOT TARGET ${umbrella_target})
        add_custom_target(${umbrella_target} DEPENDS ${output} ${files})
    endif()
endfunction(c4_cat_sources)


#------------------------------------------------------------------------------
# get a cat script
function(_c4_cat_sources_create_cat catfile)
    # create a script to concatenate the sources
    if(WIN32)
        set(cat ${CMAKE_BINARY_DIR}/_c4catfiles.bat)
        set(cattmp ${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/_c4catfiles.bat)
    else()
        set(cat ${CMAKE_BINARY_DIR}/_c4catfiles.sh)
        set(cattmp ${CMAKE_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/_c4catfiles.sh)
    endif()
    set(${catfile} ${cat} PARENT_SCOPE)
    if(NOT EXISTS ${cat})
        if(WIN32)
            file(WRITE ${cattmp} "
setlocal EnableDelayedExpansion
set \"src_files=%1\"
set \"out_file=%2\"
echo.>\"out_file%\"
for %%f in (%src_files%) do (
    echo.>>\"%out_file%\"
    echo.>>\"%out_file%\"
    echo \"/*BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB*/\".>>\"%out_file%\"
    echo \"/*BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB*/\".>>\"%out_file%\"
    echo \"/*BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB*/\".>>\"%out_file%\"
    echo \"#line 1 \\\"%%f\\\" // reset __LINE__ and __FILE__ to the correct value\".>>\"%out_file%\"
    type %%f>>\"%out_file%\"
)
")
        else()
            file(WRITE ${cattmp} "#!/bin/sh

src_files=$1
out_file=$2
#echo \"src_files $src_files\"
#echo \"out_file $out_file\"

cat > $out_file << EOF
// DO NOT EDIT.
// this is an auto-generated file, and will be overwritten
EOF
for f in $src_files ; do
    cat >> $out_file <<EOF


/*BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB*/
/*BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB*/
/*BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB*/
#line 1 \"$f\"
EOF
    cat $f >> $out_file
done

echo \"Wrote output to $out_file\"
")
        endif()
        # add execute permissions
        get_filename_component(catdir ${cat} DIRECTORY)
        file(COPY ${cattmp} DESTINATION ${catdir}
            FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
            )
    endif()
endfunction()


endif(NOT _c4CatSourcesIncluded)

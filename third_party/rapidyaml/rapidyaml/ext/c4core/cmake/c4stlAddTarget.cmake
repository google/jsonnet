include(c4project)

function(c4stl_add_target name)
    c4_add_target(c4stl ${name} ${ARGN})
endfunction() # c4stl_add_target

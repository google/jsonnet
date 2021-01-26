c4_setup_testing(DOCTEST)

# doctest does not deal well with template operator<< for substr
# see https://github.com/onqtam/doctest/pull/431
target_compile_definitions(c4core PUBLIC -DC4_SUBSTR_NO_OSTREAM_LSHIFT)
if(C4CORE_SANITIZE)
    foreach(s asan tsan msan ubsan)
        if(TARGET c4core-${s})
            target_compile_definitions(c4core-${s} PUBLIC -DC4_SUBSTR_NO_OSTREAM_LSHIFT)
        endif()
    endforeach()
endif()

c4_add_library(c4core-_libtest LIBRARY_TYPE STATIC
    SOURCES
        c4/test.hpp
        c4/main.cpp
        c4/libtest/test.cpp
        c4/libtest/archetypes.cpp
        c4/libtest/archetypes.hpp
        c4/libtest/supprwarn_push.hpp
        c4/libtest/supprwarn_pop.hpp
    LIBS c4core doctest
    INC_DIRS ${CMAKE_CURRENT_LIST_DIR}
    FOLDER test
    )

function(c4core_test name)
    c4_add_executable(c4core-test-${name}
        SOURCES ${ARGN}
        INC_DIRS ${CMAKE_CURRENT_LIST_DIR}
        LIBS c4core-_libtest
        FOLDER test)
    c4_add_test(c4core-test-${name} ARGS --duration=on --no-version=on)
endfunction()

c4core_test(preprocessor     test_preprocessor.cpp)
c4core_test(type_name        test_type_name.cpp)
c4core_test(types            test_types.cpp)
c4core_test(szconv           test_szconv.cpp)
c4core_test(error            test_error.cpp)
c4core_test(error_exception  test_error_exception.cpp)
c4core_test(blob             test_blob.cpp)
c4core_test(memory_util      test_memory_util.cpp)
c4core_test(memory_resource  test_memory_resource.cpp)
c4core_test(allocator        test_allocator.cpp)
c4core_test(ctor_dtor        test_ctor_dtor.cpp)
c4core_test(chartraits       test_char_traits.cpp)
c4core_test(enum             test_enum.cpp)
c4core_test(bitmask          test_bitmask.cpp)
c4core_test(span             test_span.cpp)
c4core_test(substr           test_substr.cpp)
c4core_test(charconv         test_charconv.cpp)
c4core_test(format           test_format.cpp)
c4core_test(base64           test_base64.cpp)
c4core_test(std_string       test_std_string.cpp)
c4core_test(std_vector       test_std_vector.cpp)


c4_add_install_include_test(c4core "c4core::")
c4_add_install_link_test(c4core "c4core::" "
#include <c4/substr.hpp>
int main()
{
    c4::csubstr s = \"12345\";
    return s.len != decltype(s.len)(5);
}
")

// charconv is because the 0.5.0 release has a bug in the single-header build.
// https://github.com/biojppm/rapidyaml/issues/364#issuecomment-1536625415
#include <charconv>

#define RYML_SINGLE_HDR_DEFINE_NOW
#include "ryml_all.hpp"

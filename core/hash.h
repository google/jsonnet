#pragma once

#include <string>

namespace jsonnet {
namespace hash {

std::string Sha256(const std::string& input);

std::string Md5(const std::string& input);

}  // namespace hash
}  // namespace jsonnet

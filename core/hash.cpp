#include "core/hash.h"

#include "absl/strings/escaping.h"
#include "openssl/bio.h"
#include "openssl/digest.h"

namespace jsonnet {
namespace hash {

namespace {

std::string digest(const std::string& input, const EVP_MD* alg)
{
    uint8_t digest[EVP_MAX_MD_SIZE];
    unsigned int digest_length = 0;
    if (EVP_Digest(input.data(), input.size(), digest, &digest_length, alg, nullptr) != 1) {
        return "";
    }
    return absl::BytesToHexString(
        std::string(reinterpret_cast<const char*>(digest), digest_length));
}
}  // namespace

std::string Sha256(const std::string& input)
{
    return digest(input, EVP_sha256());
}

std::string Md5(const std::string& input)
{
    return digest(input, EVP_md5());
}

}  // namespace hash
}  // namespace jsonnet

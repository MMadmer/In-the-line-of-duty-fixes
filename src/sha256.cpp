#include "sha256.h"

#include <Wincrypt.h>

#include <array>
#include <fstream>

namespace ild
{
namespace
{
class CryptoProvider
{
public:
    CryptoProvider()
    {
        valid_ = CryptAcquireContextW(&handle_, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) != FALSE;
    }

    ~CryptoProvider()
    {
        if (handle_)
        {
            CryptReleaseContext(handle_, 0);
        }
    }

    [[nodiscard]] HCRYPTPROV get() const { return handle_; }
    [[nodiscard]] bool valid() const { return valid_; }

private:
    HCRYPTPROV handle_{};
    bool valid_{};
};

class CryptoHash
{
public:
    explicit CryptoHash(HCRYPTPROV provider)
    {
        valid_ = CryptCreateHash(provider, CALG_SHA_256, 0, 0, &handle_) != FALSE;
    }

    ~CryptoHash()
    {
        if (handle_)
        {
            CryptDestroyHash(handle_);
        }
    }

    [[nodiscard]] HCRYPTHASH get() const { return handle_; }
    [[nodiscard]] bool valid() const { return valid_; }

private:
    HCRYPTHASH handle_{};
    bool valid_{};
};

[[nodiscard]] bool finish_hash(HCRYPTHASH hash, Sha256& digest)
{
    DWORD size = static_cast<DWORD>(digest.size());
    return CryptGetHashParam(hash, HP_HASHVAL, reinterpret_cast<BYTE*>(digest.data()), &size, 0) != FALSE &&
        size == digest.size();
}
}

bool sha256_file(const std::filesystem::path& path, Sha256& digest)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return false;
    }

    CryptoProvider provider;
    CryptoHash hash(provider.get());
    if (!provider.valid() || !hash.valid())
    {
        return false;
    }

    std::array<char, 64 * 1024> buffer{};
    while (stream)
    {
        stream.read(buffer.data(), buffer.size());
        const auto read = stream.gcount();
        if (read > 0 && CryptHashData(hash.get(), reinterpret_cast<const BYTE*>(buffer.data()),
                static_cast<DWORD>(read), 0) == FALSE)
        {
            return false;
        }
    }

    return stream.eof() && finish_hash(hash.get(), digest);
}

bool sha256_bytes(std::span<const std::byte> bytes, Sha256& digest)
{
    CryptoProvider provider;
    CryptoHash hash(provider.get());
    if (!provider.valid() || !hash.valid())
    {
        return false;
    }

    if (!bytes.empty() && CryptHashData(hash.get(), reinterpret_cast<const BYTE*>(bytes.data()),
            static_cast<DWORD>(bytes.size()), 0) == FALSE)
    {
        return false;
    }

    return finish_hash(hash.get(), digest);
}
}

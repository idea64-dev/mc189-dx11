#include "util/UUID.h"

#include "util/Random.h"

#include <array>
#include <charconv>
#include <cstring>
#include <vector>

namespace mc {

namespace {

// RFC 1321 MD5, for nameUUIDFromBytes.
std::array<uint8_t, 16> md5(std::string_view data)
{
    static constexpr uint32_t K[64] { 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8,
        0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8, 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9,
        0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
        0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3,
        0x8f0ccc92, 0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
    static constexpr int R[64] { 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 4, 11, 16,
        23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21 };
    std::vector<uint8_t> msg(data.begin(), data.end());
    const uint64_t bitLength = static_cast<uint64_t>(data.size()) * 8;
    msg.push_back(0x80);
    while (msg.size() % 64 != 56)
        msg.push_back(0);
    for (int i = 0; i < 8; ++i)
        msg.push_back(static_cast<uint8_t>(bitLength >> (8 * i)));
    uint32_t a0 = 0x67452301;
    uint32_t b0 = 0xefcdab89;
    uint32_t c0 = 0x98badcfe;
    uint32_t d0 = 0x10325476;
    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t m[16];
        for (int i = 0; i < 16; ++i) {
            m[i] = static_cast<uint32_t>(msg[chunk + i * 4]) | static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 8
                | static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 16 | static_cast<uint32_t>(msg[chunk + i * 4 + 3]) << 24;
        }
        uint32_t a = a0;
        uint32_t b = b0;
        uint32_t c = c0;
        uint32_t d = d0;
        for (int i = 0; i < 64; ++i) {
            uint32_t f;
            int g;
            if (i < 16) {
                f = (b & c) | (~b & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | (~d & c);
                g = (5 * i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3 * i + 5) % 16;
            } else {
                f = c ^ (b | ~d);
                g = (7 * i) % 16;
            }
            const uint32_t temp = d;
            d = c;
            c = b;
            const uint32_t x = a + f + K[i] + m[g];
            b = b + ((x << R[i]) | (x >> (32 - R[i])));
            a = temp;
        }
        a0 += a;
        b0 += b;
        c0 += c;
        d0 += d;
    }
    std::array<uint8_t, 16> out {};
    const uint32_t words[4] { a0, b0, c0, d0 };
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j)
            out[static_cast<size_t>(i * 4 + j)] = static_cast<uint8_t>(words[i] >> (8 * j));
    }
    return out;
}

void appendHex(std::string& out, uint64_t value, int digits)
{
    static constexpr char hex[] = "0123456789abcdef";
    for (int i = digits - 1; i >= 0; --i)
        out += hex[(value >> (i * 4)) & 15];
}

} // namespace

std::string UUID::toString() const
{
    const auto msb = static_cast<uint64_t>(mostSigBits);
    const auto lsb = static_cast<uint64_t>(leastSigBits);
    std::string out;
    out.reserve(36);
    appendHex(out, msb >> 32, 8);
    out += '-';
    appendHex(out, msb >> 16, 4);
    out += '-';
    appendHex(out, msb, 4);
    out += '-';
    appendHex(out, lsb >> 48, 4);
    out += '-';
    appendHex(out, lsb, 12);
    return out;
}

std::optional<UUID> UUID::fromString(std::string_view text)
{
    std::vector<std::string_view> parts;
    size_t start = 0;
    while (true) {
        const size_t dash = text.find('-', start);
        parts.push_back(text.substr(start, dash == std::string_view::npos ? std::string_view::npos : dash - start));
        if (dash == std::string_view::npos)
            break;
        start = dash + 1;
    }
    if (parts.size() != 5)
        return std::nullopt;
    int64_t values[5];
    for (int i = 0; i < 5; ++i) {
        // Java: Long.decode("0x" + part) -- a leading sign is allowed after the prefix only as part of the
        // number, and values above Long.MAX_VALUE fail.
        std::string_view part = parts[static_cast<size_t>(i)];
        bool negative = false;
        if (!part.empty() && (part[0] == '-' || part[0] == '+')) {
            negative = part[0] == '-';
            part.remove_prefix(1);
        }
        if (part.empty())
            return std::nullopt;
        uint64_t v = 0;
        const auto result = std::from_chars(part.data(), part.data() + part.size(), v, 16);
        if (result.ec != std::errc() || result.ptr != part.data() + part.size() || v > static_cast<uint64_t>(INT64_MAX))
            return std::nullopt;
        values[i] = negative ? -static_cast<int64_t>(v) : static_cast<int64_t>(v);
    }
    int64_t most = values[0];
    most = static_cast<int64_t>(static_cast<uint64_t>(most) << 16);
    most |= values[1];
    most = static_cast<int64_t>(static_cast<uint64_t>(most) << 16);
    most |= values[2];
    int64_t least = values[3];
    least = static_cast<int64_t>(static_cast<uint64_t>(least) << 48);
    least |= values[4];
    return UUID(most, least);
}

UUID UUID::nameUUIDFromBytes(std::string_view bytes)
{
    std::array<uint8_t, 16> md5Bytes = md5(bytes);
    md5Bytes[6] &= 0x0f;
    md5Bytes[6] |= 0x30;
    md5Bytes[8] &= 0x3f;
    md5Bytes[8] |= 0x80;
    uint64_t msb = 0;
    uint64_t lsb = 0;
    for (int i = 0; i < 8; ++i)
        msb = (msb << 8) | md5Bytes[static_cast<size_t>(i)];
    for (int i = 8; i < 16; ++i)
        lsb = (lsb << 8) | md5Bytes[static_cast<size_t>(i)];
    return UUID(static_cast<int64_t>(msb), static_cast<int64_t>(lsb));
}

UUID UUID::getRandomUuid(Random& rand)
{
    const int64_t i = (rand.nextLong() & -61441LL) | 16384LL;
    const int64_t j = (rand.nextLong() & 4611686018427387903LL) | INT64_MIN;
    return UUID(i, j);
}

} // namespace mc

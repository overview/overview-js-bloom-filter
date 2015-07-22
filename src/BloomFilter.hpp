#include "farmhash.h"

#include <arpa/inet.h>
#include <string>

class BloomFilter
{
public:
  explicit BloomFilter(size_t nBuckets, uint8_t nHashes)
  : nBuckets(nBuckets),
    nHashes(nHashes),
    buckets(new char[nBuckets / 8 + (nBuckets % 8 ? 1 : 0)])
  {
    std::fill(buckets, buckets + nBytes(), 0);
  }

  ~BloomFilter() { delete buckets; }

  inline void setBit(size_t i) { buckets[i / 8] |= 1 << (i % 8); }
  inline bool testBit(size_t i) const { return 0 != (buckets[i / 8] & (1 << (i % 8))); }

  void setBuckets(const char* s, size_t len) {
    if (len > nBytes()) len = nBytes();
    std::copy(s, s + len, buckets);
  }

  void add(const char* s, size_t len) {
    const uint64_t hash64 = util::Fingerprint64(s, len);
    const uint32_t h2 = static_cast<uint32_t>(hash64 & 0xffffffff);

    uint32_t h = static_cast<uint32_t>(hash64 >> 32) % nBuckets;
    setBit(h);

    for (uint8_t i = 1; i < nHashes; i++) {
      h = (h + h2) % nBuckets;
      setBit(h);
    }
  }

  bool test(const char* s, size_t len) {
    const uint64_t hash64 = util::Fingerprint64(s, len);
    const uint32_t h2 = static_cast<uint32_t>(hash64 & 0xffffffff);

    uint32_t h = static_cast<uint32_t>(hash64 >> 32) % nBuckets;
    if (!testBit(h)) return false;

    for (uint8_t i = 1; i < nHashes; i++) {
      h = (h + h2) % nBuckets;
      if (!testBit(h)) return false;
    }

    return true;
  }

  const std::string serialize() const {
    std::string ret;

    // Write nBuckets: four bytes
    const uint32_t i = htonl(nBuckets);
    ret.push_back(static_cast<char>((i >> 24) & 0xff));
    ret.push_back(static_cast<char>((i >> 16) & 0xff));
    ret.push_back(static_cast<char>((i >> 8) & 0xff));
    ret.push_back(static_cast<char>(i) & 0xff);

    // Write nHashes
    ret.push_back(static_cast<char>(nHashes));

    // Write the buckets as chars
    ret.append(buckets, buckets + nBytes());

    return ret;
  }

private:
  const size_t nBuckets;
  const uint8_t nHashes;
  char* buckets;

  size_t nBytes() const { return nBuckets / 8 + (nBuckets % 8 ? 1 : 0); }
};

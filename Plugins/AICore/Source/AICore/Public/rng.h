#pragma once
#include <cstdint>
// Random Number Generator
struct SplitMix64 {     // RNG 초기화 or 해시용(Zorbist 키 생성 등)
    uint64_t x;
    explicit SplitMix64(uint64_t seed = 0xC0FFEEULL) : x(seed) {}
    uint64_t next() {
        uint64_t z = (x += 0x9E3779B97F4A7C15ULL);      // 황금비와 관련된 홀수 더해주고
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;    // z 값을 여러번 xor하고 곱해서 비트 값을 섞어줌
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
};

struct XorShift64Star { // RNG는 아니고 암호학적으로 안전
    uint64_t x;
    explicit XorShift64Star(uint64_t seed = 88172645463393265ULL) : x(seed ? seed : 88172645463393265ULL) {}
    uint64_t next() {
        x ^= x >> 12; x ^= x << 25; x ^= x >> 27;       // xor, 시프트로 섞음 마지막으로 변종(*)을 곱함
        return x * 2685821657736338717ULL;
    }
};
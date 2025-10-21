#pragma once
#include <cstdint>
// Random Number Generator
struct SplitMix64 {     // RNG �ʱ�ȭ or �ؽÿ�(Zorbist Ű ���� ��)
    uint64_t x;
    explicit SplitMix64(uint64_t seed = 0xC0FFEEULL) : x(seed) {}
    uint64_t next() {
        uint64_t z = (x += 0x9E3779B97F4A7C15ULL);      // Ȳ�ݺ�� ���õ� Ȧ�� �����ְ�
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;    // z ���� ������ xor�ϰ� ���ؼ� ��Ʈ ���� ������
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
};

struct XorShift64Star { // RNG�� �ƴϰ� ��ȣ�������� ����
    uint64_t x;
    explicit XorShift64Star(uint64_t seed = 88172645463393265ULL) : x(seed ? seed : 88172645463393265ULL) {}
    uint64_t next() {
        x ^= x >> 12; x ^= x << 25; x ^= x >> 27;       // xor, ����Ʈ�� ���� ���������� ����(*)�� ����
        return x * 2685821657736338717ULL;
    }
};
#pragma once
#include <cstdint>
#include <vector>
#include <climits>
#include "action.h"

enum class ETTBound : uint8_t { Exact = 0, Lower = 1, Upper = 2 };

struct TTEntry {
    uint64_t  Key = 0;
    int16_t   Depth = INT16_MIN;       // INT16_MIN�̸� "�� ����"���� ����
    int32_t   Score = 0;
    ETTBound  Bound = ETTBound::Exact;
    Action    BestMove{};              // PV ����/�������� ���
    uint16_t  Age = 0;                 // ��ü ��å
};

struct TTBucket { TTEntry E[2]; };

class TTable {
    std::vector<TTBucket> Table;
    size_t Mask = 0;

public:
    void ResizeMB(size_t MB) {
        size_t bytes = MB * 1024ull * 1024ull;
        size_t buckets = bytes / sizeof(TTBucket);
        if (buckets < 1) buckets = 1;
        // power-of-two�� �ݿø�
        size_t p = 1; while (p < buckets) p <<= 1;
        buckets = p;

        Table.assign(buckets, {});
        Mask = buckets - 1;

        for (auto& b : Table) {
            b.E[0].Depth = INT16_MIN;
            b.E[1].Depth = INT16_MIN;
        }
    }

    inline bool IsReady() const { return !Table.empty(); }
    inline size_t Index(uint64_t key) const { return (size_t)key & Mask; }

    bool Probe(uint64_t key, TTEntry& out) const {
        if (Table.empty()) return false;
        const TTBucket& B = Table[Index(key)];
        for (int i = 0; i < 2; ++i) {
            if (B.E[i].Depth != INT16_MIN && B.E[i].Key == key) { out = B.E[i]; return true; }
        }
        return false;
    }

    void Store(uint64_t key, int16_t depth, int32_t score, ETTBound bound, const Action& best, uint16_t age) {
        if (Table.empty()) return;
        TTBucket& B = Table[Index(key)];

        // ��ü �ĺ� ����: �� ���� �켱 �� �� ���� ���� �� ���̰� ������ ��
        int repl = -1;
        if (B.E[0].Depth == INT16_MIN) repl = 0;
        else if (B.E[1].Depth == INT16_MIN) repl = 1;
        else {
            repl = (B.E[0].Depth < B.E[1].Depth) ? 0 : 1;
            if (B.E[0].Depth == B.E[1].Depth) {
                repl = (B.E[0].Age <= B.E[1].Age) ? 0 : 1;
            }
        }

        TTEntry& E = B.E[repl];
        E.Key = key; E.Depth = depth; E.Score = score; E.Bound = bound; E.BestMove = best; E.Age = age;
    }
};
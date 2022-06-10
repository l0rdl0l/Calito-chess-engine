#ifndef TTABLE_H
#define TTABLE_H

#include <cstdint>

#include "game.h"


class TTable {
    public:
        struct Entry {
            uint64_t hash;
            uint16_t flags;
            uint16_t min;
            uint16_t max;
            uint16_t move;
        };

        static Entry *lookup(uint64_t hash);

        static void insert(uint64_t hash, short eval, int nodeType, Game::Move move, int depth);

        static void setSizeInMiB(int sizeInMiB);

    private:
        static int sizeInMiB;

        static Entry *table;
};

#endif
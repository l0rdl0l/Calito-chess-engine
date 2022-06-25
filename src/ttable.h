#ifndef TTABLE_H
#define TTABLE_H

#include <cstdint>

#include "game.h"


class TTable {
    public:
        struct Entry {
            uint64_t hash;
            uint16_t depth;
            uint16_t entryType : 2;
            int16_t eval;
            uint16_t move;
        };

        static Entry *lookup(uint64_t hash);

        /**
         * inserts an entry into the table, if the replacement scheme allows it.
         * 
         * @param hash the position hash
         * @param eval the calculated evaluation
         * @param nodeType 0: eval is a lower bound; 1: eval is an exact score; 2: eval is an upper bound
         * @param move the best move. Only considered if nodeType is 0 or 1
         * @param depth the search depth with which the position has been searched
         */
        static void insert(uint64_t hash, short eval, int nodeType, Game::Move move, int depth);

        static void setSizeInMiB(int sizeInMiB);

        static void clear();

    private:
        static int sizeInMiB;

        static Entry *table;
};

#endif
#include <cstdint>
#include <iostream>

#include "ttable.h"

TTable::Entry* TTable::table = nullptr;
int TTable::sizeInMiB = 0;


void TTable::setSizeInMiB(int sizeInMiB) {

    if(TTable::sizeInMiB != sizeInMiB) {
        if(table != nullptr)
            free(table);

        TTable::sizeInMiB = sizeInMiB;
        table = (Entry*) malloc(((uint64_t) 1048576) * ((uint64_t) sizeInMiB));
    }
}

TTable::Entry * TTable::lookup(uint64_t hash) {
    uint64_t index = hash % ((1048576 / sizeof(Entry)) * sizeInMiB);

    if(table[index].hash == hash) {
        return &table[index];
    } else {
        return nullptr;
    }
}

void TTable::insert(uint64_t hash, short eval, int nodeType, Game::Move move, int depth) {
    uint64_t index = hash % ((1048576 / sizeof(Entry)) * sizeInMiB);

    if(table[index].depth < depth) {
        if(nodeType != 2)
            table[index].move = move.compress();

        table[index].hash = hash;    
        table[index].eval = eval;
        table[index].entryType = nodeType;
        table[index].depth = depth;
    }
}

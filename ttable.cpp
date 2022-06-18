#include <cstdint>
#include <iostream>

#include "ttable.h"

TTable::Entry* TTable::table = nullptr;
int TTable::sizeInMiB = 0;
int TTable::tableAge = 0;


void TTable::setSizeInMiB(int sizeInMiB) {

    if(TTable::sizeInMiB != sizeInMiB) {
        if(table != nullptr)
            free(table);

        TTable::sizeInMiB = sizeInMiB;
        table = (Entry*) calloc(((uint64_t) 1048576) * ((uint64_t) sizeInMiB), 1);
    }
    //ensures that empty slots are always used
    tableAge = 2;
}

void TTable::newPosition() {
    tableAge ++;
}

TTable::Entry * TTable::lookup(uint64_t hash) {
    uint64_t index = hash % ((1048576 / sizeof(Entry)) * sizeInMiB);

    if(table[index].hash == hash) {
        table[index].age = tableAge;
        return &table[index];
    } else {
        return nullptr;
    }
}

void TTable::insert(uint64_t hash, short eval, int nodeType, Game::Move move, int depth) {
    uint64_t index = hash % ((1048576 / sizeof(Entry)) * sizeInMiB);


    //replace if the entry hasen't been used during the last search or if replacing increases the depth.
    if((tableAge - table[index].age) > 1 || table[index].depth < depth) {
        if(nodeType != 2)
            table[index].move = move.compress();

        table[index].hash = hash;    
        table[index].eval = eval;
        table[index].entryType = nodeType;
        table[index].depth = depth;
        table[index].age = tableAge;
    }
}

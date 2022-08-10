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
        table = (Entry*) calloc(((uint64_t) 1048576) * ((uint64_t) sizeInMiB), 1);
    }
}

void TTable::clear() {
    free(table);
    table = (Entry*) calloc(((uint64_t) 1048576) * ((uint64_t) sizeInMiB), 1);
}

TTable::Entry * TTable::lookup(uint64_t hash) {
    uint64_t index = (hash % ((1048576 / sizeof(Entry)) * sizeInMiB)) & ~0x3;

    for(int i = 0; i < 4; i++) {
        if(table[index+i].hash == hash) {
            return &table[index+i];
        }
    }

    return nullptr;
}

void TTable::insert(uint64_t hash, short eval, int nodeType, Game::Move move, int depth) {
    
    /*
    If the given position already is in the table, update the values if the new depth is greater or equal than the old depth.
    Otherwise overwrite the slot with the smallest search depth, that doesn't contain an exact score.
    If all slots contain an exact score, choose the slot with the smallest depth and overwrite only if the new entry is an exact score.
    */
    
    uint64_t index = hash % ((1048576 / sizeof(Entry)) * sizeInMiB) & ~0x3;


    int minPriority = INT32_MAX;
    int minPrioritySlot = -1;
    for(int i = 0; i < 4; i++) {
        if(table[index+i].hash == hash) {
            if(table[index+i].depth <= depth || (nodeType == 1 && table[index+i].entryType != 1)) {
                minPrioritySlot = i;
                break;
            } else {
                return;
            }
        }
        int priority = table[index+i].depth + ((table[index+i].entryType == 1) << 30);
        if(minPriority > priority) {
            minPriority = priority;
            minPrioritySlot = i;
        }
    }

    if(!(table[index+minPrioritySlot].entryType == 1 && nodeType != 1)) {
        if(nodeType != 2)
            table[index+minPrioritySlot].move = move.compress();

        table[index+minPrioritySlot].hash = hash;
        table[index+minPrioritySlot].eval = eval;
        table[index+minPrioritySlot].entryType = nodeType;
        table[index+minPrioritySlot].depth = depth;
    }
}

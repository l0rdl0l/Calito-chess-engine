#include <cstdint>

#include "ttable.h"

TTable::Entry* TTable::table;
int TTable::sizeInMiB;

void TTable::setSizeInMiB(int sizeInMiB) {

    if(TTable::sizeInMiB != sizeInMiB) {
        if(table != nullptr)
            free(table);

        TTable::sizeInMiB = sizeInMiB;
        table = (Entry*) malloc(((uint64_t) 1048576) * ((uint64_t) sizeInMiB));
    }
}

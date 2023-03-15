#include "position.h"

#include <stdexcept>
#include <exception>

Move::Move() {
    //do nothing
}

Move::Move(char from, char to) {
    this->from = from;
    this->to = to;
    this->flag = 0;
    this->specialMove = 0;
}

Move::Move(char from, char to, char specialMove, char flag) {
    this->from = from;
    this->to = to;
    this->specialMove = specialMove;
    this->flag = flag;
}

Move::Move(unsigned short compressedMove) {
    this->from = compressedMove & 0x3f;
    this->to = (compressedMove & 0xfc0) >> 6;
    this->specialMove = (compressedMove & 0x3000) >> 12;
    this->flag = ((compressedMove & 0xc000) >> 14);

    if(this->specialMove == 3) {
        this->flag += KNIGHT;
    }
}



std::string Move::toString() {
    std::string result = intToField(from) + intToField(to);
    if(specialMove == 3) {
        if(flag == KNIGHT) {
            return result + "n";
        } else if(flag == BISHOP) {
            return result + "b";
        } else if(flag == ROOK) {
            return result + "r";
        } else {
            return result + "q";
        }
    }
    return result;
}

std::string Move::intToField(char field) {
    return std::string(1, field % 8 + 97).append(std::string(1, 8-field/8+0x30)); 
}

short Move::compress() {
    return from | (to << 6) | (specialMove << 12) | (specialMove == 3 ? ((flag - KNIGHT) << 14) : (flag << 14));
}

bool operator==(const Move& m1, const Move& m2) {
    return m1.flag == m2.flag && m1.from == m2.from && m1.to == m2.to && m1.specialMove == m2.specialMove;
}

bool operator!=(const Move& m1, const Move& m2) {
    return !(m1 == m2);
}

bool operator==(const Move& m1, const Move& m2);
bool operator!=(const Move& m1, const Move& m2);
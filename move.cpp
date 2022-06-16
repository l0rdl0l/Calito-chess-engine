#include "game.h"

#include <stdexcept>
#include <exception>

Game::Move::Move() {
    //do nothing
}

Game::Move::Move(char from, char to) {
    this->from = from;
    this->to = to;
    this->flags = 0;
}

Game::Move::Move(char from, char to, char promotion) {
    this->from = from;
    this->to = to;
    this->flags = promotion;
}

Game::Move::Move(short compressedMove) {
    this->from = compressedMove & 0x3f;
    this->to = (compressedMove & 0xfc0) >> 6;
    this->flags = (compressedMove & 0xf000) >> 12;
}

Game::Move::Move(std::string move) {
    if(move.length() != 4 && move.length() != 5) {
        throw std::invalid_argument("wrong move format");
    } 
    if(move[0] < 97 || move[0] > 104
            || move[1] < 0x31 || move[1] > 0x38
            || move[2] < 97 || move[2] > 104
            || move[3] < 0x31 || move[3] > 0x38) {

        throw std::invalid_argument("wrong move format");
    }
    this->from = fieldStringToInt(move.substr(0,2));
    this->to = fieldStringToInt(move.substr(2,2));
    if(move.length() == 5) {
        if(move[4] == 'n') {
            this->flags = 2;
        } else if(move[4] == 'b') {
            this->flags = 3;
        } else if(move[4] == 'r') {
            this->flags = 4;
        } else if(move[4] == 'q') {
            this->flags = 5;
        } else {
            throw std::invalid_argument("wrong move format");
        }
    } else {
        this->flags = 0;
    }
}

std::string Game::Move::toString() {
    std::string result = intToField(this->from) + intToField(this->to);
    if(this->flags == 2) {
        return result + "n";
    } else if(this->flags == 3) {
        return result + "b";
    } else if(this->flags == 4) {
        return result + "r";
    } else if(this->flags == 5) {
        return result + "q";
    }
    return result;
}

std::string Game::Move::intToField(char field) {
    return std::string(1, field % 8 + 97).append(std::string(1, 8-field/8+0x30)); 
}

char Game::Move::fieldStringToInt(std::string fieldCoords) {
    return fieldCoords[0] - 97 + (8-(fieldCoords[1]-0x30))*8;
}

short Game::Move::compress() {
    return this->from | (this->to << 6) | (this->flags << 12);
}

bool operator==(const Game::Move& m1, const Game::Move& m2) {
    return m1.flags == m2.flags && m1.from == m2.from && m1.to == m2.to;
}

bool operator!=(const Game::Move& m1, const Game::Move& m2) {
    return !(m1 == m2);
}
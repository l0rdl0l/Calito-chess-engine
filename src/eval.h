#ifndef EVAL_H
#define EVAL_H

#include "game.h"


//evaluation
class ScorePair {
    public:
        short mg;
        short eg;

        ScorePair() {
            this->mg = 0;
            this->eg = 0;
        }

        ScorePair(int score) {
            this->mg = score;
            this->eg = score;
        }

        ScorePair(int mg, int eg) {
            this->mg = mg;
            this->eg = eg;
        }

        ScorePair operator+ (ScorePair obj) {
            return ScorePair(this->mg + obj.mg, this->eg + obj.eg);
        }
        ScorePair operator- (ScorePair obj) {
            return ScorePair(this->mg - obj.mg, this->eg - obj.eg);
        }
        ScorePair operator* (ScorePair obj) {
            return ScorePair(this->mg * obj.mg, this->eg * obj.eg);
        }
        ScorePair operator/ (ScorePair obj) {
            return ScorePair(this->mg / obj.mg, this->eg / obj.eg);
        }
        
        ScorePair& operator+= (ScorePair obj) {
            this->mg += obj.mg;
            this->eg += obj.eg;
            return *this;
        }
        ScorePair& operator-= (ScorePair obj) {
            this->mg -= obj.mg;
            this->eg -= obj.eg;
            return *this;
        }
        ScorePair& operator*= (ScorePair obj) {
            this->mg *= obj.mg;
            this->eg *= obj.eg;
            return *this;
        }
        ScorePair& operator/= (ScorePair obj) {
            this->mg /= obj.mg;
            this->eg /= obj.eg;
            return *this;
        }
};

class Eval {
    public:
        struct Params {
            //material evaluation; indexed by piece constants defined in constants.h (PAWN, KNIGHT, BISHOP, ROOK, QUEEN)
            short pieceValues[6];

            //evaluation for every piece square combination
            ScorePair pieceSquare[6][64];
        };

        static Params *params;

        static short evaluate(Game::Position* pos);            

};


#endif
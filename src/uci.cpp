#include <iostream>
#include <string>
#include <regex>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <list>
#include <stdexcept>
#include <exception>

#include "position.h"
#include "engine.h"
#include "ttable.h"


#define AUTHOR "Lovis Hagemeyer"
#define ENGINE_ID "Calito beta 1.4"

#define MIN_TABLE_SIZE 1
#define MAX_TABLE_SIZE 4096
#define DEFAULT_TABLE_SIZE 256


uint64_t perft(int depth, Position& pos, bool printMoveResults, bool useCache) {

    uint64_t result = 0;
    Move moveBuffer[343];
    
    if(depth == 1) {
        return pos.getLegalMoves(moveBuffer);
    }
    uint64_t positionHash;
    uint64_t tableResult;
    TTable::Entry *ttentry = nullptr;
    if(useCache) {
        positionHash = pos.getPositionHash();
        ttentry = TTable::lookup(positionHash);
        if(ttentry != nullptr) {
            if(ttentry->depth == depth) {
                tableResult = ttentry->eval;
                tableResult |= ((uint64_t) ttentry->entryType) << 16;
                tableResult |= ((uint64_t) ttentry->move) << 32;
                return tableResult;
            }
        }
    }

    int numOfMoves = pos.getLegalMoves(moveBuffer);
    for(int i = 0; i < numOfMoves; i++) {
        pos.makeMove(moveBuffer[i]);
        uint64_t moveResult = perft(depth - 1, pos, false, useCache);
        if(printMoveResults) {
            std::cout << moveBuffer[i].toString() << ": " << moveResult << std::endl;
        }
        result += moveResult;
        pos.undo();
    }
    if(useCache) {  
        uint16_t eval = result & 0xffff;
        uint16_t entryType = (result & 0xffff0000) >> 16;
        Move move = Move((result & 0xffff00000000) >> 32);
        TTable::insert(positionHash, eval, entryType, move, depth);
    }
    return result;
}


struct engineOptions {

    int tableSize = DEFAULT_TABLE_SIZE;

} options;

std::mutex ioLock;


char fieldStringToInt(std::string fieldCoords) {
    return fieldCoords[0] - 97 + (8-(fieldCoords[1]-0x30))*8;
}

Move parseMove(std::string move, Position& pos) {
    Move result;

    if(move.length() != 4 && move.length() != 5) {
        throw std::invalid_argument("wrong move format");
    } 
    if(move[0] < 97 || move[0] > 104
            || move[1] < 0x31 || move[1] > 0x38
            || move[2] < 97 || move[2] > 104
            || move[3] < 0x31 || move[3] > 0x38) {

        throw std::invalid_argument("wrong move format");
    }
    result.from = fieldStringToInt(move.substr(0,2));
    result.to = fieldStringToInt(move.substr(2,2));

    if(move.length() == 5) {
        result.specialMove = 3;
        if(move[4] == 'n') {
            result.flag = KNIGHT;
        } else if(move[4] == 'b') {
            result.flag = BISHOP;
        } else if(move[4] == 'r') {
            result.flag = ROOK;
        } else if(move[4] == 'q') {
            result.flag = QUEEN;
        } else {
            throw std::invalid_argument("wrong move format");
        }
    } else if(pos.getPieceOnSquare(result.from) == KING && (result.from - result.to == 2 || result.from - result.to == -2)) {
        result.specialMove = 2;
        if(pos.whitesTurn) {
            if(result.from - result.to == 2) {
                result.flag = WHITE_CASTLE_QUEENSIDE;
            } else {
                result.flag = WHITE_CASTLE_KINGSIDE;
            }
        } else {
            if(result.from - result.to == 2) {
                result.flag = BLACK_CASTLE_QUEENSIDE;
            } else {
                result.flag = BLACK_CASTLE_KINGSIDE;
            }
        }
    } else if(pos.getPieceOnSquare(result.from) == PAWN && pos.getPieceOnSquare(result.to) == NO_PIECE && ((result.from-result.to) % 8) != 0) {
        result.specialMove = 1;
    }
    return result;
}

int main(int argc, char **argv) {

    Position pos;
    std::string input;

    if(argc >= 5) {
        if(std::strcmp(argv[1], "perft") == 0) {

            int depth = std::stoi(argv[2]);
            bool useCache = std::stoi(argv[3]);
            std::string fen = "";
            for(int i = 4; i < argc; i++) {
                fen += argv[i];
                fen += " ";
            }
            pos = Position(fen);
            TTable::setSizeInMiB(1024);
            std::cout << perft(depth, pos, true, useCache) << std::endl;
            return 0;
        }    
    }


    do {
        std::getline(std::cin, input);
    } while(input != "uci");

    std::cout << "id name " << ENGINE_ID << std::endl;
    std::cout << "id author " << AUTHOR << std::endl;
    
    //possible options
    std::cout << "option name Hash type spin default " << DEFAULT_TABLE_SIZE << " min " << MIN_TABLE_SIZE << " max " << MAX_TABLE_SIZE << std::endl;
    std::cout << "option name Ponder type check default true" << std::endl;

    std::cout << "uciok" << std::endl;


    while(true) {

        std::getline(std::cin, input);
        
        std::string temp = input;
        std::list<std::string> args;
        
        int index;
        while((index = temp.find_first_of(' ')) != std::string::npos) {
            args.push_back(temp.substr(0, index));
            temp.erase(0, index+1);
        }
        args.push_back(temp);

        std::string command = args.front();
        args.erase(args.begin());

        //don't interrupt command processing
        ioLock.lock();

        if(command == "quit") {
            ioLock.unlock();
            Engine::stopCalculation();
            exit(EXIT_SUCCESS);
        }

        if(command == "setoption") {
            if(std::regex_match(input, std::regex("setoption name (h|H)(a|A)(s|S)(h|H) value [0-9]+"))) {
                int tableSize = std::stoi(input.substr(26, std::string::npos));
                if(tableSize <= MAX_TABLE_SIZE && tableSize >= MIN_TABLE_SIZE) {
                    options.tableSize = tableSize;
                }
            }

            //won't check for ponder option, but instead just start pondering when told to so by GUI
        }

        if(command == "isready") {
            TTable::setSizeInMiB(options.tableSize);
            std::cout << "readyok" << std::endl;
        } 

        if(command == "ucinewgame") {
            //do nothing, a table filled with mostly useless data is better than an empty table
        }

        if(command == "debug") {
            //debug mode not supported
        }

        if(command == "register") {
            //no registration needed
        }

        if(command == "position") {
            if(args.front() == "fen") {
                std::string fen = "";
                args.pop_front();
                while (args.front() != "moves") {
                    fen += args.front();
                    fen += " ";
                    args.pop_front();

                    if(args.size() == 0)
                        break;
                }
                fen.pop_back();
                pos = Position(fen);
            } else { //use startposition
                pos = Position();
                args.pop_front();
            }
            
            if(args.size() != 0) {
                if(args.front() == "moves") {
                    args.pop_front();
                    while(args.size() > 0) {
                        Move parsedMove;
                        bool success = true;
                        try {
                            parsedMove = parseMove(args.front(), pos);
                        } catch (std::invalid_argument& e) {
                            std::cerr << "invalid move format" << std::endl;
                            success = false;
                        }
                        if(success) {
                            if(pos.moveLegal(parsedMove)) {
                                pos.makeMove(parseMove(args.front(), pos));
                            } else {
                                std::cerr << "illegal move detected: " << args.front() << std::endl;
                            }
                        }
                        args.pop_front();
                    }
                }
            }
        }

        if(command == "go") {
            
            Engine::Options goOptions;
            bool parsingSearchMoves = false;

            while(!args.empty()) {
                std::string token = args.front();
                args.pop_front();
                
                bool argIsMove = false;

                if(token == "ponder")        { goOptions.ponder = true; parsingSearchMoves = false; } 
                else if(token == "infinite") { goOptions.searchInfinitely = true; parsingSearchMoves = false; }
                else if(token == "searchmoves") {parsingSearchMoves = true;}
                else if(args.size() > 0) {

                    try {
                        if     (token == "wtime")     { goOptions.wtime = std::stoll(args.front());     args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "btime")     { goOptions.btime = std::stoll(args.front());     args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "winc")      { goOptions.winc = std::stoll(args.front());      args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "binc")      { goOptions.binc = std::stoll(args.front());      args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "movestogo") { goOptions.movesToGo = std::stoll(args.front()); args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "depth")     { goOptions.maxDepth = std::stoll(args.front());  args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "nodes")     { goOptions.maxNodes = std::stoll(args.front());  args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "movetime")  { goOptions.moveTime = std::stoll(args.front());  args.pop_front(); parsingSearchMoves = false;}
                        else if(parsingSearchMoves) {
                            argIsMove = true;
                        }
                    } catch (std::exception e) { // std::stoll threw an exception
                        std::cerr << "invalid argument: " + token + " " + args.front() << std::endl;
                        parsingSearchMoves = false;
                    }
                    
                } else if(parsingSearchMoves) {
                    argIsMove = true;
                }
                if(argIsMove) {
                    bool success = true;
                    Move parsedMove;
                    try {
                        parsedMove = parseMove(token, pos);
                    } catch (std::exception e) {
                        std::cerr << "illegal move format: " << token << std::endl;
                        success = false;
                    } 
                    if(success) {
                        if(pos.moveLegal(parsedMove)) {
                            goOptions.searchMoves.push_back(parsedMove);
                        } else {
                            std::cerr << "illegal move detected: " << token << std::endl;
                        }
                    }
                }
            }
            TTable::setSizeInMiB(options.tableSize);
            ioLock.unlock();
            Engine::startAnalyzing(pos, goOptions);
            ioLock.lock();
        }

        if(command == "ponderhit") {
            Engine::ponderHit();
        }

        if(command == "stop") {
            ioLock.unlock(); //allow the thread to print the result
            Engine::stopCalculation();
            ioLock.lock();
        }

        ioLock.unlock();
    }
}

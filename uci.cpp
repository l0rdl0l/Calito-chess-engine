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

#include "game.h"
#include "engine.h"


#define AUTHOR "Lovis Hagemeyer"
#define ENGINE_ID "CalitoEngine 0.6"

#define MIN_TABLE_SIZE 1
#define MAX_TABLE_SIZE 4096
#define DEFAULT_TABLE_SIZE 1024


uint64_t perft(int depth, Game& game, bool printMoveResults) {
    uint64_t result = 0;
    Game::Move moveBuffer[343];
    
    if(depth == 1) {
        bool check;
        return game.getNumOfMoves(check);
    }
    int numOfMoves = game.getLegalMoves(moveBuffer);
    for(int i = 0; i < numOfMoves; i++) {
        game.playMove(moveBuffer[i]);
        uint64_t moveResult = perft(depth - 1, game, false);
        if(printMoveResults) {
            std::cout << moveBuffer[i].toString() << ": " << moveResult << std::endl;
        }
        result += moveResult;
        game.undo();
    }
    return result;
}


struct engineOptions {

    int tableSize = DEFAULT_TABLE_SIZE;

} options;

std::mutex ioLock;


int main(int argc, char **argv) {

    Game game;
    std::string input;

    if(argc >= 4) {
        if(std::strcmp(argv[1], "perft") == 0) {

            int depth = std::stoi(argv[2]);
            std::string fen = "";
            for(int i = 3; i < argc; i++) {
                fen += argv[i];
                fen += " ";
            }
            game = Game(fen);
            std::cout << perft(depth, game, true) << std::endl;
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
                game = Game(fen);
            } else { //use startposition
                game = Game();
                args.pop_front();
            }
            
            if(args.size() != 0) {
                if(args.front() == "moves") {
                    args.pop_front();
                    while(args.size() > 0) {
                        Game::Move parsedMove;
                        bool success = true;
                        try {
                            parsedMove = Game::Move(args.front());
                        } catch (std::invalid_argument& e) {
                            std::cerr << "invalid move format" << std::endl;
                            success = false;
                        }
                        if(success) {
                            if(game.moveLegal(parsedMove)) {
                                game.playMove(Game::Move(args.front()));
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
            
            Engine::Options options;
            bool parsingSearchMoves = false;

            while(!args.empty()) {
                std::string token = args.front();
                args.pop_front();
                
                bool argIsMove = false;

                if(token == "ponder")        { options.ponder = true; parsingSearchMoves = false; } 
                else if(token == "infinite") { options.searchInfinitely = true; parsingSearchMoves = false; }
                else if(token == "searchmoves") {parsingSearchMoves = true;}
                else if(args.size() > 0) {

                    try {
                        if     (token == "wtime")     { options.wtime = std::stoll(args.front());     args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "btime")     { options.btime = std::stoll(args.front());     args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "winc")      { options.winc = std::stoll(args.front());      args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "binc")      { options.binc = std::stoll(args.front());      args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "movestogo") { options.movesToGo = std::stoll(args.front()); args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "depth")     { options.maxDepth = std::stoll(args.front());  args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "nodes")     { options.maxNodes = std::stoll(args.front());  args.pop_front(); parsingSearchMoves = false;}
                        else if(token == "movetime")  { options.moveTime = std::stoll(args.front());  args.pop_front(); parsingSearchMoves = false;}
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
                    Game::Move parsedMove;
                    try {
                        parsedMove = Game::Move(token);
                    } catch (std::exception e) {
                        std::cerr << "illegal move format: " << token << std::endl;
                        success = false;
                    } 
                    if(success) {
                        if(game.moveLegal(parsedMove)) {
                            options.searchMoves.push_back(parsedMove);
                        } else {
                            std::cerr << "illegal move detected: " << token << std::endl;
                        }
                    }
                }
            }
            ioLock.unlock();
            Engine::startAnalyzing(game, options);
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

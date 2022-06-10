#include <iostream>
#include <atomic>
#include <chrono>
#include <cassert>
#include <cctype>
#include <condition_variable>
#include <vector>


#include "engine.h"
#include "game.h"
#include "mutexes.h"

std::atomic<bool> Engine::stop;
std::atomic<bool> Engine::ponder;
Engine::Options Engine::options;
bool Engine::searchAborted;
uint64_t Engine::nodesSearched;
std::atomic<uint64_t> Engine::maxTimeInms;
Game Engine::game;
std::chrono::time_point<std::chrono::steady_clock> Engine::executionStartTime;
std::thread Engine::workerThread;
Game::Move Engine::pv[Engine::pvLength];
Game::Move (*Engine::killerMoves)[2];

std::thread Engine::timeController;
std::mutex Engine::timeThreadMutex;
std::condition_variable Engine::timingAbortCondition;
bool Engine::timingAbortFlag;
bool Engine::playAsWhite;



int64_t Engine::getExecutionTimeInms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - Engine::executionStartTime).count();
}

void Engine::setTimer() {
    if(options.moveTime != -1) {
        Engine::maxTimeInms = options.moveTime;
    } else {
        //if no time control checkpoint assume 50 moves to go
        int movesToGo = Engine::options.movesToGo == -1 ? 50 : Engine::options.movesToGo;


        int64_t timeOnClock = Engine::playAsWhite ? options.wtime : options.btime;
        int64_t increment = Engine::playAsWhite ? options.winc : options.binc;

        int64_t maxTime = (uint64_t) (((double) timeOnClock) / ((double) movesToGo) * 1.5 + ((double) increment));
        if(maxTime >= timeOnClock)
            maxTime = timeOnClock;

        maxTime -= connectionLagBuffer; //possible network lag

        if(maxTime <= connectionLagBuffer) {
            maxTime = connectionLagBuffer;
        }

        Engine::maxTimeInms = maxTime;
    }
    
    Engine::timingAbortFlag = false;

    Engine::timeController = std::thread([]() {
        std::unique_lock<std::mutex> lock(Engine::timeThreadMutex);
        Engine::timingAbortCondition.wait_for(lock, std::chrono::milliseconds(Engine::maxTimeInms), []{return Engine::timingAbortFlag;});
        Engine::stop = true;
    });


}

void Engine::clearTimer() {
    if(timeController.joinable()) {
        
        timeThreadMutex.lock();
        Engine::timingAbortFlag = true;
        timingAbortCondition.notify_one();
        timeThreadMutex.unlock();

        timeController.join();
    }
}

void Engine::ponderHit() {
    if(Engine::ponder) {
        setTimer();
        Engine::ponder = false;
    }
}

void Engine::stopCalculation() {
    clearTimer();
    if(workerThread.joinable()) {
        Engine::stop = true;
        workerThread.join();
    }
}

void Engine::startAnalyzing(Game& game, Engine::Options& options) {
    //stop possible ongoing search
    Engine::stopCalculation();
    
    //initialize values
    Engine::stop = false;
    Engine::ponder = options.ponder;
    Engine::executionStartTime = std::chrono::steady_clock::now();
    Engine::playAsWhite = game.whiteToMove();
    Engine::game = game;
    Engine::options = options;

    //if necessary, start timer
    if(!options.ponder && !options.searchInfinitely && (options.moveTime != -1 || (Engine::playAsWhite ? options.wtime : options.btime) != -1)) {
        setTimer();
    }

    //start worker thread
    Engine::workerThread = std::thread(&analyze);
}

bool Engine::isMate(short evaluation) {
    return (evaluation > ((32767-maxMateDistance) + 1)) || (evaluation < ((-32767 + maxMateDistance) - 1));
}


void Engine::analyze() {

    int searchDepth = 1;

    uint64_t lastDepthSearchTime = 0;

    Game::Move lastPV[Engine::pvLength];

    while(true) {

        std::chrono::time_point<std::chrono::steady_clock> depthStartTime = std::chrono::steady_clock::now();

        searchAborted = false;

        Engine::nodesSearched = 0;

        short currentEvaluation = searchWrapper(searchDepth);

        if(searchAborted) {
            break;
        }
        
        //save the outputed pv as it could be invalidated by an aborted search in the next iteration
        for(int i = 0; i < Engine::pvLength; i++)
            lastPV[i] = Engine::pv[i];

        uint64_t currentDepthSearchTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - depthStartTime).count();

        ioLock.lock();
        std::cout   << "info depth " << searchDepth;

        if(isMate(currentEvaluation)) {
            std::cout << " score mate " << getMateDistanceFromEvaluation(currentEvaluation); 
        } else {
            std::cout << " score cp " << currentEvaluation;
        }
                    
        std::cout   << " nodes " << Engine::nodesSearched 
                    << " time " << currentDepthSearchTime;

        if(currentDepthSearchTime > 10) //only send nps when the time precision is sufficient
            std::cout << " nps " << (int) (((double) nodesSearched) / ((double) currentDepthSearchTime) * 1000);
        

        std::cout << " pv";
        for(int i = 0; i < (searchDepth < pvLength ? searchDepth : pvLength); i++) {
            std::cout << " " << lastPV[i].toString();
        }
        std::cout << std::endl;
        ioLock.unlock();

        if(isMate(currentEvaluation)) {
            break;
        }

        if(options.maxDepth != -1 && searchDepth >= options.maxDepth) {
            break;
        }

        if(options.moveTime == -1 && options.movesToGo != 1 && !options.searchInfinitely && (Engine::playAsWhite ? options.wtime : options.btime) != -1 && !Engine::ponder && lastDepthSearchTime >= 100) {
            if(((double) currentDepthSearchTime) / ((double) lastDepthSearchTime) * 1.5 * ((double) currentDepthSearchTime) + getExecutionTimeInms() >= Engine::maxTimeInms) {
                //as it is quite possible that we won't finish the search of the next depth we abort, to not waste time
                break;
            }
        }
        lastDepthSearchTime = currentDepthSearchTime;
 
        searchDepth++;
    }
    
    ioLock.lock();
    //TODO prevent undefined output on interruption during the first depth;
    std::cout << "bestmove " << lastPV[0].toString();
    if(searchDepth > 1) {
        std::cout << " ponder " << lastPV[1].toString();
    }
    std::cout << std::endl;
    ioLock.unlock();

    while(!stop && (ponder || options.searchInfinitely)) {} //keep waiting if we are pondering or in infinite search and haven't recieved a stop signal

    return;
}

short Engine::getMateEvaluation(int depth) {
    return -32767 + ((depth + 1) / 2);
}

short Engine::getMateDistanceFromEvaluation(int eval) {
    if(eval < 0) {
        return -(eval + 32767);
    } else {
        return -(eval - 32767);
    }
}

short Engine::searchWrapper(int depth) {
    //initialize killer move array
    killerMoves = (Game::Move (*)[2]) malloc(sizeof(Game::Move) * (depth+1) * 2);
    for(int i = 0; i < depth+1; i++) {
        killerMoves[i][0] = Game::Move(0, 0);
        killerMoves[i][1] = Game::Move(0, 0);
    }

    Game::Move *moveBuffer = (Game::Move *) malloc(sizeof(Game::Move) * 343 * (depth + 1));

    short result = search(-32767, 32767, depth, 0, true, moveBuffer, false);

    free(killerMoves);
    free(moveBuffer);

    return result;
}

/**
 * fail soft
 * if exact score <= alpha: exact score <= return value <= alpha
 * if alpha < exact score < beta: return vale = exact score
 * if beta <= exact score: beta <= return value <= exact score
*/

short Engine::search(short alpha, short beta, int depth, int distanceToRoot, bool pvNode, Game::Move *moveBuffer, bool searchRecaptures) {

    //debug
    //game.printInternalRepresentation();

    nodesSearched ++;
    if(options.maxNodes && nodesSearched > options.maxNodes) { 
        searchAborted = true;
        return 0;
    }

    if(Engine::stop) {
        searchAborted = true;
        return 0;
    }

    int numOfMoves;
    bool kingInCheck;
    if(depth >= 0) {
        numOfMoves = game.getLegalMoves(kingInCheck, moveBuffer);
    } else {
        numOfMoves = game.getNumOfMoves(kingInCheck);
    }


    if(numOfMoves == 0) {
        if(kingInCheck) {
            return getMateEvaluation(distanceToRoot);
        } else {
            return 0;
        }
    }

    if(game.isPositionDraw(distanceToRoot)) {
        return 0;
    }

    if(distanceToRoot == 0) {
        //check if options.searchmoves contains moves to be searched exclusively. If not search all legal moves (which are computed above)
        if(options.searchMoves.size() != 0) {
            for(int i = 0; i < options.searchMoves.size(); i++) {
                moveBuffer[i] = options.searchMoves[i];
            } 
            numOfMoves = options.searchMoves.size();
        }

        //TODO: remove after transposition table implementation
        if(depth > 1) {
            for(int i = 0; i < numOfMoves; i++) {
                if(moveBuffer[i] == Engine::pv[0]) {
                    Game::Move tmp = moveBuffer[0];
                    moveBuffer[0] = moveBuffer[i];
                    moveBuffer[i] = tmp;
                }
            }
        }
    }

    if((depth == 0 && !searchRecaptures && !kingInCheck) || depth < 0) {
        return game.getLeafEvaluation(kingInCheck, numOfMoves);
    }

    Game::Move bestMove;
    bool alphaRaised = false; 

    if(!searchRecaptures) {
        //if the first killer move is legal, search that move first, if the second killer move is also legal search it afterwards, if only the second move is
        //legal search it first
        int numOfSortedMoves = 0;
        for(int i = 0; i < 2; i++) {
            for(int j = 0; j < numOfMoves; j++) {
                if(moveBuffer[j] == killerMoves[distanceToRoot][i]) {
                    Game::Move tmp = moveBuffer[numOfSortedMoves];
                    moveBuffer[numOfSortedMoves] = killerMoves[distanceToRoot][i];
                    moveBuffer[j] = tmp;
                    numOfSortedMoves++;
                    break;
                }
            }
        }
    }

    /*short thisNodeEval;
    if(depth == 1) {
        thisNodeEval = game.getLeafEvaluation(kingInCheck, numOfMoves);
    }*/

    bool printCurrentMoves = false;
    if(distanceToRoot == 0) {
        //only print currently searched move if we are already searching for one second, to prevent to much traffic
        if(getExecutionTimeInms() >= 1000) {
            printCurrentMoves = true;
        }
    }

    for(int i = 0; i < numOfMoves; i++) {

        if(printCurrentMoves) {
            ioLock.lock();
            std::cout << "info currmove " << moveBuffer[i].toString() << " currmovenumber " << i+1 << std::endl;
            ioLock.unlock();
        }
        
        short currentEval;
        bool captureMove = game.isCapture(moveBuffer[i]);
        

        //don't search non capture moves if searchRecaptures is true
        if(searchRecaptures && !captureMove) {
            continue;
        }
        
        //futility pruning
        /*if(depth == 1 && !captureMove && alpha >= thisNodeEval + 50) {
            continue;
        }*/

        //debugging
        //std::cout << "killer moves: " << killerMoves[distanceToRoot][0].toString() << " " << killerMoves[distanceToRoot][1].toString() << std::endl;
        //std::cout << "move: " << moveBuffer[i].toString() << std::endl;

        game.playMove(moveBuffer[i]);

        //capture moves in the last search depth trigger a recapture extension, searching capture move answers the opponent could give
        bool childRecaptures = ((depth == 1) && captureMove);
        
        if(pvNode && depth >= 1) {
            if(i == 0) {
                currentEval = -search(-beta, -alpha, depth - 1, distanceToRoot + 1, true, moveBuffer + numOfMoves, childRecaptures);
            } else {
                currentEval = -search(-alpha-1, -alpha, depth -1, distanceToRoot + 1, false, moveBuffer + numOfMoves, childRecaptures);
                if(currentEval > alpha) {
                    //research
                    currentEval = -search(-beta, -alpha, depth - 1, distanceToRoot+1, true, moveBuffer + numOfMoves, childRecaptures);
                }
            }
        } else {
            currentEval = -search(-beta, -alpha, depth - 1, distanceToRoot + 1, false, moveBuffer + numOfMoves, childRecaptures);
        }

        if(searchAborted) {
            return 0;
        }
                
        if(alpha < currentEval) {
            alpha = currentEval;
            alphaRaised = true;
            bestMove = moveBuffer[i];
            
            if(alpha >= beta) {
                game.undo();

                if(!searchRecaptures) {
                    //put the current cut-off move into the first killer move slot, if it is not already there.
                    //the move currently in the first slot is shifted to the second slot.
                    if(moveBuffer[i] != killerMoves[distanceToRoot][0]) {
                        killerMoves[distanceToRoot][1] = killerMoves[distanceToRoot][0];
                        killerMoves[distanceToRoot][0] = moveBuffer[i];
                    }
                }
                return beta;
            }
        }

        game.undo();
    }

    if(distanceToRoot < Engine::pvLength && pvNode && !searchRecaptures) {
        Engine::pv[distanceToRoot] = bestMove;
    }

    if(searchRecaptures && !alphaRaised) { // there was no recapture to calculate
        short eval = game.getLeafEvaluation(kingInCheck, numOfMoves);
        if(eval > alpha) 
            alpha = eval;
    }

    return alpha;
}
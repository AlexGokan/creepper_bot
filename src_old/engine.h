
#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include <chrono>
#include "chess.hpp"

#include "search.h"
#include "eval.h"

/*

BENCHMARKING AND OPTIMZATION RESULTS

Original version
searchbenchmark m 5
==============
Best Move:
b2b3
Time: 3061.31
==============

const move&
searchbenchmark m 5
==============
Best Move:
b2b3
Time: 2973.12
==============

after removing sidetomove recalculation
searchbenchmark m 5 
==============
Best Move:
b2b3
Time: 2984.8
==============

after removing the isGameOver from search function
==============
Best Move:
b2b3
Time: 2418.89
==============

after removing isGameOver from eval function
==============
Best Move:
b2b3
Time: 535.372
==============

after making eval take a board  reference
searchbenchmark m 5
==============
Best Move:
b2b3
Time: 379.881
==============

reducing search range of bb interator with pop
==============
Best Move:
b2b3
Time: 375.944
==============

after removing if/else chain for lookup
searchbenchmark m 5
==============
Best Move:
b2b3
Time: 347.092
==============

sorting moves with captures first
searchbenchmark m 5
==============
Best Move:
b2b3
Time: 231.219
==============

enabling -O3 compiler flags
searchbenchmark m 5
==============
Best Move:
b2b3
Time: 12.3859
==============


*/



class ChessEngine{
    public:
    
    chess::Board board;
    long long random_seed;
    std::mt19937_64 rng;

    int nodes_evaluated;
    std::ofstream commands_log;

    ChessEngine(){
        
        this->board = chess::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        this->random_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        
        
        this->rng = std::mt19937_64(random_seed);

        this->nodes_evaluated = 0;
        this->commands_log = std::ofstream("C:/src/creepp_bot/logs/commands.txt");

        
    }

    ~ChessEngine(){
        this->commands_log << "=============================================================" << std::endl;
        this->commands_log.close();
        
    }

    void cmd_uci(){
        std::cout << "id name nullptrbot" << std::endl;
        std::cout << "id author alex" << std::endl;
        std::cout << "uciok" << std::endl;
    }

    void cmd_ucinewgame(){

    }

    void cmd_isready(){
        std::cout << "readyok" << std::endl;
    }

    void cmd_position(std::vector<std::string> tokens){
        size_t next_idx = 1;
        if(tokens[1] == "startpos"){
            this->board = chess::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            next_idx = 2;
        }else if (tokens[1] == "fen"){
            std::string fen_str = "";
            for(int i=2; i<8; ++i){
                fen_str += tokens[i];
                fen_str += " ";
            }
            this->board = chess::Board(fen_str);
            next_idx = 8;
        }else{
            std::cout << "bad position command" << std::endl;
            return;
        }

        if(tokens[next_idx] == "moves"){
            next_idx++;
            while(next_idx < tokens.size()){
                std::string s = tokens[next_idx];
                auto m = chess::uci::uciToMove(this->board,s);
                this->board.makeMove(m);
                next_idx++;
            }
        }

    }

    void cmd_go(std::vector<std::string> tokens){
        //example go string
        //go wtime 118000 btime 120000 winc 0 binc 0


        bool preset_think_time = false;
        int time_limit_ms = 0;

        int wtime = 100000;
        int btime = 100000;
        int binc = 0;
        int winc = 0;
        

        for(size_t i=1; i<tokens.size(); i++){
            if(tokens[i] == "winc"){
                winc = std::stoi(tokens[i+1]);
            }else if(tokens[i] == "binc"){
                binc = std::stoi(tokens[i+1]);
            }else if(tokens[i] == "wtime"){
                wtime = std::stoi(tokens[i+1]);
            }else if(tokens[i] == "btime"){
                btime = std::stoi(tokens[i+1]);
            }else if(tokens[i] == "movetime"){
                time_limit_ms = std::stoi(tokens[i+1]);
                preset_think_time = true;
            }
            //do nothing if its not one of the above (will just do nothing on the numeric tokens)
        }


        if(!preset_think_time){
            float w_think_time = static_cast<float>(winc) + static_cast<float>(wtime)/25.0;
            float b_think_time = static_cast<float>(binc) + static_cast<float>(btime)/25.0;

            if(this->board.sideToMove() == chess::Color::WHITE){
                time_limit_ms = w_think_time;
            }else{
                time_limit_ms = b_think_time;
            }
        }


        chess::Move best_move = negamax_caller(*this,this->board,7,false,time_limit_ms);
        std::string s = chess::uci::moveToUci(best_move);

        std::cout << "bestmove " << s << std::endl;
    }

    void cmd_searchbenchmark(std::vector<std::string> tokens){
        
        if(tokens.size() == 1){

        }else if(tokens.size() == 3){
            if(tokens[1]=="m"){
                this->nodes_evaluated = 0;
                int depth = std::stoi(tokens[2]);

                auto start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

                chess::Move best_move = negamax_caller(*this,this->board,depth,false,1000*1000);//give it a real high time limit

                auto end_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();

                std::string s = chess::uci::moveToUci(best_move);


                std::cout << "==============" << std::endl;
                std::cout << "Best Move: " << std::endl;
                std::cout << s << std::endl;
                std::cout << "Time: " << (end_time - start_time) / 1000000.0 << std::endl;
                std::cout << "Nodes Eval'd: " << this->nodes_evaluated << std::endl;
                std::cout << "==============" << std::endl;

            }
        }
        

        
    }

    void cmd_gstatus(){
        //print current board
        std::cout << this->board.getFen() << std::endl;

        //print hashtable

        //print random seed
        std::cout << "Random seed: " << this->random_seed << std::endl;

    }

    void cmd_eval(){
        float evaluation = evaluate_simple(this->board);
        std::cout << "--------------------------" << std::endl;
        std::cout << "EVAL: " << evaluation << std::endl;
        std::cout << "--------------------------" << std::endl;
    }

    void cmd_clearhash(){

    }

    void cmd_hashstatus(){

    }

    void cmd_hashme(){

    }

    void cmd_setbookstatus(){

    }

    void cmd_playout(){
        playout_wrapper(*this);
    }

    void cmd_setseed(std::vector<std::string> tokens){
        if(tokens.size() > 2){
            std::cout << "Too many tokens to set seed" << std::endl;
            return;
        }

        try{
            long long seed = std::stoll(tokens[1]);
            this->random_seed = seed;
            this->rng = std::mt19937_64(this->random_seed);
        }catch(...){
            std::cout << "could not set random seed" << std::endl;
            return;
        }
    }


    void run(){
        std::string line;
        

        while (true) {
            if (!std::getline(std::cin, line)) {
                break;  // Exit if EOF or input error
            }

            this->commands_log << line << std::endl;
            
            // Split the line by spaces
            std::vector<std::string> input_sep;
            std::stringstream ss(line);
            std::string word;
            while (ss >> word) {
                input_sep.push_back(word);
            }
            
            if(input_sep.size() > 0){
                if(input_sep[0] == "quit"){
                    return;
                }else if(input_sep[0] == "uci"){
                    cmd_uci();
                }else if(input_sep[0] == "ucinewgame"){
                    cmd_ucinewgame();
                }else if(input_sep[0] == "isready"){
                    cmd_isready();
                }else if(input_sep[0] == "position"){
                    cmd_position(input_sep);
                }else if(input_sep[0] == "go"){
                    cmd_go(input_sep);
                }else if(input_sep[0] == "searchbenchmark"){
                    cmd_searchbenchmark(input_sep);
                }else if(input_sep[0] == "gstatus"){
                    cmd_gstatus();
                }else if(input_sep[0] == "eval"){
                    cmd_eval();
                }else if(input_sep[0] == "clearhash"){
                    cmd_clearhash();
                }else if(input_sep[0] == "hashstatus"){
                    cmd_hashstatus();
                }else if(input_sep[0] == "hashme"){
                    cmd_hashme();
                }else if(input_sep[0] == "setbookstatus"){
                    cmd_setbookstatus();
                }else if(input_sep[0] == "playout"){
                    cmd_playout();
                }else if(input_sep[0] == "setseed"){
                    cmd_setseed(input_sep);
                }
                                
                else{
                    std::cout << "unrecognized command" << std::endl;
                    std::cout << input_sep[0] << std::endl;
                }
            }

        }
    }

};


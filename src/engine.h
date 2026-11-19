
/*
An example of how to run a tournament 
C:\Users\Alex\Desktop\fastchess-windows-x86-64>fastchess.exe -engine cmd=C:/src/creepp_bot/main_without_table.exe name=dumb -engine cmd=C:/src/creepp_bot/main.exe name=p_smart -each tc=40+0.2 -rounds 1 -repeat -log file=C:/Users/Alex/Desktop/tourney_log.txt engine=true level=info append=false -pgnout file=C:/Users/Alex/Desktop/tourney_pgn.pgn

*/


#pragma once

#include <iostream>
#include <sstream>
#include <fstream>
#include <random>
#include <cstring>
#include <chrono>
#include <stdlib.h>
#include <cmath>
#include "chess.hpp"

#include "search.h"


struct BookEntry{
    uint64_t key;
    chess::Square move_source;
    chess::Square move_dest;
    int16_t weight;
    float normalized_weight;
    float cum_weight;
};


class ChessEngine{
    public:
    
    chess::Board board;
    long long random_seed;
    std::mt19937_64 rng;

    uint64_t nodes_evaluated;
    std::ofstream commands_log;

    bool debug;
    int think_time_ms;

    std::unordered_map<uint64_t, std::vector<BookEntry>> opening_book;

    bool in_book;

    bool stop_search;

    ChessEngine(){
        
        this->board = chess::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        this->random_seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        
        
        this->rng = std::mt19937_64(random_seed);

        this->nodes_evaluated = 0;
        this->commands_log = std::ofstream("C:/src/creepp_bot/logs/commands.txt");

        this->debug = false;
        this->think_time_ms = -1;

        this->opening_book = load_book("C:/src/nullptr_bot/books/Titans.bin");

        this->in_book = true;

        this->stop_search = false;

        
    }

    ~ChessEngine(){
        this->commands_log << "=============================================================" << std::endl;
        this->commands_log.close();
        
    }



    std::unordered_map<uint64_t, std::vector<BookEntry>> load_book(std::string filename){
        std::unordered_map<uint64_t, std::vector<BookEntry>> opening_book = {};
        
        int64_t startpos_hash = 0x463b96181691fc9c;//for checking zobrist implementation
        
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if(!file.is_open()){
            std::cout << "info string: ----->  Could not open opening book" << std::endl;
            return opening_book;
        }

        std::streamsize size = file.tellg();
        if(size <= 0){
            std::cout << "info string: ----->  Could not open opening book" << std::endl;
            return opening_book;
        }

        file.seekg(0,std::ios::beg);
        std::vector<char> buffer(size);

        if(!file.read(buffer.data(),size)){
             std::cout << "info string: ----->  Could not read opening book into buffer" << std::endl;
             return opening_book;
        }

        size_t num_entries = buffer.size() / 16;
        
        for(size_t idx=0; idx<num_entries; ++idx){
            size_t offset = idx * 16;
            
            uint64_t key;
            std::memcpy(&key,buffer.data()+offset+0,sizeof(key));//TODO: need to verify this actually works lol. File is encoded big-endian as I understand it
            key = __builtin_bswap64(key);

            uint16_t move;
            std::memcpy(&move,buffer.data()+offset+8,sizeof(move));
            move = __builtin_bswap16(move);

            uint16_t weight;
            std::memcpy(&weight,buffer.data()+offset+10,sizeof(weight));
            weight = __builtin_bswap16(weight);

            uint32_t learn;
            std::memcpy(&learn,buffer.data()+offset+12,sizeof(learn));
            learn = __builtin_bswap32(learn);



            //--------------turn the bit representation into a move object-------------------
            
            int dest_file = (move >> 0) & 0x7;
            int dest_rank = (move >> 3) & 0x7;

            chess::Rank drank = chess::Rank(dest_rank);
            chess::File dfile = chess::File(dest_file);
            chess::Square destination = chess::Square(dfile,drank);

            int src_file = (move >> 6) & 0x7;
            int src_rank = (move >> 9) & 0x7;
            

            chess::Rank srank = chess::Rank(src_rank);
            chess::File sfile = chess::File(src_file);
            chess::Square source = chess::Square(sfile,srank);

            BookEntry entry;
            entry.key = key;
            entry.move_source = source;
            entry.move_dest = destination;
            entry.weight = weight;

            auto it = opening_book.find(key);
            if (it != opening_book.end()) {//found the key (hashed board)
                auto& vec = it->second;
                std::size_t curr_idx = 0;
                while (curr_idx < vec.size() && vec[curr_idx].weight > entry.weight) {
                    curr_idx += 1;
                }
                vec.insert(vec.begin() + curr_idx, entry);
            } else {//not found
                opening_book[key].push_back(entry);
            }

        }


        //for each key, normalize the weight to a 0-1 weight
        for(auto& [key,value] : opening_book){
            int weight_sum = 0;
            for(auto& entry : value){
                weight_sum += entry.weight;
            }

            float cum_weight = 0.0;
            for(size_t i=0; i< value.size(); i++){
                float prob = static_cast<float>(value[i].weight) / static_cast<float>(weight_sum);
                value[i].normalized_weight = prob;
                
                cum_weight += prob;
                value[i].cum_weight = cum_weight;
            }

        }

        return opening_book;

    }

    void cmd_uci(){
        std::cout << "id name nullptrbot" << std::endl;
        std::cout << "id author alex" << std::endl;
        std::cout << "uciok" << std::endl;
    }

    void cmd_ucinewgame(){
        this->in_book = true;
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
            for(int i=2; i<tokens.size(); ++i){
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
                time_limit_ms = static_cast<int>(std::lround(w_think_time));
            }else{
                time_limit_ms = static_cast<int>(std::lround(b_think_time));
            }
        }

        chess::Move best_move = find_move(this,this->board,time_limit_ms,true);
        std::string s = chess::uci::moveToUci(best_move,false);

        std::cout << "bestmove " << s << std::endl;

    }

    void cmd_searchbenchmark(std::vector<std::string> tokens){
        return;
    }

    void cmd_gstatus(){
        //print current board
        std::cout << this->board.getFen() << std::endl;

        //print hashtable

        //print random seed
        std::cout << "Random seed: " << this->random_seed << std::endl;
    }

    void cmd_eval(){
       return;
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
        return;
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

    void cmd_setmovetime(std::vector<std::string> tokens){
        if(tokens.size() > 2){
            std::cout << "Too many tokens to set time" << std::endl;
            return;
        }

        try{
            this->think_time_ms = std::stoi(tokens[1]);
        }catch(...){
            std::cout << "could not set time" << std::endl;
            return;
        }
    }

    void cmd_debugon(){
        this->debug = true;
    }

    void cmd_debugoff(){
        this->debug = false;
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
                }else if(input_sep[0] == "debugon"){
                    cmd_debugon();
                }else if(input_sep[0] == "debugoff"){
                    cmd_debugoff();
                }else if(input_sep[0] == "setmovetime"){
                    cmd_setmovetime(input_sep);
                }
                                
                else{
                    std::cout << "unrecognized command" << std::endl;
                    std::cout << input_sep[0] << std::endl;
                }
            }

        }
    }

};


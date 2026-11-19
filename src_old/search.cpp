

#include "search.h"
#include "engine.h"
#include "chess.hpp"
#include "eval.h"

#include <cmath>

void simple_ab_search(ChessEngine& ce){
    ce.cmd_gstatus();
}


chess::Move deepening_negamax_caller(ChessEngine& ce, chess::Board& board, bool print_info, int time_limit_ms){
    chess::Movelist moves;
    chess::movegen::legalmoves(moves,board);

    chess::Move best_move = chess::Move::NULL_MOVE;
    int best_val = -1e9;

    //get initial time and deadline time
    long long start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();//this measures in nanoseconds
    long long deadline = start_time + static_cast<long long>(time_limit_ms*1000);

    bool keep_searching = true;
    
    int depth = 1;
    while(keep_searching){
        for(const chess::Move& move : moves){
            board.makeMove(move);

            int move_value = -1 * negamax_quiescent_search(ce,board,depth-1,-1e9,1e9,deadline);

            board.unmakeMove(move);

            long long now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            

            if(move_value > best_val){
                best_val = move_value;
                best_move = move;
            }

            if(now > deadline){
                keep_searching = false;
                return best_move;
            }

            if(print_info){
                std::cout << "---------------" << std::endl;
                std::cout << chess::uci::moveToUci(move) << std::endl;
                std::cout << std::fixed << move_value << std::endl;
                std::cout << "(Current best move): " << chess::uci::moveToUci(best_move) << " " << std::fixed << best_val << std::endl;
            }
        }

        depth += 1;
    }

    return best_move;
    
}

chess::Move negamax_caller(ChessEngine& ce, chess::Board& board, int depth, bool print_info, int time_limit_ms){
    
    chess::Movelist moves;
    chess::movegen::legalmoves(moves,board);

    chess::Move best_move = chess::Move::NULL_MOVE;
    int best_val = -1e9;

    //get initial time and deadline time
    long long start_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();//this measures in nanoseconds
    long long deadline = start_time + static_cast<long long>(time_limit_ms*1000);


    for(const chess::Move& move : moves){
        board.makeMove(move);

        int move_value = -1 * negamax_quiescent_search(ce,board,depth-1,-1e9,1e9,deadline);

        board.unmakeMove(move);

       

        if(move_value > best_val){
            best_val = move_value;
            best_move = move;
        }

         if(print_info){
            std::cout << "---------------" << std::endl;
            std::cout << chess::uci::moveToUci(move) << std::endl;
            std::cout << std::fixed << move_value << std::endl;
            std::cout << "(Current best move): " << chess::uci::moveToUci(best_move) << " " << std::fixed << best_val << std::endl;
        }

    }

    return best_move;

}



int negamax_quiescent_search(ChessEngine& ce, chess::Board& board, int  depth, float alpha, float beta, long long deadline){
    /*
    //--here, we will allow depth to go negative
    if node is terminal, return eval

    if depth <= 0
        if no captures
            return eval
        if captures
            movelist = captures
    
    if depth > 0
        movelist = allmoves
    
    sort moves with captures first

    do the regular search
    */

    if(board.isHalfMoveDraw()){
        return board.getHalfMoveDrawType().first == chess::GameResultReason::CHECKMATE ? 
        -1e8 - depth: 0;
    }

    if(board.isRepetition()){
        return 0;
    }

    chess::Movelist move_list;
    long long now = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    if(depth <= 0 || now > deadline){
        chess::movegen::legalmoves<chess::movegen::MoveGenType::CAPTURE>(move_list,board);
        if(move_list.empty() || now > deadline){
            //return eval
            ce.nodes_evaluated += 1;
            int color_mult = board.sideToMove()==chess::Color::WHITE ? 1 : -1;        
            return evaluate_simple(board) * color_mult;
        }
    }else{
        chess::movegen::legalmoves<chess::movegen::MoveGenType::ALL>(move_list,board);
        std::stable_sort(move_list.begin(), move_list.end(), 
        [&board](const chess::Move& a, const chess::Move& b) {
            // Captures before non-captures
            return (board.at(a.to()) != chess::Piece::NONE) > 
                (board.at(b.to()) != chess::Piece::NONE);
        });
    }


    if(move_list.empty()){
        return board.inCheck() ? -1e8 - depth : 0;
    }
    
    int value = -1e9;
    for(chess::Move m : move_list){
        board.makeMove(m);
        
        int move_val = -1 * negamax_quiescent_search(ce,board,depth-1,-1*beta,-1*alpha,deadline);
        value = move_val > value ? move_val : value;//max(value,move_val)
        alpha = alpha>value ? alpha : value;//alpha=max(alpha,value)
        

        board.unmakeMove(m);
    
        if(alpha >= beta){
            break;
        }
    
    }

    return value;


}

int negamax_search(ChessEngine& ce, chess::Board& board, int depth, float alpha, float beta, long long deadline){
    

    //I discount the checkmate value (-1e8) be depth to prefer holding off for as long as possible if you're losign

    //if node is terminal, return eval
    //=============================================================
    if(board.isHalfMoveDraw()){
        return board.getHalfMoveDrawType().first == chess::GameResultReason::CHECKMATE ? 
        -1e8 - depth: 0;
    }

    if(board.isRepetition()){
        return 0;
    }



    //=============================================================

    long long now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    if(depth <= 0 || now >= deadline){
        ce.nodes_evaluated += 1;
        int color_mult = board.sideToMove()==chess::Color::WHITE ? 1 : -1;
    
        return evaluate_simple(board) * color_mult;
    }


    //----------------------------
    chess::Movelist moves;
    chess::movegen::legalmoves<chess::movegen::MoveGenType::ALL>(moves,board);
    



    //sort the moves based on captures first
    std::stable_sort(moves.begin(), moves.end(), 
    [&board](const chess::Move& a, const chess::Move& b) {
        // Captures before non-captures
        return (board.at(a.to()) != chess::Piece::NONE) > 
               (board.at(b.to()) != chess::Piece::NONE);
    });



    if(moves.empty()){
        return board.inCheck() ? -1e8 - depth : 0;
    }
    
    int value = -1e9;
    for(chess::Move m : moves){
        board.makeMove(m);
        
        int move_val = -1 * negamax_search(ce,board,depth-1,-1*beta,-1*alpha,deadline);
        value = move_val > value ? move_val : value;//max(value,move_val)
        alpha = alpha>value ? alpha : value;//alpha=max(alpha,value)
        

        board.unmakeMove(m);
    
        if(alpha >= beta){
            break;
        }
    
    }


    return value;
}


void playout(ChessEngine& ce, int depth_limit){
    chess::Movelist moves;
    chess::Board p_board = ce.board;

    int i = 0;

    bool game_playing = true;
    while(game_playing && i<depth_limit){
        std::cout << "------------------------------" << std::endl;


        chess::movegen::legalmoves(moves,p_board);
        
        int num_moves = moves.size();
        std::uniform_int_distribution<> dist(0,num_moves);//may be faster to predefine a distribution and do rejection sampling over the number of moves?
        int random_idx = dist(ce.rng);

        chess::Move selected_move = moves[random_idx];
        p_board.makeMove(selected_move);

        auto s = chess::uci::moveToUci(selected_move);
        std::cout << s << std::endl;
        std::cout << p_board.getFen() << std::endl;
        i++;



        if(p_board.isGameOver().second != chess::GameResult::NONE){
            game_playing = false;
        }
    }


    
}

void playout_wrapper(ChessEngine& ce){
    playout(ce,100);
}

void playout_wrapper(ChessEngine& ce, int depth_limit){
    playout(ce,depth_limit);
}


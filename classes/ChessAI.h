#pragma once

#include "Bitboard.h"
#include <array>
#include <vector>

class Chess;

class ChessAI {
public:
    // Self-contained board snapshot used by the search tree
    struct State {
        std::array<int, 64> board;
        bool canCastleKingSide[2];
        bool canCastleQueenSide[2];
        int  enPassantSquare; // -1 if none
        int  sideToMove;      // 0 = white, 1 = black

        State() : board{}, enPassantSquare(-1), sideToMove(0) {
            board.fill(0);
            canCastleKingSide[0]  = canCastleKingSide[1]  = true;
            canCastleQueenSide[0] = canCastleQueenSide[1] = true;
        }
    };

    static State   fromChess(Chess* chess);
    static int     evaluate(const State& state);
    static std::vector<BitMove> generateMoves(const State& state);
    static State   applyMove(const State& state, const BitMove& move);
    static BitMove getBestMove(const State& state, int depth = 3);

private:
    static bool isSquareAttacked(const std::array<int,64>& board, int square, int attacker);
    static bool isInCheck(const std::array<int,64>& board, int player);
    static int  negamax(const State& state, int depth, int alpha, int beta);
    static int  pieceValue(int pieceType);
    static int  pstValue(int pieceType, int pstIdx);

    // PST tables written rank 8 → rank 1, file a → h
    // White lookup: (7 - sq/8)*8 + (sq%8)   Black lookup: sq
    static const int PST_PAWN[64];
    static const int PST_KNIGHT[64];
    static const int PST_BISHOP[64];
    static const int PST_ROOK[64];
    static const int PST_QUEEN[64];
    static const int PST_KING_MG[64];
};

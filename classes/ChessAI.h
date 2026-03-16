#pragma once

#include "Bitboard.h"
#include <array>
#include <vector>

class Chess; // forward declaration — ChessAI.cpp includes Chess.h

// ---------------------------------------------------------------------------
// ChessAI
//   Owns all "pure-logic" chess AI: board state, static evaluation, and
//   (in later steps) move generation and negamax search.
//
//   The State struct is a self-contained snapshot of everything needed for
//   search — no dependency on the visual Grid/Bit layer.
// ---------------------------------------------------------------------------
class ChessAI {
public:
    // -----------------------------------------------------------------------
    // State: complete board snapshot for the search tree
    // -----------------------------------------------------------------------
    struct State {
        std::array<int, 64> board;   // tag per square (0 = empty)
        bool canCastleKingSide[2];
        bool canCastleQueenSide[2];
        int  enPassantSquare;        // -1 if none
        int  sideToMove;             // 0 = white, 1 = black

        State() : board{}, enPassantSquare(-1), sideToMove(0) {
            board.fill(0);
            canCastleKingSide[0]  = canCastleKingSide[1]  = true;
            canCastleQueenSide[0] = canCastleQueenSide[1] = true;
        }
    };

    // Build a State snapshot from the live Chess game
    static State fromChess(Chess* chess);

    // Static evaluation from sideToMove's perspective (centipawns).
    // Positive = good for sideToMove, negative = bad.
    static int evaluate(const State& state);

private:
    // ------ scoring helpers ------------------------------------------------
    static int  pieceValue(int pieceType);
    static int  pstValue(int pieceType, int pstIdx);

    // ------ piece-square tables (written rank 8 → rank 1, file a → h) ------
    // Index convention:
    //   White piece at board square s  →  pstIdx = (7 - s/8)*8 + (s%8)
    //   Black piece at board square s  →  pstIdx = s
    // This gives both sides the same positional incentives symmetrically.
    static const int PST_PAWN[64];
    static const int PST_KNIGHT[64];
    static const int PST_BISHOP[64];
    static const int PST_ROOK[64];
    static const int PST_QUEEN[64];
    static const int PST_KING_MG[64];
};

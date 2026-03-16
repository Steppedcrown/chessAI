#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"
#include <array>
#include <vector>

constexpr int pieceSize = 80;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    void onBitPickedUp(Bit& bit, BitHolder& src) override;
    void clearBoardHighlights() override;

    void drawFrame() override;
    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    std::vector<BitMove> generateAllMoves();

    bool gameHasAI() override;
    void updateAI()  override;

    // Accessors for ChessAI
    std::array<int, 64> getBoardArray() const;
    bool getCanCastleKingSide(int player)  const { return _canCastleKingSide[player]; }
    bool getCanCastleQueenSide(int player) const { return _canCastleQueenSide[player]; }
    int  getEnPassantSquare()              const { return _enPassantSquare; }

    Grid* getGrid() override { return _grid; }

private:
    void applyAIMove(const BitMove& move);
    void promotePawn(int square, int player, ChessPiece piece);

    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    void buildBitboards(int player, BitboardElement& occupied, BitboardElement& friendly, BitboardElement& enemy);
    BitboardElement getPawnMoves(ChessSquare* src, int player);
    BitboardElement getJumpMoves(ChessSquare* src, int player, const int offsets[][2], int numOffsets);
    BitboardElement getSlidingMoves(ChessSquare* src, int player, const int dirs[][2], int numDirs);

    Grid* _grid;
    std::vector<char> _moveList;
    int _moveCount = 0;
    bool _canCastleKingSide[2];
    bool _canCastleQueenSide[2];
    int _enPassantSquare;
    bool _promotionPending = false;
    int  _promotionSquare  = -1;
    int  _promotionPlayer  = -1;
};
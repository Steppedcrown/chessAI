# Chess Project Plan

## Goal
Finish the chess program so it supports full legal chess movement rules (checks, checkmate, castling, en passant) and add a computer opponent using a negamax search with alpha/beta pruning (depth >= 3). 

---

## 2) Implement Full Chess Rules

### ✅ Step 2: Enable checks and checkmate detection
- Ensure move generation can identify if the king is in check.
- Filter out moves that leave the player's own king in check.
- Implement a function `bool isInCheck(Color side)` or similar.
- Implement `bool isCheckmate(Color side)` by checking:
  - king is in check, and
  - there are no legal moves that escape check.

### ✅ Step 3: Castling (king-side and queen-side)
- Add castling rights tracking (per side): `canCastleKingSide`, `canCastleQueenSide`.
- Update rights when king or rook moves (or rook is captured).
- Generate castle moves only if:
  - King and rook haven’t moved.
  - No pieces between king and rook.
  - King is not currently in check.
  - King does not pass through or end on a square under attack.
- Implement move execution and undo for castling.

### ✅ Step 4: En Passant
- Track en passant target square after a 2-square pawn advance.
- Generate en passant capture moves for opposing pawns.
- Apply and undo en passant captures correctly (capture the pawn that “passed by”).
- Ensure en passant is illegal if it would leave own king in check.

---

## 3) Negamax AI with Alpha/Beta Pruning

### ✅ Step 5: Board evaluation function
- Implement a static evaluation function `int evaluateBoard(Color side)`.
- Include:
  - Material values (e.g., pawn=100, knight=320, bishop=330, rook=500, queen=900, king=20000).
  - Piece-square tables (bonus/penalty based on piece position).
  - Simple mobility or king safety heuristics (optional but helpful).
- Ensure evaluation is from the perspective of the side to move (negamax style).

### ✅ Step 6: Negamax search (depth >= 3) with alpha/beta pruning
- Implement `int negamax(Board &board, int depth, int alpha, int beta, Color sideToMove)`.
- Use recursion and swap sign: `score = -negamax(..., -beta, -alpha, oppositeSide)`.
- Return a score in centipawns.
- At leaf nodes or depth 0, return static evaluation.
- Incrementally update alpha and prune when `alpha >= beta`.

### ✅ Step 7: Move ordering and basic optimization (optional but helpful)
- Order moves so captures and checks are searched first.
- Use a simple heuristic (capture value, checks, promotions).

### ✅ Step 8: Integrate AI into game loop
- Add UI/control to pick AI side (WHITE, BLACK, or none).
- When it is the AI’s turn, call negamax search to pick best move.
- Apply the chosen move and continue the game loop.
- Ensure searching for depth 3 runs quickly (under a few seconds) for a standard mid-game position.

---

## 4) Testing & Validation

### ✅ Step 9: Regression tests / manual verification
- Verify basic move generation still works for all pieces.
- Test manually: illegal king moves, illegal castling through check, en passant capture, promoting pawns.
- Test check detection and checkmate recognition in classic mate patterns.
- Validate AI plays legal moves and responds (depth 3 search should choose captures and avoid blunders).

### ✅ Step 10: Document usage
- Add notes to `readme.md` explaining:
  - How to run the game vs AI.
  - How to select which side the AI plays.
  - How to enable/disable debug output.

---

## 5) Optional Enhancements (if time permits)

- Add a transposition table (Zobrist hashing) to speed up repeated positions.
- Add iterative deepening (depth 1..N) with a time limit.
- Add a basic opening book or move history / undo support.

---

## Success Criteria ✅
- Full legal chess rules (checks, checkmates, castling, en passant) are correctly enforced.
- AI can play as either WHITE or BLACK and uses negamax + alpha/beta with depth >= 3.
- The finished work is in a separate fork/clone repository so the original assignment is preserved.

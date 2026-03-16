# Chess AI

A fully playable chess program with a negamax AI opponent, built on top of a the previous chess assignment code.

*Note: this project was developed with generative AI assistance.*

---

## How to Use

Launch the program and use the **Settings** panel on the left side of the window.

**Starting a game:**
- **Start Chess** — two human players, local hotseat
- **Start Chess vs AI (you play White)** — you control White, AI controls Black and moves first after you
- **Start Chess vs AI (you play Black)** — AI controls White and moves immediately on startup

**AI Depth slider:** appears both before starting (to configure before the game begins) and during an active AI game so you can adjust it live. Higher depth means stronger play but longer think time. A value of 6 is the default and plays well; 8+ will noticeably slow down in complex positions.

**Resetting:** when the game ends a "Reset Game" button appears in Settings.

---

## Chess Rules Implemented

### Castling
Both kingside and queenside castling are supported for both sides. The king cannot castle if it is currently in check, passes through an attacked square, or lands on an attacked square. Castling rights are permanently revoked if the king or the relevant rook moves at any point during the game, even if they return to their original squares.

### En Passant
After any double pawn push, the opposing side has exactly one move to capture en passant. The capture is validated for legality (it cannot leave the capturing player's king in check), and the captured pawn is correctly removed from the board.

### Pawn Promotion
When a pawn reaches the back rank a modal dialog appears letting the human player choose between Queen, Rook, Bishop, or Knight. No other moves are allowed until the choice is made. The AI always auto-promotes to a Queen.

---

## AI — Negamax with Alpha-Beta Pruning

### How it works
The AI uses a negamax search with alpha-beta pruning. At each node it generates all legal moves, applies them to a self-contained board state (no dependency on the visual layer), and recursively evaluates the resulting positions. Alpha-beta pruning cuts branches that cannot possibly affect the final result, dramatically reducing the number of nodes evaluated.

The static evaluation function scores each position using:
- **Material values** — Pawn 100, Knight 320, Bishop 330, Rook 500, Queen 900, King 20000 centipawns
- **Piece-square tables** — positional bonuses and penalties for each piece type encouraging good structure (central knights, advanced pawns, castled king, rooks on open files, etc.)

Move ordering (MVV-LVA — Most Valuable Victim, Least Valuable Attacker) sorts captures and promotions to the front of the list at every node. Searching likely-good moves first maximises alpha-beta cutoffs, roughly halving the effective branching factor and allowing the same depth to be reached in a fraction of the time.

### Depth achieved
The default search depth is **6 half-moves (plies)**. With move ordering, depth 6 runs in well under a second on most middlegame positions. Depth 8 is typically 2–5 seconds and produces noticeably stronger play. The slider goes up to 10 for experimentation, though positions with many pieces at depth 9–10 can take tens of seconds.

### How well it plays
At depth 6 the AI plays solidly — it captures hanging pieces, avoids simple blunders, develops towards the centre, and keeps its king safe behind pawns. It will find basic tactics a few moves deep (forks, pins, back-rank threats) and promote pawns efficiently. It does not use an opening book or a transposition table, so it can be slow to commit to a plan in quiet positional games, and it may repeat moves in symmetrical positions. A casual player will find it a reasonable challenge; a strong club player will be able to outmanoeuvre it in complex strategic positions. It's decision making typically takes in the range of 2-10 seconds depending on the complexity of the position.

---

## Challenges

**Castling rook movement** — the biggest early bug was that the rook was being deleted when the king castled. `BitHolder::setBit(nullptr)` deletes whatever piece it currently holds if that piece's parent is still the source square. The fix was to change the rook's parent to the destination square *before* clearing the source, so the ownership check in `setBit` sees a mismatch and skips deletion.

**Separating AI logic from the visual layer** — the game framework uses a `Grid` of `BitHolder` squares with rendered `Bit` pieces. Running a negamax search requires making and unmaking thousands of moves per second, which is impossible on the live visual board. The solution was a separate `ChessAI::State` struct (a plain 64-integer array plus castling and en passant flags) that the search operates on entirely independently, with a `fromChess()` snapshot and an `applyMove()` that returns new states immutably.

**Move legality filtering** — every candidate move must be verified not to leave the moving side's king in check. This is done by applying the move to the board array and calling `isInCheck` on the result. The same approach is used in both the visual game (via `generateAllMoves`) and the AI search (via `ChessAI::generateMoves`), keeping the logic consistent.

**Pawn promotion turn flow** — the framework ends the turn inside `bitMovedFromTo` by calling `endTurn()` via `Game::bitMovedFromTo`. For human promotion, the turn needed to stay open until the player picked a piece. This was solved by returning early from `bitMovedFromTo` before the `Game::` call when promotion is pending, then calling `endTurn()` directly from the ImGui dialog callback once a piece is chosen.

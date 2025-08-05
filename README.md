# CheckersC

This app presents a classic checkers game with a native window on Windows. It is written in C with CMake and the Win32 API.

AI overview
The computer player uses a minimax search with alpha beta pruning. The evaluation favors material kings center control forward progress and move freedom. Move generation enforces our capture rule and supports multi jump sequences.

Sources
Wikipedia articles on minimax and alpha beta pruning
Computer checkers overview on Wikipedia
Russell and Norvig Artificial Intelligence A Modern Approach

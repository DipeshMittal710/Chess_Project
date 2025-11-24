Chess — Terminal Based (C Project)

A simple yet powerful terminal-based Chess game built entirely in C, designed for clarity, accuracy, and learning.
The project handles complete board logic, move validation, piece rules, and turn-based gameplay — all inside the command line.

🚀 Features

Full 8×8 chessboard implemented in C
Move validation for all pieces (Pawn, Rook, Knight, Bishop, Queen, King)
Turn-by-turn gameplay
Illegal move detection
Clean board rendering in terminal
Simple menu & interaction
Beginner-friendly modular code
Easy to extend (AI, save-load, timers, etc.)

🛠️ How It Works

Chessboard is stored using a 2D 8×8 matrix, and each piece is represented using characters.
Move validation is handled through clear functions that check:
Piece type
Target cell
Path clearing
Check for invalid moves
Game loop runs until a player quits or game ends.

📂 Project Structure
chess/
│
├── chess.c        # Main game file
└── README.md      # You're reading this :)

▶️ How to Run
Linux / macOS
gcc chess.c -o chess
./chess

Windows (MinGW)
gcc chess.c -o chess.exe
chess.exe

📘 Controls / Input Format
Enter moves in a simple format like:

e2 e4
b1 c3


Lowercase/uppercase both accepted
Invalid moves show an error message
Quit anytime using:
exit

👨‍💻 Author

Dipesh Mittal
A simple beginner-friendly chess logic demo made for learning C and building logic skills.

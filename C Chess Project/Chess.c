#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BOARD_DIMENSION 8
#define MAX_MOVE_HISTORY 1024
#define MAX_INPUT_LINE 256

typedef enum
{
    PIECE_EMPTY = 0,
    PIECE_WHITE_PAWN,
    PIECE_WHITE_KNIGHT,
    PIECE_WHITE_BISHOP,
    PIECE_WHITE_ROOK,
    PIECE_WHITE_QUEEN,
    PIECE_WHITE_KING,
    PIECE_BLACK_PAWN,
    PIECE_BLACK_KNIGHT,
    PIECE_BLACK_BISHOP,
    PIECE_BLACK_ROOK,
    PIECE_BLACK_QUEEN,
    PIECE_BLACK_KING
} PieceType;

typedef struct
{
    int row;
    int col;
} BoardPosition;

typedef struct
{
    PieceType board[BOARD_DIMENSION][BOARD_DIMENSION];
    int is_white_turn;

    int white_can_castle_kingside;
    int white_can_castle_queenside;
    int black_can_castle_kingside;
    int black_can_castle_queenside;

    BoardPosition en_passant_target;

    int halfmove_clock;
    int fullmove_number;
} GameState;

typedef struct
{
    BoardPosition from;
    BoardPosition to;
    PieceType moved_piece;
    PieceType captured_piece;

    int prev_white_can_castle_kingside;
    int prev_white_can_castle_queenside;
    int prev_black_can_castle_kingside;
    int prev_black_can_castle_queenside;

    BoardPosition prev_en_passant_target;
    int prev_halfmove_clock;
    int prev_fullmove_number;

    PieceType promotion_piece;
} MoveRecord;

static GameState current_game_state;
static MoveRecord move_history[MAX_MOVE_HISTORY];
static int move_history_length = 0;

char piece_to_char(PieceType piece)
{
    switch (piece)
    {
    case PIECE_WHITE_PAWN:
        return 'P';
    case PIECE_WHITE_KNIGHT:
        return 'N';
    case PIECE_WHITE_BISHOP:
        return 'B';
    case PIECE_WHITE_ROOK:
        return 'R';
    case PIECE_WHITE_QUEEN:
        return 'Q';
    case PIECE_WHITE_KING:
        return 'K';
    case PIECE_BLACK_PAWN:
        return 'p';
    case PIECE_BLACK_KNIGHT:
        return 'n';
    case PIECE_BLACK_BISHOP:
        return 'b';
    case PIECE_BLACK_ROOK:
        return 'r';
    case PIECE_BLACK_QUEEN:
        return 'q';
    case PIECE_BLACK_KING:
        return 'k';
    default:
        return '.';
    }
}

int position_in_bounds(int row, int col)
{
    if (row < 0)
        return 0;
    if (col < 0)
        return 0;
    if (row >= BOARD_DIMENSION)
        return 0;
    if (col >= BOARD_DIMENSION)
        return 0;
    return 1;
}

int algebraic_to_position(const char *text, BoardPosition *out_pos)
{
    if (text == NULL)
        return 0;
    size_t length = strlen(text);
    if (length < 2)
        return 0;

    char file_char = tolower((unsigned char)text[0]);
    char rank_char = text[1];

    if (file_char < 'a' || file_char > 'h')
        return 0;
    if (rank_char < '1' || rank_char > '8')
        return 0;

    int column = file_char - 'a';
    int rank_number = rank_char - '0';
    int row = 8 - rank_number;

    if (!position_in_bounds(row, column))
        return 0;

    out_pos->row = row;
    out_pos->col = column;
    return 1;
}

void position_to_algebraic(BoardPosition pos, char *out_buffer)
{
    out_buffer[0] = 'a' + pos.col;
    int rank_number = 8 - pos.row;
    out_buffer[1] = '0' + rank_number;
    out_buffer[2] = '\0';
}

void copy_game_state(GameState *destination, const GameState *source)
{
    *destination = *source;
}

void initialize_game_state(GameState *state)
{
    PieceType initial_board[BOARD_DIMENSION][BOARD_DIMENSION] = {
        {PIECE_BLACK_ROOK, PIECE_BLACK_KNIGHT, PIECE_BLACK_BISHOP, PIECE_BLACK_QUEEN, PIECE_BLACK_KING, PIECE_BLACK_BISHOP, PIECE_BLACK_KNIGHT, PIECE_BLACK_ROOK},
        {PIECE_BLACK_PAWN, PIECE_BLACK_PAWN, PIECE_BLACK_PAWN, PIECE_BLACK_PAWN, PIECE_BLACK_PAWN, PIECE_BLACK_PAWN, PIECE_BLACK_PAWN, PIECE_BLACK_PAWN},
        {PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY},
        {PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY},
        {PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY},
        {PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY, PIECE_EMPTY},
        {PIECE_WHITE_PAWN, PIECE_WHITE_PAWN, PIECE_WHITE_PAWN, PIECE_WHITE_PAWN, PIECE_WHITE_PAWN, PIECE_WHITE_PAWN, PIECE_WHITE_PAWN, PIECE_WHITE_PAWN},
        {PIECE_WHITE_ROOK, PIECE_WHITE_KNIGHT, PIECE_WHITE_BISHOP, PIECE_WHITE_QUEEN, PIECE_WHITE_KING, PIECE_WHITE_BISHOP, PIECE_WHITE_KNIGHT, PIECE_WHITE_ROOK}};

    memcpy(state->board, initial_board, sizeof(initial_board));
    state->is_white_turn = 1;

    state->white_can_castle_kingside = 1;
    state->white_can_castle_queenside = 1;
    state->black_can_castle_kingside = 1;
    state->black_can_castle_queenside = 1;

    state->en_passant_target.row = -1;
    state->en_passant_target.col = -1;

    state->halfmove_clock = 0;
    state->fullmove_number = 1;
}

void print_board_state(const GameState *state)
{
    printf("\n    a b c d e f g h\n");
    printf("  +-----------------+\n");
    for (int row = 0; row < BOARD_DIMENSION; ++row)
    {
        int rank_number = 8 - row;
        printf("%d |", rank_number);
        for (int col = 0; col < BOARD_DIMENSION; ++col)
        {
            char symbol = piece_to_char(state->board[row][col]);
            printf(" %c", symbol);
        }
        printf(" | %d\n", rank_number);
    }
    printf("  +-----------------+\n");
    printf("    a b c d e f g h\n");

    if (state->is_white_turn)
    {
        printf("\nTurn: White\n");
    }
    else
    {
        printf("\nTurn: Black\n");
    }

    printf("Castling rights: ");
    if (state->white_can_castle_kingside)
        printf("K");
    if (state->white_can_castle_queenside)
        printf("Q");
    if (!state->white_can_castle_kingside && !state->white_can_castle_queenside)
        printf("-");
    printf(" ");

    if (state->black_can_castle_kingside)
        printf("k");
    if (state->black_can_castle_queenside)
        printf("q");
    if (!state->black_can_castle_kingside && !state->black_can_castle_queenside)
        printf("-");
    printf("\n");

    if (state->en_passant_target.row == -1)
    {
        printf("En-passant target: -\n");
    }
    else
    {
        char buffer[4];
        position_to_algebraic(state->en_passant_target, buffer);
        printf("En-passant target: %s\n", buffer);
    }

    printf("Halfmove clock: %d | Fullmove number: %d\n",
           state->halfmove_clock, state->fullmove_number);
}

int square_attacked_by(const GameState *state, int target_row, int target_col, int attacker_is_white)
{

    if (attacker_is_white)
    {
        int attack_row = target_row - 1;
        int left_col = target_col - 1;
        int right_col = target_col + 1;
        if (position_in_bounds(attack_row, left_col) && state->board[attack_row][left_col] == PIECE_WHITE_PAWN)
        {
            return 1;
        }
        if (position_in_bounds(attack_row, right_col) && state->board[attack_row][right_col] == PIECE_WHITE_PAWN)
        {
            return 1;
        }
    }
    else
    {
        int attack_row = target_row + 1;
        int left_col = target_col - 1;
        int right_col = target_col + 1;
        if (position_in_bounds(attack_row, left_col) && state->board[attack_row][left_col] == PIECE_BLACK_PAWN)
        {
            return 1;
        }
        if (position_in_bounds(attack_row, right_col) && state->board[attack_row][right_col] == PIECE_BLACK_PAWN)
        {
            return 1;
        }
    }

    const int knight_offsets[8][2] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}};
    for (int i = 0; i < 8; ++i)
    {
        int check_row = target_row + knight_offsets[i][0];
        int check_col = target_col + knight_offsets[i][1];
        if (!position_in_bounds(check_row, check_col))
            continue;
        PieceType occupant = state->board[check_row][check_col];
        if (attacker_is_white && occupant == PIECE_WHITE_KNIGHT)
            return 1;
        if (!attacker_is_white && occupant == PIECE_BLACK_KNIGHT)
            return 1;
    }

    const int orthogonal_dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (int d = 0; d < 4; ++d)
    {
        int dr = orthogonal_dirs[d][0];
        int dc = orthogonal_dirs[d][1];
        int r = target_row + dr;
        int c = target_col + dc;
        while (position_in_bounds(r, c))
        {
            PieceType occupant = state->board[r][c];
            if (occupant != PIECE_EMPTY)
            {
                if (attacker_is_white)
                {
                    if (occupant == PIECE_WHITE_ROOK || occupant == PIECE_WHITE_QUEEN)
                    {
                        return 1;
                    }

                    if (occupant == PIECE_WHITE_KING && r == target_row + dr && c == target_col + dc)
                    {
                        return 1;
                    }
                }
                else
                {
                    if (occupant == PIECE_BLACK_ROOK || occupant == PIECE_BLACK_QUEEN)
                    {
                        return 1;
                    }
                    if (occupant == PIECE_BLACK_KING && r == target_row + dr && c == target_col + dc)
                    {
                        return 1;
                    }
                }
                break;
            }
            r += dr;
            c += dc;
        }
    }

    const int diagonal_dirs[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    for (int d = 0; d < 4; ++d)
    {
        int dr = diagonal_dirs[d][0];
        int dc = diagonal_dirs[d][1];
        int r = target_row + dr;
        int c = target_col + dc;
        while (position_in_bounds(r, c))
        {
            PieceType occupant = state->board[r][c];
            if (occupant != PIECE_EMPTY)
            {
                if (attacker_is_white)
                {
                    if (occupant == PIECE_WHITE_BISHOP || occupant == PIECE_WHITE_QUEEN)
                    {
                        return 1;
                    }
                    if (occupant == PIECE_WHITE_KING && r == target_row + dr && c == target_col + dc)
                    {
                        return 1;
                    }
                }
                else
                {
                    if (occupant == PIECE_BLACK_BISHOP || occupant == PIECE_BLACK_QUEEN)
                    {
                        return 1;
                    }
                    if (occupant == PIECE_BLACK_KING && r == target_row + dr && c == target_col + dc)
                    {
                        return 1;
                    }
                }
                break;
            }
            r += dr;
            c += dc;
        }
    }

    return 0;
}

int find_king_position(const GameState *state, int white, BoardPosition *king_pos)
{
    PieceType target_king = white ? PIECE_WHITE_KING : PIECE_BLACK_KING;
    for (int r = 0; r < BOARD_DIMENSION; ++r)
    {
        for (int c = 0; c < BOARD_DIMENSION; ++c)
        {
            if (state->board[r][c] == target_king)
            {
                king_pos->row = r;
                king_pos->col = c;
                return 1;
            }
        }
    }
    return 0;
}

int is_in_check(const GameState *state, int white)
{
    BoardPosition king_position;
    int found = find_king_position(state, white, &king_position);
    if (!found)
    {
        return 0;
    }
    int attacker_is_white = 0;
    if (white)
    {
        attacker_is_white = 0;
    }
    else
    {
        attacker_is_white = 1;
    }
    return square_attacked_by(state, king_position.row, king_position.col, attacker_is_white);
}

int is_pseudo_legal_move(const GameState *state, BoardPosition from, BoardPosition to, PieceType promotion_piece)
{
    if (!position_in_bounds(from.row, from.col))
        return 0;
    if (!position_in_bounds(to.row, to.col))
        return 0;

    PieceType piece = state->board[from.row][from.col];
    if (piece == PIECE_EMPTY)
        return 0;

    int moving_piece_is_white = 0;
    if (piece == PIECE_WHITE_PAWN || piece == PIECE_WHITE_KNIGHT || piece == PIECE_WHITE_BISHOP ||
        piece == PIECE_WHITE_ROOK || piece == PIECE_WHITE_QUEEN || piece == PIECE_WHITE_KING)
    {
        moving_piece_is_white = 1;
    }
    else
    {
        moving_piece_is_white = 0;
    }

    if (state->is_white_turn && !moving_piece_is_white)
        return 0;
    if (!state->is_white_turn && moving_piece_is_white)
        return 0;

    PieceType target_piece = state->board[to.row][to.col];
    if (target_piece != PIECE_EMPTY)
    {
        int target_is_white = 0;
        if (target_piece == PIECE_WHITE_PAWN || target_piece == PIECE_WHITE_KNIGHT || target_piece == PIECE_WHITE_BISHOP ||
            target_piece == PIECE_WHITE_ROOK || target_piece == PIECE_WHITE_QUEEN || target_piece == PIECE_WHITE_KING)
        {
            target_is_white = 1;
        }
        else
        {
            target_is_white = 0;
        }
        if (target_is_white == moving_piece_is_white)
        {
            return 0;
        }
    }

    int delta_row = to.row - from.row;
    int delta_col = to.col - from.col;
    int abs_dr = delta_row < 0 ? -delta_row : delta_row;
    int abs_dc = delta_col < 0 ? -delta_col : delta_col;

    if (piece == PIECE_WHITE_PAWN || piece == PIECE_BLACK_PAWN)
    {
        int direction;
        if (piece == PIECE_WHITE_PAWN)
            direction = -1;
        else
            direction = 1;

        if (delta_col == 0)
        {
            if (delta_row == direction)
            {
                if (state->board[to.row][to.col] == PIECE_EMPTY)
                {
                    if (piece == PIECE_WHITE_PAWN && to.row == 0)
                    {
                        if (promotion_piece == PIECE_EMPTY)
                            return 0;
                    }
                    if (piece == PIECE_BLACK_PAWN && to.row == 7)
                    {
                        if (promotion_piece == PIECE_EMPTY)
                            return 0;
                    }
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
            if (piece == PIECE_WHITE_PAWN && from.row == 6 && delta_row == -2)
            {
                int intermediate_row = from.row - 1;
                if (state->board[intermediate_row][from.col] == PIECE_EMPTY && state->board[to.row][to.col] == PIECE_EMPTY)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
            if (piece == PIECE_BLACK_PAWN && from.row == 1 && delta_row == 2)
            {
                int intermediate_row = from.row + 1;
                if (state->board[intermediate_row][from.col] == PIECE_EMPTY && state->board[to.row][to.col] == PIECE_EMPTY)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
            return 0;
        }

        if (abs_dc == 1 && delta_row == direction)
        {
            if (state->board[to.row][to.col] != PIECE_EMPTY)
            {
                return 1;
            }
            if (state->en_passant_target.row != -1 &&
                to.row == state->en_passant_target.row &&
                to.col == state->en_passant_target.col)
            {
                return 1;
            }
            return 0;
        }

        return 0;
    }

    if (piece == PIECE_WHITE_KNIGHT || piece == PIECE_BLACK_KNIGHT)
    {
        if ((abs_dr == 2 && abs_dc == 1) || (abs_dr == 1 && abs_dc == 2))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    if (piece == PIECE_WHITE_KING || piece == PIECE_BLACK_KING)
    {
        if (abs_dr <= 1 && abs_dc <= 1)
        {
            return 1;
        }
        if (delta_row == 0 && abs_dc == 2)
        {
            if (piece == PIECE_WHITE_KING && from.row == 7 && from.col == 4)
            {
                if (delta_col == 2)
                {
                    int path_clear = 0;
                    if (state->white_can_castle_kingside &&
                        state->board[7][5] == PIECE_EMPTY && state->board[7][6] == PIECE_EMPTY &&
                        state->board[7][7] == PIECE_WHITE_ROOK)
                    {
                        path_clear = 1;
                    }
                    if (path_clear)
                        return 1;
                }
                if (delta_col == -2)
                {
                    int path_clear = 0;
                    if (state->white_can_castle_queenside &&
                        state->board[7][3] == PIECE_EMPTY && state->board[7][2] == PIECE_EMPTY && state->board[7][1] == PIECE_EMPTY &&
                        state->board[7][0] == PIECE_WHITE_ROOK)
                    {
                        path_clear = 1;
                    }
                    if (path_clear)
                        return 1;
                }
            }

            if (piece == PIECE_BLACK_KING && from.row == 0 && from.col == 4)
            {

                if (delta_col == 2)
                {
                    int path_clear = 0;
                    if (state->black_can_castle_kingside &&
                        state->board[0][5] == PIECE_EMPTY && state->board[0][6] == PIECE_EMPTY &&
                        state->board[0][7] == PIECE_BLACK_ROOK)
                    {
                        path_clear = 1;
                    }
                    if (path_clear)
                        return 1;
                }

                if (delta_col == -2)
                {
                    int path_clear = 0;
                    if (state->black_can_castle_queenside &&
                        state->board[0][3] == PIECE_EMPTY && state->board[0][2] == PIECE_EMPTY && state->board[0][1] == PIECE_EMPTY &&
                        state->board[0][0] == PIECE_BLACK_ROOK)
                    {
                        path_clear = 1;
                    }
                    if (path_clear)
                        return 1;
                }
            }
        }
        return 0;
    }

    if (piece == PIECE_WHITE_ROOK || piece == PIECE_BLACK_ROOK ||
        piece == PIECE_WHITE_BISHOP || piece == PIECE_BLACK_BISHOP ||
        piece == PIECE_WHITE_QUEEN || piece == PIECE_BLACK_QUEEN)
    {

        if (abs_dr == 0 && abs_dc == 0)
            return 0;

        int rook_like = 0;
        int bishop_like = 0;

        if (piece == PIECE_WHITE_ROOK || piece == PIECE_BLACK_ROOK ||
            piece == PIECE_WHITE_QUEEN || piece == PIECE_BLACK_QUEEN)
        {
            if (delta_row == 0 || delta_col == 0)
                rook_like = 1;
        }
        if (piece == PIECE_WHITE_BISHOP || piece == PIECE_BLACK_BISHOP ||
            piece == PIECE_WHITE_QUEEN || piece == PIECE_BLACK_QUEEN)
        {
            if (abs_dr == abs_dc)
                bishop_like = 1;
        }

        if (!rook_like && !bishop_like)
            return 0;

        int step_row = 0;
        if (delta_row > 0)
            step_row = 1;
        else if (delta_row < 0)
            step_row = -1;

        int step_col = 0;
        if (delta_col > 0)
            step_col = 1;
        else if (delta_col < 0)
            step_col = -1;

        int r = from.row + step_row;
        int c = from.col + step_col;
        while (r != to.row || c != to.col)
        {
            if (!position_in_bounds(r, c))
                return 0;
            if (state->board[r][c] != PIECE_EMPTY)
            {
                return 0;
            }
            r += step_row;
            c += step_col;
        }
        return 1;
    }

    return 0;
}

void perform_move_internal(GameState *state, BoardPosition from, BoardPosition to, MoveRecord *rec, PieceType promotion_piece)
{
    rec->from = from;
    rec->to = to;
    rec->moved_piece = state->board[from.row][from.col];
    rec->captured_piece = state->board[to.row][to.col];
    rec->prev_white_can_castle_kingside = state->white_can_castle_kingside;
    rec->prev_white_can_castle_queenside = state->white_can_castle_queenside;
    rec->prev_black_can_castle_kingside = state->black_can_castle_kingside;
    rec->prev_black_can_castle_queenside = state->black_can_castle_queenside;
    rec->prev_en_passant_target = state->en_passant_target;
    rec->prev_halfmove_clock = state->halfmove_clock;
    rec->prev_fullmove_number = state->fullmove_number;
    rec->promotion_piece = PIECE_EMPTY;

    PieceType piece = state->board[from.row][from.col];

    state->en_passant_target.row = -1;
    state->en_passant_target.col = -1;

    if (piece == PIECE_WHITE_PAWN || piece == PIECE_BLACK_PAWN)
    {
        state->halfmove_clock = 0;
    }
    else
    {
        if (state->board[to.row][to.col] != PIECE_EMPTY)
        {
            state->halfmove_clock = 0;
        }
        else
        {
            state->halfmove_clock = state->halfmove_clock + 1;
        }
    }

    if ((piece == PIECE_WHITE_PAWN || piece == PIECE_BLACK_PAWN) &&
        rec->prev_en_passant_target.row != -1 &&
        to.row == rec->prev_en_passant_target.row &&
        to.col == rec->prev_en_passant_target.col &&
        rec->captured_piece == PIECE_EMPTY)
    {

        if (piece == PIECE_WHITE_PAWN)
        {
            int captured_row = to.row + 1;
            int captured_col = to.col;
            rec->captured_piece = state->board[captured_row][captured_col];
            state->board[captured_row][captured_col] = PIECE_EMPTY;
        }
        else
        {
            int captured_row = to.row - 1;
            int captured_col = to.col;
            rec->captured_piece = state->board[captured_row][captured_col];
            state->board[captured_row][captured_col] = PIECE_EMPTY;
        }
    }

    state->board[to.row][to.col] = state->board[from.row][from.col];
    state->board[from.row][from.col] = PIECE_EMPTY;

    if (piece == PIECE_WHITE_PAWN && from.row == 6 && to.row == 4)
    {
        state->en_passant_target.row = 5;
        state->en_passant_target.col = from.col;
    }
    else if (piece == PIECE_BLACK_PAWN && from.row == 1 && to.row == 3)
    {
        state->en_passant_target.row = 2;
        state->en_passant_target.col = from.col;
    }

    if ((piece == PIECE_WHITE_KING || piece == PIECE_BLACK_KING) &&
        from.row == to.row && (to.col - from.col == 2 || to.col - from.col == -2))
    {

        if (to.col == 6)
        {
            int rook_from_col = 7;
            int rook_to_col = 5;
            state->board[to.row][rook_to_col] = state->board[to.row][rook_from_col];
            state->board[to.row][rook_from_col] = PIECE_EMPTY;
        }
        else if (to.col == 2)
        {
            int rook_from_col = 0;
            int rook_to_col = 3;
            state->board[to.row][rook_to_col] = state->board[to.row][rook_from_col];
            state->board[to.row][rook_from_col] = PIECE_EMPTY;
        }
    }

    if ((piece == PIECE_WHITE_PAWN && to.row == 0) || (piece == PIECE_BLACK_PAWN && to.row == 7))
    {
        PieceType prom = promotion_piece;
        if (prom == PIECE_EMPTY)
        {
            if (piece == PIECE_WHITE_PAWN)
                prom = PIECE_WHITE_QUEEN;
            else
                prom = PIECE_BLACK_QUEEN;
        }
        state->board[to.row][to.col] = prom;
        rec->promotion_piece = prom;
    }

    if (piece == PIECE_WHITE_KING)
    {
        state->white_can_castle_kingside = 0;
        state->white_can_castle_queenside = 0;
    }
    if (piece == PIECE_BLACK_KING)
    {
        state->black_can_castle_kingside = 0;
        state->black_can_castle_queenside = 0;
    }

    if (from.row == 7 && from.col == 0)
        state->white_can_castle_queenside = 0;
    if (from.row == 7 && from.col == 7)
        state->white_can_castle_kingside = 0;
    if (from.row == 0 && from.col == 0)
        state->black_can_castle_queenside = 0;
    if (from.row == 0 && from.col == 7)
        state->black_can_castle_kingside = 0;

    if (to.row == 7 && to.col == 0)
        state->white_can_castle_queenside = 0;
    if (to.row == 7 && to.col == 7)
        state->white_can_castle_kingside = 0;
    if (to.row == 0 && to.col == 0)
        state->black_can_castle_queenside = 0;
    if (to.row == 0 && to.col == 7)
        state->black_can_castle_kingside = 0;

    if (!state->is_white_turn)
    {
        state->fullmove_number = state->fullmove_number + 1;
    }
    state->is_white_turn = !state->is_white_turn;
}

int undo_last_move(GameState *state)
{
    if (move_history_length <= 0)
    {
        return 0;
    }
    MoveRecord last = move_history[--move_history_length];

    state->is_white_turn = !state->is_white_turn;
    state->fullmove_number = last.prev_fullmove_number;
    state->halfmove_clock = last.prev_halfmove_clock;

    state->board[last.from.row][last.from.col] = last.moved_piece;
    state->board[last.to.row][last.to.col] = last.captured_piece;

    if ((last.moved_piece == PIECE_WHITE_KING || last.moved_piece == PIECE_BLACK_KING) &&
        last.from.row == last.to.row && (last.to.col - last.from.col == 2 || last.to.col - last.from.col == -2))
    {

        if (last.to.col == 6)
        {
            int rook_src_col = 5;
            int rook_dest_col = 7;
            state->board[last.to.row][rook_dest_col] = state->board[last.to.row][rook_src_col];
            state->board[last.to.row][rook_src_col] = PIECE_EMPTY;
        }
        else if (last.to.col == 2)
        {
            int rook_src_col = 3;
            int rook_dest_col = 0;
            state->board[last.to.row][rook_dest_col] = state->board[last.to.row][rook_src_col];
            state->board[last.to.row][rook_src_col] = PIECE_EMPTY;
        }
    }

    if (last.promotion_piece != PIECE_EMPTY)
    {
        if (last.moved_piece == PIECE_WHITE_PAWN || (last.moved_piece >= PIECE_WHITE_KNIGHT && last.moved_piece <= PIECE_WHITE_KING))
        {

            state->board[last.from.row][last.from.col] = PIECE_WHITE_PAWN;
        }
        else
        {
            state->board[last.from.row][last.from.col] = PIECE_BLACK_PAWN;
        }
    }

    state->white_can_castle_kingside = last.prev_white_can_castle_kingside;
    state->white_can_castle_queenside = last.prev_white_can_castle_queenside;
    state->black_can_castle_kingside = last.prev_black_can_castle_kingside;
    state->black_can_castle_queenside = last.prev_black_can_castle_queenside;
    state->en_passant_target = last.prev_en_passant_target;

    return 1;
}

int make_move_if_legal(GameState *state, BoardPosition from, BoardPosition to, char promotion_char)
{
    PieceType promotion_piece = PIECE_EMPTY;
    if (promotion_char != 0)
    {
        int moving_white = 0;
        PieceType from_piece = state->board[from.row][from.col];
        if (from_piece == PIECE_WHITE_PAWN || (from_piece >= PIECE_WHITE_KNIGHT && from_piece <= PIECE_WHITE_KING))
        {
            moving_white = 1;
        }
        else
        {
            moving_white = 0;
        }
        char lower = tolower((unsigned char)promotion_char);
        if (lower == 'q')
        {
            promotion_piece = moving_white ? PIECE_WHITE_QUEEN : PIECE_BLACK_QUEEN;
        }
        else if (lower == 'r')
        {
            promotion_piece = moving_white ? PIECE_WHITE_ROOK : PIECE_BLACK_ROOK;
        }
        else if (lower == 'b')
        {
            promotion_piece = moving_white ? PIECE_WHITE_BISHOP : PIECE_BLACK_BISHOP;
        }
        else if (lower == 'n')
        {
            promotion_piece = moving_white ? PIECE_WHITE_KNIGHT : PIECE_BLACK_KNIGHT;
        }
        else
        {
            promotion_piece = PIECE_EMPTY;
        }
    }

    if (!is_pseudo_legal_move(state, from, to, promotion_piece))
    {
        return 0;
    }

    GameState simulated_state = *state;
    MoveRecord simulated_record;
    perform_move_internal(&simulated_state, from, to, &simulated_record, promotion_piece);
    PieceType moving_piece = state->board[from.row][from.col];
    int moved_was_white = 0;
    if (moving_piece == PIECE_WHITE_PAWN || moving_piece == PIECE_WHITE_KNIGHT || moving_piece == PIECE_WHITE_BISHOP ||
        moving_piece == PIECE_WHITE_ROOK || moving_piece == PIECE_WHITE_QUEEN || moving_piece == PIECE_WHITE_KING)
    {
        moved_was_white = 1;
    }
    else
    {
        moved_was_white = 0;
    }

    int side_in_check_after = is_in_check(&simulated_state, moved_was_white);
    if (side_in_check_after)
    {
        return 0;
    }

    MoveRecord real_record;
    perform_move_internal(state, from, to, &real_record, promotion_piece);
    if (move_history_length < MAX_MOVE_HISTORY)
    {
        move_history[move_history_length++] = real_record;
    }
    return 1;
}
int side_has_any_legal_move(const GameState *state, int white)
{
    for (int r = 0; r < BOARD_DIMENSION; ++r)
    {
        for (int c = 0; c < BOARD_DIMENSION; ++c)
        {
            PieceType piece = state->board[r][c];
            if (piece == PIECE_EMPTY)
                continue;

            int piece_is_white = 0;
            if (piece == PIECE_WHITE_PAWN || piece == PIECE_WHITE_KNIGHT || piece == PIECE_WHITE_BISHOP ||
                piece == PIECE_WHITE_ROOK || piece == PIECE_WHITE_QUEEN || piece == PIECE_WHITE_KING)
            {
                piece_is_white = 1;
            }
            else
            {
                piece_is_white = 0;
            }
            if (piece_is_white != white)
                continue;

            for (int tr = 0; tr < BOARD_DIMENSION; ++tr)
            {
                for (int tc = 0; tc < BOARD_DIMENSION; ++tc)
                {
                    BoardPosition from = {r, c};
                    BoardPosition to = {tr, tc};

                    if (!is_pseudo_legal_move(state, from, to, PIECE_EMPTY))
                    {
                        if (piece == PIECE_WHITE_PAWN && to.row == 0)
                        {
                            if (!is_pseudo_legal_move(state, from, to, PIECE_WHITE_QUEEN))
                                continue;
                        }
                        else if (piece == PIECE_BLACK_PAWN && to.row == 7)
                        {
                            if (!is_pseudo_legal_move(state, from, to, PIECE_BLACK_QUEEN))
                                continue;
                        }
                        else
                        {
                            continue;
                        }
                    }

                    GameState simulated = *state;
                    MoveRecord rec;
                    PieceType promotion_for_test = PIECE_EMPTY;
                    if (piece == PIECE_WHITE_PAWN && to.row == 0)
                        promotion_for_test = PIECE_WHITE_QUEEN;
                    if (piece == PIECE_BLACK_PAWN && to.row == 7)
                        promotion_for_test = PIECE_BLACK_QUEEN;

                    perform_move_internal(&simulated, from, to, &rec, promotion_for_test);

                    if (!is_in_check(&simulated, white))
                    {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

int parse_user_input(const char *input_line, BoardPosition *out_from, BoardPosition *out_to, char *out_promotion_char, char **out_cmd_arg, char *cmdbuf)
{
    *out_promotion_char = 0;
    *out_cmd_arg = NULL;

    char buffer[MAX_INPUT_LINE];
    strncpy(buffer, input_line, MAX_INPUT_LINE - 1);
    buffer[MAX_INPUT_LINE - 1] = '\0';

    char *s = buffer;
    while (*s && isspace((unsigned char)*s))
        s++;

    if (*s == '\0')
        return 0;

    if (strncmp(s, "resign", 6) == 0)
    {
        strcpy(cmdbuf, "resign");
        return -1;
    }
    if (strncmp(s, "undo", 4) == 0)
    {
        strcpy(cmdbuf, "undo");
        return -1;
    }
    if (strncmp(s, "help", 4) == 0)
    {
        strcpy(cmdbuf, "help");
        return -1;
    }
    if (strncmp(s, "exit", 4) == 0)
    {
        strcpy(cmdbuf, "exit");
        return -1;
    }
    if (strncmp(s, "save ", 5) == 0)
    {
        strcpy(cmdbuf, "save");
        *out_cmd_arg = s + 5;
        return -1;
    }
    if (strncmp(s, "load ", 5) == 0)
    {
        strcpy(cmdbuf, "load");
        *out_cmd_arg = s + 5;
        return -1;
    }

    char token1[16];
    char token2[16];
    int t1_idx = 0;
    int t2_idx = 0;

    while (*s && isspace((unsigned char)*s))
        s++;
    if (!isalpha((unsigned char)*s))
        return 0;
    token1[t1_idx++] = *s++;
    if (*s && isdigit((unsigned char)*s))
        token1[t1_idx++] = *s++;
    token1[t1_idx] = '\0';

    if (!algebraic_to_position(token1, out_from))
        return 0;

    while (*s && isspace((unsigned char)*s))
        s++;

    if (!*s)
        return 0;

    if (!isalpha((unsigned char)*s))
        return 0;
    token2[t2_idx++] = *s++;
    if (*s && isdigit((unsigned char)*s))
        token2[t2_idx++] = *s++;
    token2[t2_idx] = '\0';

    if (!algebraic_to_position(token2, out_to))
        return 0;

    while (*s && isspace((unsigned char)*s))
        s++;
    if (*s && isalpha((unsigned char)*s))
    {
        *out_promotion_char = tolower((unsigned char)*s);
    }

    return 1;
}

void save_move_history_to_file(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Unable to open %s for writing\n", filename);
        return;
    }
    for (int i = 0; i < move_history_length; ++i)
    {
        char from_alg[4];
        char to_alg[4];
        position_to_algebraic(move_history[i].from, from_alg);
        position_to_algebraic(move_history[i].to, to_alg);

        if (move_history[i].promotion_piece != PIECE_EMPTY)
        {
            char pchar = 'q';
            if (move_history[i].promotion_piece == PIECE_WHITE_QUEEN || move_history[i].promotion_piece == PIECE_BLACK_QUEEN)
                pchar = 'q';
            else if (move_history[i].promotion_piece == PIECE_WHITE_ROOK || move_history[i].promotion_piece == PIECE_BLACK_ROOK)
                pchar = 'r';
            else if (move_history[i].promotion_piece == PIECE_WHITE_BISHOP || move_history[i].promotion_piece == PIECE_BLACK_BISHOP)
                pchar = 'b';
            else if (move_history[i].promotion_piece == PIECE_WHITE_KNIGHT || move_history[i].promotion_piece == PIECE_BLACK_KNIGHT)
                pchar = 'n';
            fprintf(file, "%s%s%c\n", from_alg, to_alg, pchar);
        }
        else
        {
            fprintf(file, "%s%s\n", from_alg, to_alg);
        }
    }
    fclose(file);
    printf("Saved %d moves to %s\n", move_history_length, filename);
}

int load_move_history_from_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Unable to open %s for reading\n", filename);
        return 0;
    }

    initialize_game_state(&current_game_state);
    move_history_length = 0;

    char line[MAX_INPUT_LINE];
    int line_no = 0;
    while (fgets(line, sizeof(line), file))
    {
        line_no++;
        char *newline_char = strchr(line, '\n');
        if (newline_char)
            *newline_char = '\0';

        BoardPosition from, to;
        char promotion = 0;
        char *cmd_arg = NULL;
        char cmdbuf[64];
        char buffer_copy[64];
        snprintf(buffer_copy, sizeof(buffer_copy), "%s", line);

        int parsed = parse_user_input(buffer_copy, &from, &to, &promotion, &cmd_arg, cmdbuf);
        if (parsed != 1)
        {
            printf("Invalid move in file at line %d: %s\n", line_no, line);
            fclose(file);
            return 0;
        }
        int ok = make_move_if_legal(&current_game_state, from, to, promotion);
        if (!ok)
        {
            printf("Illegal move in file at line %d: %s\n", line_no, line);
            fclose(file);
            return 0;
        }
    }
    fclose(file);
    printf("Loaded %s with %d moves\n", filename, move_history_length);
    return 1;
}

void print_help_text(void)
{
    printf("\nCommands available:\n");
    printf(" - Standard move: 'e2 e4' or 'e7e8q' (promotion: q/r/b/n)\n");
    printf(" - save <filename>\n");
    printf(" - load <filename>\n");
    printf(" - undo\n");
    printf(" - resign\n");
    printf(" - help\n");
    printf(" - exit\n");
}

void print_game_status_verbose(GameState *state)
{
    int side_to_move = state->is_white_turn ? 1 : 0;
    int in_check_flag = is_in_check(state, side_to_move);
    int any_legal = side_has_any_legal_move(state, side_to_move);

    if (in_check_flag && !any_legal)
    {
        if (side_to_move)
            printf("White is checkmated.\n");
        else
            printf("Black is checkmated.\n");
    }
    else if (!in_check_flag && !any_legal)
    {
        printf("Stalemate.\n");
    }
    else if (in_check_flag)
    {
        if (side_to_move)
            printf("White is in check.\n");
        else
            printf("Black is in check.\n");
    }
    else
    {
        printf("No check.\n");
    }
}

int main(void)
{
    initialize_game_state(&current_game_state);
    move_history_length = 0;

    char input_line[MAX_INPUT_LINE];

    printf("Console Chess (expanded/verbose C version)\n");
    printf("Type 'help' for commands.\n");
    print_board_state(&current_game_state);
    print_help_text();

    while (1)
    {
        if (current_game_state.is_white_turn)
            printf("\nWhite to move > ");
        else
            printf("\nBlack to move > ");

        if (!fgets(input_line, sizeof(input_line), stdin))
        {
            break;
        }

        char *nl = strchr(input_line, '\n');
        if (nl)
            *nl = '\0';

        BoardPosition from, to;
        char promotion_char = 0;
        char *command_argument = NULL;
        char command_buffer[64];

        int parse_result = parse_user_input(input_line, &from, &to, &promotion_char, &command_argument, command_buffer);

        if (parse_result == 0)
        {
            printf("Could not parse input. Type 'help' for usage.\n");
            continue;
        }

        if (parse_result == -1)
        {
            if (strcmp(command_buffer, "resign") == 0)
            {
                if (current_game_state.is_white_turn)
                    printf("White resigns. Game over.\n");
                else
                    printf("Black resigns. Game over.\n");
                break;
            }
            if (strcmp(command_buffer, "help") == 0)
            {
                print_help_text();
                continue;
            }
            if (strcmp(command_buffer, "exit") == 0)
            {
                printf("Exiting.\n");
                break;
            }
            if (strcmp(command_buffer, "undo") == 0)
            {
                if (undo_last_move(&current_game_state))
                {
                    printf("Undo performed.\n");
                    print_board_state(&current_game_state);
                }
                else
                {
                    printf("Nothing to undo.\n");
                }
                continue;
            }
            if (strcmp(command_buffer, "save") == 0)
            {
                if (command_argument == NULL)
                {
                    printf("Usage: save <filename>\n");
                    continue;
                }
                while (*command_argument && isspace((unsigned char)*command_argument))
                    command_argument++;
                save_move_history_to_file(command_argument);
                continue;
            }
            if (strcmp(command_buffer, "load") == 0)
            {
                if (command_argument == NULL)
                {
                    printf("Usage: load <filename>\n");
                    continue;
                }
                while (*command_argument && isspace((unsigned char)*command_argument))
                    command_argument++;
                if (load_move_history_from_file(command_argument))
                {
                    print_board_state(&current_game_state);
                }
                continue;
            }
            continue;
        }

        int ok = make_move_if_legal(&current_game_state, from, to, promotion_char);
        if (!ok)
        {
            printf("Illegal move.\n");
            continue;
        }
        else
        {
            print_board_state(&current_game_state);
            print_game_status_verbose(&current_game_state);
        }
    }

    return 0;
}
#include <iostream>
#include <coroutine>
#include <vector>
#include <random>
#include <optional>

#ifdef _WIN32
#include <windows.h>
#endif

struct Move {
    int row;
    int col;
    char symbol;
};

class TicTacToeGame {
public:
    struct promise_type {
        Move current_move;
        
        TicTacToeGame get_return_object() {
            return TicTacToeGame{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        
        std::suspend_always yield_value(Move move) {
            current_move = move;
            return {};
        }
        
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    
    using handle_type = std::coroutine_handle<promise_type>;
    
    TicTacToeGame(handle_type h) : coro_handle(h) {}
    ~TicTacToeGame() { if (coro_handle) coro_handle.destroy(); }
    
    TicTacToeGame(const TicTacToeGame&) = delete;
    TicTacToeGame& operator=(const TicTacToeGame&) = delete;
    
    TicTacToeGame(TicTacToeGame&& other) : coro_handle(other.coro_handle) {
        other.coro_handle = nullptr;
    }
    
    std::optional<Move> next() {
        if (!coro_handle || coro_handle.done()) {
            return std::nullopt;
        }
        coro_handle.resume();
        if (coro_handle.done()) {
            return std::nullopt;
        }
        return coro_handle.promise().current_move;
    }
    
private:
    handle_type coro_handle;
};

class Board {
public:
    Board() : grid(3, std::vector<char>(3, ' ')) {}
    
    bool is_valid_move(int row, int col) const {
        return row >= 0 && row < 3 && col >= 0 && col < 3 && grid[row][col] == ' ';
    }
    
    void make_move(int row, int col, char symbol) {
        grid[row][col] = symbol;
    }
    
    char check_winner() const {
        for (int i = 0; i < 3; i++) {
            if (grid[i][0] != ' ' && grid[i][0] == grid[i][1] && grid[i][1] == grid[i][2]) {
                return grid[i][0];
            }
        }
        
        for (int j = 0; j < 3; j++) {
            if (grid[0][j] != ' ' && grid[0][j] == grid[1][j] && grid[1][j] == grid[2][j]) {
                return grid[0][j];
            }
        }
        
        if (grid[0][0] != ' ' && grid[0][0] == grid[1][1] && grid[1][1] == grid[2][2]) {
            return grid[0][0];
        }
        if (grid[0][2] != ' ' && grid[0][2] == grid[1][1] && grid[1][1] == grid[2][0]) {
            return grid[0][2];
        }
        
        return ' ';
    }
    
    bool is_full() const {
        for (const auto& row : grid) {
            for (char cell : row) {
                if (cell == ' ') return false;
            }
        }
        return true;
    }
    
    void print() const {
        std::cout << "\n";
        for (int i = 0; i < 3; i++) {
            std::cout << " ";
            for (int j = 0; j < 3; j++) {
                std::cout << grid[i][j];
                if (j < 2) std::cout << " | ";
            }
            std::cout << "\n";
            if (i < 2) std::cout << "---|---|---\n";
        }
        std::cout << "\n";
    }
    
    const std::vector<std::vector<char>>& get_grid() const { return grid; }
    
private:
    std::vector<std::vector<char>> grid;
};

TicTacToeGame player_X(Board& board) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 2);
    
    while (true) {
        int row, col;
        do {
            row = dis(gen);
            col = dis(gen);
        } while (!board.is_valid_move(row, col));
        
        co_yield Move{row, col, 'X'};
    }
}

TicTacToeGame player_O(Board& board) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 2);
    
    while (true) {
        int row, col;
        do {
            row = dis(gen);
            col = dis(gen);
        } while (!board.is_valid_move(row, col));
        
        co_yield Move{row, col, 'O'};
    }
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    
    std::cout << "=== Tic-Tac-Toe Game ===\n";
    std::cout << "Two coroutines playing against each other\n\n";
    
    Board board;
    auto coroutine_X = player_X(board);
    auto coroutine_O = player_O(board);
    
    board.print();
    
    int move_count = 0;
    char current_player = 'X';
    
    while (true) {
        auto move_opt = (current_player == 'X') ? coroutine_X.next() : coroutine_O.next();
        
        if (!move_opt) break;
        
        Move move = *move_opt;
        board.make_move(move.row, move.col, move.symbol);
        move_count++;
        
        std::cout << "Move " << move_count << ": Player " << move.symbol 
                  << " moves to position (" << move.row + 1 << ", " << move.col + 1 << ")\n";
        board.print();
        
        char winner = board.check_winner();
        if (winner != ' ') {
            std::cout << "Winner: Player " << winner << "!\n\n";
            return 0;
        }
        
        if (board.is_full()) {
            std::cout << "Game ended in a draw!\n\n";
            return 0;
        }
        
        current_player = (current_player == 'X') ? 'O' : 'X';
    }
    
    return 0;
}

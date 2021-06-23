#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <climits>

#define DEPTH 5

struct Point
{
    int x, y;
    Point() : Point(0, 0) {}
    Point(float x, float y) : x(x), y(y) {}
    Point operator+(const Point &rhs) const
    {
        return Point(x + rhs.x, y + rhs.y);
    }
};

int Player;
const int SIZE = 8;
const std::array<Point, 8> directions{{Point(-1, -1), Point(-1, 0), Point(-1, 1),
                                       Point(0, -1), /*{0, 0}, */ Point(0, 1),
                                       Point(1, -1), Point(1, 0), Point(1, 1)}};
std::array<std::array<int, SIZE>, SIZE> Board;
std::vector<Point> Next_Valid_Spots;
enum SPOT_STATE
{
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2
};

class State
{
public:
    std::array<std::array<int, SIZE>, SIZE> board;
    std::vector<Point> next_valid_spots;
    std::array<int, 3> disc_count;
    int cur_player;

private:
    int get_next_player(int player) const
    {
        return 3 - player;
    }
    bool is_spot_on_board(Point p) const
    {
        return 0 <= p.x && p.x < SIZE && 0 <= p.y && p.y < SIZE;
    }
    int get_disc(Point p) const
    {
        return board[p.x][p.y];
    }
    void set_disc(Point p, int disc)
    {
        board[p.x][p.y] = disc;
    }
    bool is_disc_at(Point p, int disc) const
    {
        if (!is_spot_on_board(p))
            return false;
        if (get_disc(p) != disc)
            return false;
        return true;
    }
    bool is_spot_valid(Point center) const
    {
        if (get_disc(center) != EMPTY)
            return false;
        for (Point dir : directions)
        {
            // Move along the direction while testing.
            Point p = center + dir;
            if (!is_disc_at(p, get_next_player(cur_player)))
                continue;
            p = p + dir;
            while (is_spot_on_board(p) && get_disc(p) != EMPTY)
            {
                if (is_disc_at(p, cur_player))
                    return true;
                p = p + dir;
            }
        }
        return false;
    }
    void flip_discs(Point center)
    {
        for (Point dir : directions)
        {
            // Move along the direction while testing.
            Point p = center + dir;
            if (!is_disc_at(p, get_next_player(cur_player)))
                continue;
            std::vector<Point> discs({p});
            p = p + dir;
            while (is_spot_on_board(p) && get_disc(p) != EMPTY)
            {
                if (is_disc_at(p, cur_player))
                {
                    for (Point s : discs)
                    {
                        set_disc(s, cur_player);
                    }
                    disc_count[cur_player] += discs.size();
                    disc_count[get_next_player(cur_player)] -= discs.size();
                    break;
                }
                discs.push_back(p);
                p = p + dir;
            }
        }
    }

public:
    State()
        : cur_player(Player)
    {
        int E = 0, B = 0, W = 0;
        for (int i = 0; i < SIZE; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                board[i][j] = Board[i][j];
                switch (board[i][j])
                {
                case EMPTY:
                    E++;
                    break;
                case BLACK:
                    B++;
                    break;
                case WHITE:
                    W++;
                    break;
                }
            }
        }
        disc_count[EMPTY] = E;
        disc_count[BLACK] = B;
        disc_count[WHITE] = W;

        int size = Next_Valid_Spots.size();
        next_valid_spots.resize(size);
        for (int i = 0; i < size; i++)
        {
            next_valid_spots[i] = Next_Valid_Spots[i];
        }
    }
    State(const State &rhs)
        : cur_player(rhs.cur_player)
    {
        for (int i = 0; i < SIZE; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                board[i][j] = rhs.board[i][j];
            }
        }
        disc_count[EMPTY] = rhs.disc_count[EMPTY];
        disc_count[BLACK] = rhs.disc_count[BLACK];
        disc_count[WHITE] = rhs.disc_count[WHITE];

        int size = rhs.next_valid_spots.size();
        next_valid_spots.resize(size);
        for (int i = 0; i < size; i++)
        {
            next_valid_spots[i] = rhs.next_valid_spots[i];
        }
    }
    std::vector<Point> get_valid_spots() const
    {
        std::vector<Point> valid_spots;
        for (int i = 0; i < SIZE; i++)
        {
            for (int j = 0; j < SIZE; j++)
            {
                Point p = Point(i, j);
                if (board[i][j] != EMPTY)
                    continue;
                if (is_spot_valid(p))
                    valid_spots.push_back(p);
            }
        }
        return valid_spots;
    }
    bool put_disc(Point p)
    {
        set_disc(p, cur_player);
        disc_count[cur_player]++;
        disc_count[EMPTY]--;
        flip_discs(p);
        // Give control to the other player.
        cur_player = get_next_player(cur_player);
        next_valid_spots = get_valid_spots();
        return true;
    }
};

int value_function(const State &curState, int depth, int alpha, int beta, bool maximize_player)
{
    if (curState.disc_count[EMPTY] == 0)
    {
        if (curState.disc_count[Player] > curState.disc_count[3 - Player])
            return INT_MAX;
        else if (curState.disc_count[Player] < curState.disc_count[3 - Player])
            return INT_MIN;
        else
            return 0;
    }
    else if (depth == 0)
    {
        return curState.disc_count[Player] - curState.disc_count[3 - Player];
    }
    if (maximize_player)
    {
        int value = INT_MIN;
        for (Point p : curState.next_valid_spots)
        {
            State newState = curState;
            newState.put_disc(p);
            value = std::max(value, value_function(newState, depth - 1, alpha, beta, false));
            alpha = std::max(alpha, value);
            if (alpha >= beta)
                break;
        }
        return value;
    }
    else
    {
        int value = INT_MAX;
        for (Point p : curState.next_valid_spots)
        {
            State newState = curState;
            newState.put_disc(p);
            value = std::min(value, value_function(newState, depth - 1, alpha, beta, true));
            beta = std::min(beta, value);
            if (beta <= alpha)
                break;
        }
        return value;
    }
}

int minmax_function(const State &curState, int depth, bool maximize_player)
{
    if (curState.disc_count[EMPTY] == 0)
    {
        if (curState.disc_count[Player] > curState.disc_count[3 - Player])
            return INT_MAX;
        else if (curState.disc_count[Player] < curState.disc_count[3 - Player])
            return INT_MIN;
        else
            return 0;
    }
    else if (depth == 0)
    {
        return curState.disc_count[Player] - curState.disc_count[3 - Player];
    }
    if (maximize_player)
    {
        int value = INT_MIN;
        for (Point p : curState.next_valid_spots)
        {
            State newState = curState;
            newState.put_disc(p);
            value = std::max(value, minmax_function(newState, depth-1, false));
        }
        return value;
    }
    else
    {
        int value = INT_MAX;
        for (Point p : curState.next_valid_spots)
        {
            State newState = curState;
            newState.put_disc(p);
            value = std::min(value, minmax_function(newState, depth-1, true));
        }
        return value;
    }
}

void read_board(std::ifstream &fin)
{
    fin >> Player;
    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            fin >> Board[i][j];
        }
    }
}

void read_valid_spots(std::ifstream &fin)
{
    int n_valid_spots;
    fin >> n_valid_spots;
    int x, y;
    for (int i = 0; i < n_valid_spots; i++)
    {
        fin >> x >> y;
        Next_Valid_Spots.push_back(Point(x, y));
    }
}

void write_valid_spot(std::ofstream &fout)
{
    State initState;
    int value = INT_MIN;
    fout << Next_Valid_Spots[0].x << " " << Next_Valid_Spots[0].y << std::endl;
    fout.flush();
    for (Point p : initState.next_valid_spots)
    {
        State newState = initState;
        newState.put_disc(p);
        int new_value = value_function(newState, DEPTH - 1, value, INT_MAX, false);
        if (new_value > value)
        {
            value = new_value;
            fout << p.x << " " << p.y << std::endl;
            fout.flush();
        }
    }
}

int main(int, char **argv)
{
    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);
    read_board(fin);
    read_valid_spots(fin);
    write_valid_spot(fout);
    fin.close();
    fout.close();
    return 0;
}
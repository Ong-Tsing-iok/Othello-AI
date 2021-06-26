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
enum SPOT_STATE
{
    EMPTY = 0,
    BLACK = 1,
    WHITE = 2
};
enum TABLE_SCORE
{
    C = 10,
    X = -10,
    N = -5,
    E = 1,
    M = 0
};
// heuristic score
const int CORNER = 100;
const int XSPOT = -100;
const int CSPOT = -50;
const int MOBILITY = 10;
const int POTENTIAL_MOBILITY = 5;
const int FRONTIER = -5;
const int DISC = 1;

int Player, Opponent;
const int SIZE = 8;
const std::array<Point, 8> directions{{Point(-1, -1), Point(-1, 0), Point(-1, 1),
                                       Point(0, -1), /*{0, 0}, */ Point(0, 1),
                                       Point(1, -1), Point(1, 0), Point(1, 1)}};
const std::array<Point, 4> corners{{Point(0, 0), Point(0, SIZE - 1), Point(SIZE - 1, 0), Point(SIZE - 1, SIZE - 1)}};
const std::array<Point, 4> xspots{{Point(1, 1), Point(1, SIZE - 2), Point(SIZE - 2, 1), Point(SIZE - 2, SIZE - 2)}};
const std::array<std::array<Point, 2>, 4> cspots{{{{Point(0, 1), Point(1, 0)}},
                                                  {{Point(0, SIZE - 2), Point(1, SIZE - 1)}},
                                                  {{Point(SIZE - 2, 0), Point(SIZE - 1, 1)}},
                                                  {{Point(SIZE - 2, SIZE - 1), Point(SIZE - 1, SIZE - 2)}}}};
std::array<std::array<int, SIZE>, SIZE> score_table{{{{C, N, E, E, E, E, N, C}},
                                                     {{N, X, M, M, M, M, X, N}},
                                                     {{E, M, M, M, M, M, M, E}},
                                                     {{E, M, M, M, M, M, M, E}},
                                                     {{E, M, M, M, M, M, M, E}},
                                                     {{E, M, M, M, M, M, M, E}},
                                                     {{N, X, M, M, M, M, X, N}},
                                                     {{C, N, E, E, E, E, N, C}}}};
std::array<std::array<int, SIZE>, SIZE> Board;
std::vector<Point> Next_Valid_Spots;

class State
{
public:
    std::array<std::array<int, SIZE>, SIZE> board;
    std::vector<Point> next_valid_spots;
    std::array<int, 3> disc_count;
    int cur_player;

public:
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

        next_valid_spots = Next_Valid_Spots;
        std::sort(next_valid_spots.begin(), next_valid_spots.end(), [](Point a, Point b)
                  { return score_table[a.x][a.y] > score_table[b.x][b.y]; });
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

        next_valid_spots = rhs.next_valid_spots;

        // int size = rhs.next_valid_spots.size();
        // next_valid_spots.resize(size);
        // for (int i = 0; i < size; i++)
        // {
        //     next_valid_spots[i] = rhs.next_valid_spots[i];
        // }
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
        std::sort(valid_spots.begin(), valid_spots.end(), [](Point a, Point b)
                  { return score_table[a.x][a.y] > score_table[b.x][b.y]; });
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
    bool pass()
    {
        // Give control to the other player.
        cur_player = get_next_player(cur_player);
        next_valid_spots = get_valid_spots();
        return true;
    }
};

int disc_count_heuristic(const State &curState)
{
    return curState.disc_count[Player] - curState.disc_count[Opponent];
}

int heuristic(const State &curState)
{
    int h = 0;
    // corners
    for (int i = 0; i < 4; i++)
    {
        if (curState.board[corners[i].x][corners[i].y] == Player)
        {
            h += CORNER;
        }
        else if (curState.board[corners[i].x][corners[i].y] == Opponent)
        {
            h -= CORNER;
        }
        else
        {
            // xspot
            if (curState.board[xspots[i].x][xspots[i].y] == Player)
            {
                h += XSPOT;
            }
            else if (curState.board[xspots[i].x][xspots[i].y] == Opponent)
            {
                h -= XSPOT;
            }
            // cspot
            for (int j = 0; j < 2; j++)
            {
                if (curState.board[cspots[i][j].x][cspots[i][j].y] == Player)
                {
                    h += CSPOT;
                }
                else if (curState.board[cspots[i][j].x][cspots[i][j].y] == Opponent)
                {
                    h -= CSPOT;
                }
            }
        }
    }

    // mobility
    if (curState.cur_player == Player)
    {
        h += curState.next_valid_spots.size() * MOBILITY;
        State nextState = curState;
        nextState.pass();
        h -= curState.next_valid_spots.size() * MOBILITY;
    }
    else
    {
        h -= curState.next_valid_spots.size() * MOBILITY;
        State nextState = curState;
        nextState.pass();
        h += curState.next_valid_spots.size() * MOBILITY;
    }

    // potential mobility
    // for (int i = 0; i < SIZE; i++)
    // {
    //     for (int j = 0; j < SIZE; j++)
    //     {
    //         if (curState.board[i][j] == EMPTY)
    //         {
    //             Point p(i, j);
    //             for (int k = 0; k < 8; k++)
    //             {
    //                 if (curState.is_disc_at(p + directions[k], Opponent))
    //                 {
    //                     h += POTENTIAL_MOBILITY;
    //                     break;
    //                 }
    //             }
    //             for (int k = 0; k < 8; k++)
    //             {
    //                 if (curState.is_disc_at(p + directions[k], Player))
    //                 {
    //                     h -= POTENTIAL_MOBILITY;
    //                     break;
    //                 }
    //             }
    //         }
    //     }
    // }
    // frontier
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (curState.board[i][j] != EMPTY)
            {
                Point p(i, j);
                for (int k = 0; k < 8; k++)
                {
                    if (curState.is_disc_at(p + directions[k], EMPTY))
                    {
                        if (curState.board[i][j] == Player)
                            h += FRONTIER;
                        else
                            h -= FRONTIER;
                        break;
                    }
                }
            }
        }
    }
    // disc
    h += (curState.disc_count[Player] - curState.disc_count[Opponent]) * DISC;
    return h;
}

int gameEnd(const State &curState)
{
    if (curState.disc_count[Player] > curState.disc_count[Opponent])
        return INT_MAX;
    else if (curState.disc_count[Player] < curState.disc_count[Opponent])
        return INT_MIN;
    else
        return 0;
}

int value_function(const State &curState, int depth, int alpha, int beta, bool maximize_player, bool passed = false)
{
    if (curState.disc_count[EMPTY] == 0)
    {
        return gameEnd(curState);
    }
    // else if (curState.disc_count[Player] == 0)
    // {
    //     return INT_MIN;
    // }
    // else if (curState.disc_count[Opponent] == 0)
    // {
    //     return INT_MAX;
    // }
    else if (depth == 0)
    {
        return heuristic(curState);
    }

    if (maximize_player)
    {
        int value = INT_MIN;
        if (curState.next_valid_spots.size() == 0)
        {
            if (passed)
                return gameEnd(curState);

            State newState = curState;
            newState.pass();
            value = std::max(value, value_function(newState, depth, alpha, beta, false, true));
        }
        else if (curState.next_valid_spots.size() == 1)
        {
            State newState = curState;
            newState.put_disc(curState.next_valid_spots.front());
            value = std::max(value, value_function(newState, depth, alpha, beta, false));
            alpha = std::max(alpha, value);
        }
        else
        {
            for (Point p : curState.next_valid_spots)
            {
                State newState = curState;
                newState.put_disc(p);
                // corner move
                if ((p.x == 0 || p.x == SIZE - 1) && (p.y == 0 || p.y == SIZE - 1))
                {
                    value = std::max(value, value_function(newState, depth, alpha, beta, false));
                }
                else
                {
                    value = std::max(value, value_function(newState, depth - 1, alpha, beta, false));
                }
                alpha = std::max(alpha, value);
                if (alpha >= beta)
                    break;
            }
        }
        return value;
    }
    else
    {
        int value = INT_MAX;
        if (curState.next_valid_spots.size() == 0)
        {
            if (passed)
                return gameEnd(curState);

            State newState = curState;
            newState.pass();
            value = std::min(value, value_function(newState, depth, alpha, beta, true));
        }
        else if (curState.next_valid_spots.size() == 1)
        {
            State newState = curState;
            newState.put_disc(curState.next_valid_spots.front());
            value = std::min(value, value_function(newState, depth, alpha, beta, true));
            beta = std::min(beta, value);
        }
        else
        {
            for (Point p : curState.next_valid_spots)
            {
                State newState = curState;
                newState.put_disc(p);
                // corner move
                if ((p.x == 0 || p.x == SIZE - 1) && (p.y == 0 || p.y == SIZE - 1))
                {
                    value = std::min(value, value_function(newState, depth, alpha, beta, true));
                }
                else
                {
                    value = std::min(value, value_function(newState, depth - 1, alpha, beta, true));
                }
                beta = std::min(beta, value);
                if (beta <= alpha)
                    break;
            }
        }
        return value;
    }
}

int minmax_function(const State &curState, int depth, bool minimize_opponent)
{
    if (curState.disc_count[EMPTY] == 0)
    {
        if (curState.disc_count[Player] > curState.disc_count[Opponent])
            return INT_MAX;
        else if (curState.disc_count[Player] < curState.disc_count[Opponent])
            return INT_MIN;
        else
            return 0;
    }
    else if (depth == 0)
    {
        return curState.next_valid_spots.size();
    }
    if (minimize_opponent)
    {
        int value = INT_MAX;
        for (Point p : curState.next_valid_spots)
        {
            State newState = curState;
            newState.put_disc(p);
            value = std::min(value, minmax_function(newState, depth - 1, false));
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
            value = std::min(value, minmax_function(newState, depth - 1, true));
        }
        return value;
    }
}

void read_board(std::ifstream &fin)
{
    fin >> Player;
    Opponent = 3 - Player;
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
    if (!initState.next_valid_spots.empty())
    {
        fout << initState.next_valid_spots.front().x << " " << initState.next_valid_spots.front().y << std::endl;
        fout.flush();
    }
    for (Point p : initState.next_valid_spots)
    {
        // if ((p.x == 0 || p.x == SIZE - 1) && (p.y == 0 || p.y == SIZE - 1))
        // {
        //     fout << p.x << " " << p.y << std::endl;
        //     fout.flush();
        //     break;
        // }
        State newState = initState;
        newState.put_disc(p);
        int new_value = value_function(newState, DEPTH - 1, value, INT_MAX, false);
        //int new_value = minmax_function(newState, DEPTH - 1, false);
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
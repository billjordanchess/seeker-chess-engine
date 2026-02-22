using namespace std;

#include "globals.h"

void xboard()
{
    int computer_side;
    int m;
    int post = 0;
    int analyze = 0;

    string line;
    string command;

    signal(SIGINT, SIG_IGN);
    NewGame();
    computer_side = EMPTY;
    LoadWeights();

    // main loop
    while (true)
    {
        // engine to move
        if (side == computer_side)
        {
            if (fixed_time2 == 1)
                think2();
            else
                think2();

            SetMaterial();
            Gen();

            move_list[WHITE].from = move_start;
            move_list[WHITE].to = move_dest;
            cout << "move " << MoveString(move_start, move_dest, 0) << endl;

            MakeMove(move_start, move_dest);

            ply = 0;
            Gen();
            if (XboardResult() > 0)
            {
                NewGame();
                computer_side = EMPTY;
            }
            continue;
        }

        // read one line from GUI
        if (!std::getline(cin, line))
            return;         // EOF or error

        if (line.empty())
            continue;

        // first token = command
        std::istringstream text(line);
        text >> command;

        // --- protocol commands ---

        if (command == "xboard")
        {
            // nothing special to do here
            continue;
        }

        if (command == "protover")
        {
            // advertise features
            cout << "feature usermove=0 ping=1 setboard=0 colors=1 sigint=0 san=0 done=1" << endl;
            continue;
        }

        if (command == "new")
        {
            NewGame();
            computer_side = EMPTY;
            continue;
        }

        if (command == "quit")
        {
            return;
        }

        if (command == "force")
        {
            computer_side = EMPTY;
            continue;
        }

        if (command == "white")
        {
            side = 0;
            xside = 1;
            Gen();
            computer_side = EMPTY;
            continue;
        }

        if (command == "black")
        {
            side = 1;
            xside = 0;
            Gen();
            computer_side = EMPTY;
            continue;
        }

        if (command == "sd")
        {
            int depth;
            if (text >> depth)
            {
                max_depth2 = depth;
                fixed_depth2 = 1;
            }
            continue;
        }
        if (command == "st")
        {
            if (text >> max_time2)
            {
                max_time2 *= 1000;
                max_depth2 = MAX_PLY;
                fixed_time2 = 1;
                fixed_depth2 = 0;
                continue;
            }
        }

        if (command == "time")
        {
            int cs = 0;
            if (text >> cs)
            {
                int ms = cs * 10;
                max_time2 = ms / 30;
                if (max_time2 < 50)
                    max_time2 = 50;
                fixed_time2 = 0;
                max_depth2 = MAX_PLY;
            }
            continue;
        }

        if (command == "otim")
        {
            continue;
        }

        if (command == "go")
        {
            computer_side = side;
            continue;
        }

        if (command == "random" ||
            command == "level" ||
            command == "hard" ||
            command == "easy")
        {
            continue;
        }

        if (command == "undo")
        {
            if (!hply)
                continue;
            UnMakeMove();
            ply = 0;
            Gen();
            continue;
        }

        if (command == "remove")
        {
            if (hply < 2)
                continue;
            UnMakeMove();
            UnMakeMove();
            ply = 0;
            Gen();
            continue;
        }

        // --- assume it's a move like "e2e4" ---

        first_move[0] = 0;
        Gen();

        m = ParseMove(command);

        if (m == -1 || !MakeMove(move_list[m].from, move_list[m].to))
        {
            cerr << "Error (unknown command): " << command << endl;
            continue;
        }

        ply = 0;
        Gen();
        if (XboardResult() > 0)
        {
            NewGame();
            computer_side = EMPTY;
        }
        else if (computer_side == EMPTY)
        {
            computer_side = side;
        }
    }
}
/*

<b>XboardResult()</b> is called from <b>xboard</b> and is similar to <b>CheckResult()</b>.

*/
int XboardResult()
{
    int i;
    int flag = 0;

    SetMaterial();
    if (pawn_mat[WHITE] == 0 && pawn_mat[BLACK] == 0 && piece_mat[WHITE] <= 300 && piece_mat[BLACK] <= 300)
    {
        cout << "1/2-1/2 {Material}" << endl;
        return 1;
    }
    ply = 0;

    Gen();
    for (i = 0; i < first_move[1]; ++i)
        if (MakeMove(move_list[i].from, move_list[i].to))
        {
            UnMakeMove();
            flag = 1;
            break;
        }
    if (i == first_move[1] && flag == 0)
    {
        if (Attack(xside, kingloc[side]))
        {
            if (side == 0)
            {
                cout << "0-1 {BLACK mates}" << endl;
            }
            else
            {
                cout << "1-0 {WHITE mates}" << endl;
            }
        }
        else
        {
            cout << "1/2-1/2 {Stalemate}" << endl;
        }
        return 1;
    }
    if (Reps() >= 3)
    {
        cout << "1/2-1/2 {Draw by repetition}" << endl;
        return 1;
    }
    else if (fifty >= 100)
    {
        cout << "1/2-1/2 {Draw by fifty move rule}" << endl;
        return 1;
    }
    return 0;
}
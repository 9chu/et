/**
 * @file
 * @author chu
 * @date 18/1/8
 */
#include <et.hpp>
#include <et/Base.hpp>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iterator>

using namespace std;

int main(int argc, const char* argv[])
{
    lua_State* L = nullptr;

    // parse args
    int paramIndex = INT_MAX;
    const char* path = nullptr;
    const char* output = nullptr;

    for (int i = 1, state = 0; i < argc; ++i)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
            goto ShowUsage;
        else if (strcmp(argv[i], "--") == 0)
        {
            paramIndex = i + 1;
            break;
        }

        switch (state)
        {
            case 0:
                if (strcmp(argv[i], "--stdin") == 0 || strcmp(argv[i], "-i") == 0)
                {
                    state = 1;
                    break;
                }
                else if (strncmp(argv[i], "--", 2) == 0 || strncmp(argv[i], "-", 1) == 0)
                    goto ShowUsage;
                path = argv[i];
                state = 1;
                break;
            case 1:
                if (strncmp(argv[i], "--", 2) == 0 || strncmp(argv[i], "-", 1) == 0)
                    goto ShowUsage;
                output = argv[i];
                state = 2;
                break;
            default:
                goto ShowUsage;
        }
    }

    L = luaL_newstate();
    if (!L)
    {
        cerr << "Create lua VM failed" << endl;
        return -1;
    }

    luaL_openlibs(L);
    et::RegisterLibrary(L, "et");

    // evaluate input expressions
    for (int i = paramIndex; i < argc; ++i)
    {
        const char* expr = argv[i];

        int err = (luaL_loadstring(L, expr) || lua_pcall(L, 0, 0, 0));
        if (err != LUA_OK)
        {
            const char* msg = lua_tostring(L, -1);
            fprintf(stderr, "%s", msg);

            lua_close(L);
            return -2;
        }
    }

    try
    {
        string builder;

        // if no input file, read from stdin
        if (path == nullptr)
        {
            cin >> std::noskipws;

            istream_iterator<char> it(cin);
            istream_iterator<char> end;
            string input(it, end);

            builder.reserve(input.length());
            et::RenderString(builder, L, input.c_str(), "stdin");
        }
        else
            et::RenderFile(builder, L, path);

        // write to file or stdout
        if (output == nullptr)
            cout << builder;
        else
        {
            fstream f(output, ios::out);
            if (!f)
            {
                cerr << "Open output file \"" << output << "\" error" << endl;
                return -3;
            }
            f << builder;
        }
    }
    catch (const std::exception& ex)
    {
        cerr << ex.what() << endl;
    }

    return 0;

ShowUsage:
    cerr << "A simple text template renderer." << endl;
    cerr << "Usage: " << et::GetFileName(argv[0]) << " [<input> [<output>]] [-- <expr...>]" << endl;
    cerr << "Options:" << endl;
    cerr << "  --stdin, -i     Input from stdin" << endl;
    cerr << "  --help, -h      Show this help" << endl;
    return -1;
}

/**
 * @file
 * @author chu
 * @date 18/1/8
 */
#include <et.hpp>
#include <et/TemplateNode.hpp>

using namespace std;
using namespace et;

//////////////////////////////////////////////////////////////////////////////// Export for lua

namespace
{
    static int LuaRenderString(lua_State* L)noexcept  // input: string, [sourceName: string], [env: table]
    {
        const char* input = luaL_checkstring(L, 1);
        const char* sourceName = "Unknown";
        int envIndex = 0;
        if (lua_gettop(L) > 1)
        {
            if (lua_type(L, 2) == LUA_TSTRING)
            {
                sourceName = luaL_checkstring(L, 2);
                if (lua_gettop(L) > 2)
                {
                    luaL_checktype(L, 3, LUA_TTABLE);
                    envIndex = lua_absindex(L, 3);
                }
            }
            else if (lua_type(L, 2) == LUA_TTABLE)
                envIndex = lua_absindex(L, 2);
        }

        bool error = false;
        string output;

        // 处理异常
        try
        {
            RenderString(output, L, input, sourceName, envIndex);
        }
        catch (const std::exception& ex)
        {
            error = true;

            try
            {
                output.clear();
                output = ex.what();
            }
            catch (const std::exception& ex)  // 基本就是bad_alloc了，也尝试恢复下
            {
                luaL_error(L, "%s", ex.what());
            }
        }

        if (error)
        {
            lua_pushnil(L);
            lua_pushlstring(L, output.c_str(), output.length());
            return 2;
        }
        else
        {
            lua_pushlstring(L, output.c_str(), output.length());
            return 1;
        }
    }

    static int LuaRenderFile(lua_State* L)noexcept  // path: string
    {
        const char* path = luaL_checkstring(L, 1);

        bool error = false;
        string output;

        // 处理异常
        try
        {
            RenderFile(output, L, path);
        }
        catch (const std::exception& ex)
        {
            error = true;

            try
            {
                output.clear();
                output = ex.what();
            }
            catch (const std::exception& ex)  // 基本就是bad_alloc了，也尝试恢复下
            {
                luaL_error(L, "%s", ex.what());
            }
        }

        if (error)
        {
            lua_pushnil(L);
            lua_pushlstring(L, output.c_str(), output.length());
            return 2;
        }
        else
        {
            lua_pushlstring(L, output.c_str(), output.length());
            return 1;
        }
    }

    static int LuaDumpString(lua_State* L)noexcept
    {
        return 0;
    }

    static int LuaDumpValue(lua_State* L)noexcept
    {
        return 0;
    }
}

extern "C" int luaopen_et(lua_State* L)
{
    static const luaL_Reg kEntry[] = {
        { "render_string", LuaRenderString },
        { "render_file", LuaRenderFile },
        { "dump_string", LuaDumpString },
        { "dump_value", LuaDumpValue },
    };

    luaL_newlib(L, kEntry);
    return 1;
}

//////////////////////////////////////////////////////////////////////////////// Api

void et::RenderString(std::string& out, lua_State* L, const char* input, const char* sourceName, int env)
{
    size_t length = strlen(input);
    out.clear();
    out.reserve(length);

    // 解析
    TextReader reader(input, length, sourceName);
    TemplateParser parser;
    parser.Run(reader);

    // 生成模板语法树
    auto root = BuildRootNode(parser);

    // 渲染
#ifndef NDEBUG
    int top = lua_gettop(L);
#endif
    root->Render(out, L, env);
    assert(top == lua_gettop(L));
}

void et::RenderFile(std::string& out, lua_State* L, const char* path, int env)
{
    out.clear();

    // 读取文件
    string input;
    ReadFile(input, path);
    out.reserve(input.size());

    // 解析
    string sourceName = GetFileName(path);
    TextReader reader(input.c_str(), input.length(), sourceName.c_str());
    TemplateParser parser;
    parser.Run(reader);

    // 生成模板语法树
    auto root = BuildRootNode(parser);

    // 渲染
#ifndef NDEBUG
    int top = lua_gettop(L);
#endif
    root->Render(out, L, env);
    assert(top == lua_gettop(L));
}

void et::RegisterLibrary(lua_State* L, const char* name)
{
    auto ret = luaopen_et(L);
    assert(ret == 1);
    lua_setglobal(L, name);
}

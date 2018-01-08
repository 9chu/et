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

static const char kHexDigitTable[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

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
            else
            {
                luaL_checktype(L, 2, LUA_TTABLE);
                envIndex = lua_absindex(L, 2);
            }
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

    static int LuaRenderFile(lua_State* L)noexcept  // path: string, [env: table]
    {
        const char* path = luaL_checkstring(L, 1);
        int envIndex = 0;

        if (lua_gettop(L) > 1)
        {
            luaL_checktype(L, 2, LUA_TTABLE);
            envIndex = lua_absindex(L, 2);
        }

        bool error = false;
        string output;

        // 处理异常
        try
        {
            RenderFile(output, L, path, envIndex);
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

    static int LuaDumpString(lua_State* L)noexcept  // raw: string
    {
        const char* raw = luaL_checkstring(L, 1);
        size_t len = strlen(raw);

        char tmp[64];
        size_t tmpUsed = 0;

        lua_pushstring(L, "\"");  // '"'

        for (size_t i = 0; i < len; ++i)
        {
            if (tmpUsed + 4 + 1 >= sizeof(tmp))
            {
                lua_pushlstring(L, tmp, tmpUsed);  // s s
                lua_concat(L, 2);  // s
                tmpUsed = 0;
            }

            char ch = raw[i];
            switch (ch)
            {
                case '\a':
                    tmp[tmpUsed++] = '\\';
                    tmp[tmpUsed++] = 'a';
                    break;
                case '\b':
                    tmp[tmpUsed++] = '\\';
                    tmp[tmpUsed++] = 'b';
                    break;
                case '\f':
                    tmp[tmpUsed++] = '\\';
                    tmp[tmpUsed++] = 'f';
                    break;
                case '\n':
                    tmp[tmpUsed++] = '\\';
                    tmp[tmpUsed++] = 'n';
                    break;
                case '\r':
                    tmp[tmpUsed++] = '\\';
                    tmp[tmpUsed++] = 'r';
                    break;
                case '\t':
                    tmp[tmpUsed++] = '\\';
                    tmp[tmpUsed++] = 't';
                    break;
                case '\v':
                    tmp[tmpUsed++] = '\\';
                    tmp[tmpUsed++] = 'v';
                    break;
                case '\\':
                    tmp[tmpUsed++] = '\\';
                    tmp[tmpUsed++] = '\\';
                    break;
                case '\'':
                    tmp[tmpUsed++] = '\\';
                    tmp[tmpUsed++] = '\'';
                    break;
                case '\"':
                    tmp[tmpUsed++] = '\\';
                    tmp[tmpUsed++] = '"';
                    break;
                default:
                    if (ch > 0 && isprint(ch))
                        tmp[tmpUsed++] = ch;
                    else
                    {
                        int hex = static_cast<uint8_t>(ch);
                        tmp[tmpUsed++] = '\\';
                        tmp[tmpUsed++] = 'x';
                        tmp[tmpUsed++] = kHexDigitTable[(hex >> 4) & 0xF];
                        tmp[tmpUsed++] = kHexDigitTable[hex & 0xF];
                    }
                    break;
            }
        }

        tmp[tmpUsed++] = '"';

        lua_pushlstring(L, tmp, tmpUsed);  // s s
        lua_concat(L, 2);  // s
        return 1;
    }

    static int LuaDumpValue(lua_State* L)noexcept  // value: any
    {
        switch (lua_type(L, 1))
        {
            case LUA_TNIL:
                lua_pushstring(L, "nil");
                return 1;
            case LUA_TBOOLEAN:
                lua_pushstring(L, lua_toboolean(L, 1) ? "true" : "false");
                return 1;
            case LUA_TNUMBER:
                lua_pushvalue(L, 1);
                lua_tostring(L, -1);
                return 1;
            case LUA_TSTRING:
                lua_pushcfunction(L, LuaDumpString);
                lua_pushvalue(L, 1);
                lua_call(L, 1, 1);
                return 1;
            case LUA_TTABLE:
                {
                    lua_pushstring(L, "{");
                    int base = lua_gettop(L);  // '{'的位置

                    bool firstPair = true;
                    const char* key = nullptr;

                    lua_pushnil(L);
                    while (lua_next(L, 1) != 0)  // key在top - 2，value在top - 1
                    {
                        int valueIdx = lua_gettop(L);  // value的位置
                        int keyIdx = valueIdx - 1;

                        // 复制上一次合并结果
                        lua_pushvalue(L, base);  // key, value, '{'

                        // 决定是否加上逗号
                        if (firstPair)
                            firstPair = false;
                        else
                            lua_pushstring(L, ", ");

                        // 导出Key
                        switch (lua_type(L, keyIdx))
                        {
                            case LUA_TBOOLEAN:
                                lua_pushstring(L, "[");
                                lua_pushstring(L, lua_toboolean(L, keyIdx) ? "true" : "false");
                                lua_pushstring(L, "]=");
                                break;
                            case LUA_TNUMBER:
                                lua_pushstring(L, "[");
                                lua_pushvalue(L, keyIdx);
                                lua_tostring(L, -1);
                                lua_pushstring(L, "]=");
                                break;
                            case LUA_TSTRING:
                                key = lua_tostring(L, keyIdx);
                                if (IsLuaIdentifier(key))
                                {
                                    lua_pushvalue(L, keyIdx);
                                    lua_pushstring(L, "=");
                                }
                                else
                                {
                                    lua_pushstring(L, "[");
                                    lua_pushcfunction(L, LuaDumpString);
                                    lua_pushvalue(L, keyIdx);
                                    lua_call(L, 1, 1);
                                    lua_pushstring(L, "]=");
                                }
                                break;
                            default:
                                luaL_error(L, "Unexpected key of type \"%s\"", luaL_typename(L, keyIdx));
                                return 0;
                        }

                        // 导出Value
                        lua_pushcfunction(L, LuaDumpValue);
                        lua_pushvalue(L, valueIdx);
                        lua_call(L, 1, 1);

                        // 合并所有字符串
                        lua_concat(L, lua_gettop(L) - valueIdx);

                        // 删除之前的结果
                        lua_remove(L, base);
                        lua_insert(L, base);

                        lua_pop(L, 1);  // 去掉Value，保留栈顶的Key继续进行遍历
                    }
                }

                lua_pushstring(L, "}");
                lua_concat(L, 2);
                return 1;
            default:
                luaL_error(L, "Unexpected value of type \"%s\"", luaL_typename(L, 1));
                return 0;
        }
    }

    static int LuaRangeClosure(lua_State* L)noexcept  // state: any, lastvalue: number
    {
        double last = luaL_checknumber(L, 2);
        double to = lua_tonumber(L, lua_upvalueindex(1));
        double step = lua_tonumber(L, lua_upvalueindex(2));

        double next = last + step;
        if ((step > 0 && next <= to) || (step < 0 && next >= to) || step == 0)
            lua_pushnumber(L, next);
        else
            lua_pushnil(L);
        return 1;
    }

    static int LuaRangeClosureInteger(lua_State* L)noexcept  // state: any, lastvalue: integer
    {
        lua_Integer last = luaL_checkinteger(L, 2);
        lua_Integer to = lua_tointeger(L, lua_upvalueindex(1));
        lua_Integer step = lua_tointeger(L, lua_upvalueindex(2));

        lua_Integer next = last + step;
        if ((step > 0 && next <= to) || (step < 0 && next >= to) || step == 0)
            lua_pushinteger(L, next);
        else
            lua_pushnil(L);
        return 1;
    }

    static int LuaRange(lua_State* L)noexcept  // from: number, to: number, [step: number=1]
    {
        // see: http://lua-users.org/wiki/RangeIterator
        if (lua_isinteger(L, 1) && lua_isinteger(L, 2) && !(lua_gettop(L) > 2 && !lua_isinteger(L, 3)))
        {
            // 纯整数迭代版本
            lua_Integer from = lua_tointeger(L, 1);
            lua_Integer to = lua_tointeger(L, 2);
            lua_Integer step = 1;
            if (lua_gettop(L) > 2)
                step = lua_tointeger(L, 3);

            lua_pushinteger(L, to);  // upvalue 1
            lua_pushinteger(L, step);  // upvalue 2
            lua_pushcclosure(L, LuaRangeClosureInteger, 2);
            lua_pushnil(L);
            lua_pushinteger(L, from - step);
            return 3;
        }
        else
        {
            double from = luaL_checknumber(L, 1);
            double to = luaL_checknumber(L, 2);
            double step = 1;
            if (lua_gettop(L) > 2)
                step = luaL_checknumber(L, 3);

            lua_pushnumber(L, to);  // upvalue 1
            lua_pushnumber(L, step);  // upvalue 2
            lua_pushcclosure(L, LuaRangeClosure, 2);
            lua_pushnil(L);
            lua_pushnumber(L, from - step);
            return 3;
        }
    }
}

extern "C" int ET_EXPORT_API luaopen_et(lua_State* L)
{
    static const luaL_Reg kEntry[] = {
        { "render_string", LuaRenderString },
        { "render_file", LuaRenderFile },
        { "dump_string", LuaDumpString },
        { "dump_value", LuaDumpValue },
        { "range", LuaRange },
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

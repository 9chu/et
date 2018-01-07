/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <et/LuaHelper.hpp>

#include <set>

using namespace std;
using namespace et;

namespace
{
    class KeywordList
    {
    public:
        KeywordList()
        {
            m_stList.emplace("and");
            m_stList.emplace("false");
            m_stList.emplace("local");
            m_stList.emplace("then");
            m_stList.emplace("break");
            m_stList.emplace("for");
            m_stList.emplace("nil");
            m_stList.emplace("true");
            m_stList.emplace("do");
            m_stList.emplace("function");
            m_stList.emplace("not");
            m_stList.emplace("until");
            m_stList.emplace("else");
            m_stList.emplace("goto");
            m_stList.emplace("or");
            m_stList.emplace("while");
            m_stList.emplace("elseif");
            m_stList.emplace("if");
            m_stList.emplace("repeat");
            m_stList.emplace("end");
            m_stList.emplace("in");
            m_stList.emplace("return");
        }

    public:
        bool operator()(const std::string& test)const noexcept
        {
            return m_stList.find(test) != m_stList.end();
        }

        bool operator()(const char* test)const noexcept
        {
            return m_stList.find(test) != m_stList.end();
        }

    private:
        set<string> m_stList;
    };
}

bool LuaHelper::IsIdentifier(const char* raw)noexcept
{
    auto len = strlen(raw);
    if (len == 0)
        return false;

    for (size_t i = 0; i < len; ++i)
    {
        auto ch = raw[i];

        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || (i > 0 && ch >= '0' && ch <= '9')))
            return false;
    }

    return true;
}

bool LuaHelper::IsKeyword(const char* raw)noexcept
{
    static const KeywordList kKeywords {};

    return kKeywords(raw);
}

bool LuaHelper::IsKeyword(const std::string& raw)noexcept
{
    static const KeywordList kKeywords {};

    return kKeywords(raw);
}

std::string LuaHelper::BuildString(const char* raw)
{
    string ret;
    auto len = strlen(raw);

    ret.reserve(len + 2);
    ret.push_back('"');

    for (size_t i = 0; i < len; ++i)
    {
        auto ch = raw[i];
        switch (ch)
        {
            case '\a':
                ret.append("\\a");
                break;
            case '\b':
                ret.append("\\b");
                break;
            case '\f':
                ret.append("\\f");
                break;
            case '\n':
                ret.append("\\n");
                break;
            case '\r':
                ret.append("\\r");
                break;
            case '\t':
                ret.append("\\t");
                break;
            case '\v':
                ret.append("\\v");
                break;
            case '\\':
                ret.append("\\\\");
                break;
            case '\'':
                ret.append("\\'");
                break;
            case '\"':
                ret.append("\\\"");
                break;
            default:
                ret.push_back(ch);
                break;
        }
    }

    ret.push_back('"');
    return ret;
}

std::string& LuaHelper::BuildValue(std::string& builder, lua_State* L, int idx)
{
    idx = lua_absindex(L, idx);

    switch (lua_type(L, idx))
    {
        case LUA_TNIL:
            builder.append("nil");
            break;
        case LUA_TBOOLEAN:
            builder.append(lua_toboolean(L, idx) ? "true" : "false");
            break;
        case LUA_TNUMBER:
            builder.append(Format("%g", lua_tonumber(L, idx)));
            break;
        case LUA_TSTRING:
            builder.append(BuildString(lua_tostring(L, idx)));
            break;
        case LUA_TTABLE:
            builder.push_back('{');

            {
                bool firstPair = true;
                const char* key = nullptr;

                lua_pushnil(L);
                while (lua_next(L, idx) != 0)  // key在top - 2，value在top - 1
                {
                    try
                    {
                        // 决定是否加上逗号
                        if (firstPair)
                            firstPair = false;
                        else
                            builder.append(", ");

                        // 渲染key
                        switch (lua_type(L, -2))
                        {
                            case LUA_TBOOLEAN:
                                builder.push_back('[');
                                builder.append(lua_toboolean(L, idx) ? "true" : "false");
                                builder.append("]=");
                                break;
                            case LUA_TNUMBER:
                                builder.push_back('[');
                                builder.append(Format("%g", lua_tonumber(L, -2)));
                                builder.append("]=");
                                break;
                            case LUA_TSTRING:
                                key = lua_tostring(L, -2);
                                if (IsIdentifier(key))
                                {
                                    builder.append(key);
                                    builder.push_back('=');
                                }
                                else
                                {
                                    builder.push_back('[');
                                    builder.append(BuildString(key));
                                    builder.append("]=");
                                }
                                break;
                            default:
                                ET_THROW(InvalidArgumentException, "Unexpected key of type \"%s\"",
                                    luaL_typename(L, -2));
                        }

                        // 渲染Value
                        BuildValue(builder, L, -1);
                    }
                    catch (...)
                    {
                        lua_pop(L, 2);  // 平衡堆栈
                        throw;
                    }

                    lua_pop(L, 1);  // 去掉Value，保留栈顶的Key继续进行遍历
                }
            }

            builder.push_back('}');
            break;
        default:
            ET_THROW(InvalidArgumentException, "Unexpected value of type \"%s\"", luaL_typename(L, idx));
    }

    return builder;
}

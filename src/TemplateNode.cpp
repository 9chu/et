/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <et/TemplateNode.hpp>

#include <queue>
#include <cassert>

using namespace std;
using namespace et;

//////////////////////////////////////////////////////////////////////////////// TemplateTextNode

TemplateTextNode::TemplateTextNode(const char* start, size_t len)
    : m_pszStart(start), m_ullLength(len)
{
}

TemplateNodeTypes TemplateTextNode::GetType()const noexcept
{
    return TemplateNodeTypes::Text;
}

void TemplateTextNode::Render(std::string& builder, lua_State* L)const
{
    ET_UNUSED(L);

    if (m_pszStart && m_ullLength > 0)
        builder.append(m_pszStart, m_ullLength);
}

//////////////////////////////////////////////////////////////////////////////// TemplateBlockNode

TemplateBlockNode::TemplateBlockNode(TemplateBlockNode&& rhs)noexcept
    : m_vecNodes(std::move(rhs.m_vecNodes))
{
}

TemplateBlockNode& TemplateBlockNode::operator=(TemplateBlockNode&& rhs)noexcept
{
    m_vecNodes = std::move(rhs.m_vecNodes);
    return *this;
}

TemplateNodeBase* TemplateBlockNode::GetByIndex(size_t idx)const noexcept
{
    if (idx >= m_vecNodes.size())
        return nullptr;
    return m_vecNodes[idx].get();
}

void TemplateBlockNode::Append(std::unique_ptr<TemplateNodeBase>&& p)
{
    m_vecNodes.emplace_back(std::move(p));
}

TemplateNodeTypes TemplateBlockNode::GetType()const noexcept
{
    return TemplateNodeTypes::Block;
}

void TemplateBlockNode::Render(std::string& builder, lua_State* L)const
{
    for (auto& node : m_vecNodes)
        node->Render(builder, L);
}

//////////////////////////////////////////////////////////////////////////////// TemplateExpressionNode

static const char kReturn[] = "return ";

TemplateExpressionNode::TemplateExpressionNode(const char* source, uint32_t line, const char* start, size_t len)
    : m_pszSource(source), m_uLine(line)
{
    // 加上一个return使得变成一个表达式
    m_stExpression.reserve(sizeof(kReturn) + len);
    m_stExpression.append(kReturn);
    m_stExpression.append(!start ? "" : start, len);
}

TemplateNodeTypes TemplateExpressionNode::GetType()const noexcept
{
    return TemplateNodeTypes::Expression;
}

void TemplateExpressionNode::Render(std::string& builder, lua_State* L)const
{
    // 先以表达式方式编译
    int ret = luaL_loadbufferx(L, m_stExpression.c_str(), m_stExpression.length(), "=(expr)", "t");
    if (ret != LUA_OK)
    {
        // 编译失败，换成语句块模式
        lua_pop(L, 1);
        ret = luaL_loadbufferx(L, m_stExpression.c_str() + sizeof(kReturn) - 1,
            m_stExpression.length() - sizeof(kReturn) + 1, "=(expr)", "t");

        if (ret != LUA_OK)
        {
            string error = lua_tostring(L, -1);
            lua_pop(L, 1);
            ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
        }
    }

    // 执行语句或者表达式
    int base = lua_gettop(L) - 1;  // 去掉栈顶的语句块
    ret = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (ret != LUA_OK)
    {
        string error = lua_tostring(L, -1);
        lua_pop(L, 1);
        ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
    }

    // 栈顶剩下需要打印输出的内容
    int top = lua_gettop(L);
    assert(top >= base);
    int count = top - base;

    try
    {
        for (auto i = 0; i < count; ++i)
        {
            auto idx = i + base + 1;

            switch (lua_type(L, idx))
            {
                case LUA_TNIL:
                    break;
                case LUA_TBOOLEAN:
                    builder.append(lua_toboolean(L, idx) ? "true" : "false");
                    break;
                case LUA_TNUMBER:
                case LUA_TSTRING:
                    builder.append(lua_tostring(L, idx));
                    break;
                default:
                    ET_THROW(RenderException, "%s:%u: Unexpected expression return type %s", m_pszSource, m_uLine,
                        luaL_typename(L, idx));
            }
        }
    }
    catch (...)
    {
        // 平衡堆栈
        lua_pop(L, count);
        throw;
    }

    // 平衡堆栈
    lua_pop(L, count);
}

//////////////////////////////////////////////////////////////////////////////// BuildRootNode

std::unique_ptr<TemplateBlockNode> et::BuildRootNode(const TemplateParser& parser)
{
    unique_ptr<TemplateBlockNode> result;
    queue<TemplateNodeBase*> unclosed;

    for (size_t i = 0; i < parser.GetTokenCount(); ++i)
    {
        const auto& node = parser.GetTokenByIndex(i);
        auto back = (unclosed.empty() ? nullptr : unclosed.back());

        switch (node.Type)
        {
            case TemplateParser::TokenTypes::Literal:
                break;
        }
    }

    return result;
}

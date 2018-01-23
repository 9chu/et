/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <et/TemplateNode.hpp>

#include <stack>
#include <cassert>

using namespace std;
using namespace et;

static const char kReturn[] = "return ";

namespace
{
    string SafeAssignString(const char* raw)
    {
        string ret;
        try
        {
            ret.assign(raw);
        }
        catch (...)
        {
        }
        return ret;
    }
}

//////////////////////////////////////////////////////////////////////////////// TemplateNodeBase

size_t TemplateNodeBase::GetNodeCount()const noexcept
{
    return 0u;
}

TemplateNodeBase* TemplateNodeBase::GetNodeByIndex(size_t index)const noexcept
{
    return nullptr;
}

size_t TemplateNodeBase::FindNode(TemplateNodeBase* node)const noexcept
{
    ET_UNUSED(node);
    return static_cast<size_t>(-1);
}

void TemplateNodeBase::AppendNode(std::unique_ptr<TemplateNodeBase>&& p)
{
    ET_UNUSED(p);
    assert(false);
}

bool TemplateNodeBase::RemoveNode(size_t index)noexcept
{
    ET_UNUSED(index);
    return false;
}

//////////////////////////////////////////////////////////////////////////////// TemplateTextNode

TemplateTextNode::TemplateTextNode(std::string&& content)
    : m_stContent(std::move(content))
{
}

TemplateNodeTypes TemplateTextNode::GetType()const noexcept
{
    return TemplateNodeTypes::Text;
}

void TemplateTextNode::Render(std::string& builder, lua_State* L, int env)const
{
    ET_UNUSED(L);
    ET_UNUSED(env);
    builder.append(m_stContent);
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

TemplateNodeTypes TemplateBlockNode::GetType()const noexcept
{
    return TemplateNodeTypes::Block;
}

size_t TemplateBlockNode::GetNodeCount()const noexcept
{
    return m_vecNodes.size();
}

TemplateNodeBase* TemplateBlockNode::GetNodeByIndex(size_t index)const noexcept
{
    if (index >= m_vecNodes.size())
        return nullptr;
    return m_vecNodes[index].get();
}

size_t TemplateBlockNode::FindNode(TemplateNodeBase* node)const noexcept
{
    for (size_t i = 0u; i < m_vecNodes.size(); ++i)
    {
        if (m_vecNodes[i].get() == node)
            return i;
    }
    return static_cast<size_t>(-1);
}

void TemplateBlockNode::AppendNode(std::unique_ptr<TemplateNodeBase>&& p)
{
    assert(p->GetParent() == nullptr);
    m_vecNodes.emplace_back(std::move(p));
    m_vecNodes.back()->SetParent(this);
}

bool TemplateBlockNode::RemoveNode(size_t index)noexcept
{
    if (index >= m_vecNodes.size())
        return false;
    m_vecNodes.erase(m_vecNodes.begin() + index);
    return true;
}

void TemplateBlockNode::Render(std::string& builder, lua_State* L, int env)const
{
    for (const auto& node : m_vecNodes)
        node->Render(builder, L, env);
}

//////////////////////////////////////////////////////////////////////////////// TemplateExpressionNode

TemplateExpressionNode::TemplateExpressionNode(const char* source, uint32_t line, std::string&& expr)
    : m_pszSource(source), m_uLine(line)
{
    // 加上一个return使得变成一个表达式
    m_stExpression = std::move(expr);
    m_stExpression.insert(0, kReturn);
}

TemplateNodeTypes TemplateExpressionNode::GetType()const noexcept
{
    return TemplateNodeTypes::Expression;
}

void TemplateExpressionNode::Render(std::string& builder, lua_State* L, int env)const
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
            string error = SafeAssignString(lua_tostring(L, -1));
            lua_pop(L, 1);
            ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
        }
    }

    // 设置ENV
    if (env != 0)
    {
        lua_pushvalue(L, env);
        if (!lua_setupvalue(L, -2, 1))
            lua_pop(L, 1);
    }

    // 执行语句或者表达式
    int base = lua_gettop(L) - 1;  // 去掉栈顶的语句块
    ret = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (ret != LUA_OK)
    {
        string error = SafeAssignString(lua_tostring(L, -1));
        lua_pop(L, 1);
        ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
    }

    // 栈顶剩下需要打印输出的内容
    int top = lua_gettop(L);
    assert(top >= base);
    int count = top - base;

    try
    {
        for (int i = 0; i < count; ++i)
        {
            int idx = i + base + 1;

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

//////////////////////////////////////////////////////////////////////////////// TemplateIfNode

TemplateIfNode::TemplateIfNode(const char* source, uint32_t line, std::string&& expr)
    : m_pszSource(source), m_uLine(line)
{
    // 加上一个return使得变成一个表达式
    m_stExpression = std::move(expr);
    m_stExpression.insert(0, kReturn);
}

TemplateIfNode::TemplateIfNode(TemplateIfNode&& rhs)noexcept
    : m_pszSource(rhs.m_pszSource), m_uLine(rhs.m_uLine), m_stExpression(std::move(rhs.m_stExpression)),
    m_vecTrueBranchNodes(std::move(rhs.m_vecTrueBranchNodes))
{
}

TemplateIfNode& TemplateIfNode::operator=(TemplateIfNode&& rhs)noexcept
{
    m_pszSource = rhs.m_pszSource;
    m_uLine = rhs.m_uLine;
    m_stExpression = std::move(rhs.m_stExpression);
    m_vecTrueBranchNodes = std::move(rhs.m_vecTrueBranchNodes);
    return *this;
}

TemplateIfNode::TemplateIfNode(const char* source, uint32_t line)
    : m_pszSource(source), m_uLine(line)
{
}

TemplateNodeBase* TemplateIfNode::GetTrueBranchNodeByIndex(size_t idx)const noexcept
{
    if (idx >= m_vecTrueBranchNodes.size())
        return nullptr;
    return m_vecTrueBranchNodes[idx].get();
}

TemplateNodeTypes TemplateIfNode::GetType()const noexcept
{
    return TemplateNodeTypes::If;
}

size_t TemplateIfNode::GetNodeCount()const noexcept
{
    return GetTrueBranchNodeCount();
}

TemplateNodeBase* TemplateIfNode::GetNodeByIndex(size_t index)const noexcept
{
    return GetTrueBranchNodeByIndex(index);
}

size_t TemplateIfNode::FindNode(TemplateNodeBase* node)const noexcept
{
    for (size_t i = 0u; i < m_vecTrueBranchNodes.size(); ++i)
    {
        if (m_vecTrueBranchNodes[i].get() == node)
            return i;
    }
    return static_cast<size_t>(-1);
}

void TemplateIfNode::AppendNode(std::unique_ptr<TemplateNodeBase>&& p)
{
    assert(p->GetParent() == nullptr);
    m_vecTrueBranchNodes.emplace_back(std::move(p));
    m_vecTrueBranchNodes.back()->SetParent(this);
}

bool TemplateIfNode::RemoveNode(size_t index)noexcept
{
    if (index >= m_vecTrueBranchNodes.size())
        return false;
    m_vecTrueBranchNodes.erase(m_vecTrueBranchNodes.begin() + index);
    return true;
}

void TemplateIfNode::Render(std::string& builder, lua_State* L, int env)const
{
    int ret = luaL_loadbufferx(L, m_stExpression.c_str(), m_stExpression.length(), "=(if)", "t");
    if (ret != LUA_OK)
    {
        string error = SafeAssignString(lua_tostring(L, -1));
        lua_pop(L, 1);
        ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
    }

    // 设置ENV
    if (env != 0)
    {
        lua_pushvalue(L, env);
        if (!lua_setupvalue(L, -2, 1))
            lua_pop(L, 1);
    }

    // 执行表达式
    ret = lua_pcall(L, 0, 1, 0);  // 只取第一个返回值
    if (ret != LUA_OK)
    {
        string error = SafeAssignString(lua_tostring(L, -1));
        lua_pop(L, 1);
        ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
    }

    // 检查结果
    auto result = static_cast<bool>(lua_toboolean(L, -1));
    lua_pop(L, 1);

    // 如果为真则执行TrueBranch
    if (result)
    {
        for (const auto& node : m_vecTrueBranchNodes)
            node->Render(builder, L, env);
    }
}

//////////////////////////////////////////////////////////////////////////////// TemplateIfElseNode

TemplateIfElseNode::TemplateIfElseNode(TemplateIfNode& origin)
    : TemplateIfNode(origin.m_pszSource, origin.m_uLine)
{
    m_stExpression = std::move(origin.m_stExpression);
    m_vecTrueBranchNodes = std::move(origin.m_vecTrueBranchNodes);
    for (auto& node : m_vecTrueBranchNodes)
        node->SetParent(this);
}

TemplateIfElseNode::TemplateIfElseNode(TemplateIfElseNode&& rhs)noexcept
    : TemplateIfNode(std::move(rhs)), m_vecFalseBranchNodes(std::move(rhs.m_vecFalseBranchNodes))
{
}

TemplateIfElseNode& TemplateIfElseNode::operator=(TemplateIfElseNode&& rhs)noexcept
{
    TemplateIfNode::operator=(std::move(rhs));
    m_vecFalseBranchNodes = std::move(rhs.m_vecFalseBranchNodes);
    return *this;
}

TemplateNodeBase* TemplateIfElseNode::GetFalseBranchNodeByIndex(size_t idx)const noexcept
{
    if (idx >= m_vecFalseBranchNodes.size())
        return nullptr;
    return m_vecFalseBranchNodes[idx].get();
}

TemplateNodeTypes TemplateIfElseNode::GetType()const noexcept
{
    return TemplateNodeTypes::IfElse;
}

size_t TemplateIfElseNode::GetNodeCount()const noexcept
{
    return GetFalseBranchNodeCount();
}

TemplateNodeBase* TemplateIfElseNode::GetNodeByIndex(size_t index)const noexcept
{
    return GetFalseBranchNodeByIndex(index);
}

size_t TemplateIfElseNode::FindNode(TemplateNodeBase* node)const noexcept
{
    for (size_t i = 0u; i < m_vecFalseBranchNodes.size(); ++i)
    {
        if (m_vecFalseBranchNodes[i].get() == node)
            return i;
    }
    return static_cast<size_t>(-1);
}

void TemplateIfElseNode::AppendNode(std::unique_ptr<TemplateNodeBase>&& p)
{
    assert(p->GetParent() == nullptr);
    m_vecFalseBranchNodes.emplace_back(std::move(p));
    m_vecFalseBranchNodes.back()->SetParent(this);
}

bool TemplateIfElseNode::RemoveNode(size_t index)noexcept
{
    if (index >= m_vecFalseBranchNodes.size())
        return false;
    m_vecFalseBranchNodes.erase(m_vecFalseBranchNodes.begin() + index);
    return true;
}

void TemplateIfElseNode::Render(std::string& builder, lua_State* L, int env)const
{
    int ret = luaL_loadbufferx(L, m_stExpression.c_str(), m_stExpression.length(), "=(if)", "t");
    if (ret != LUA_OK)
    {
        string error = SafeAssignString(lua_tostring(L, -1));
        lua_pop(L, 1);
        ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
    }

    // 设置ENV
    if (env != 0)
    {
        lua_pushvalue(L, env);
        if (!lua_setupvalue(L, -2, 1))
            lua_pop(L, 1);
    }

    // 执行表达式
    ret = lua_pcall(L, 0, 1, 0);  // 只取第一个返回值
    if (ret != LUA_OK)
    {
        string error = SafeAssignString(lua_tostring(L, -1));
        lua_pop(L, 1);
        ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
    }

    // 检查结果
    auto result = static_cast<bool>(lua_toboolean(L, -1));
    lua_pop(L, 1);

    // 如果为真则执行TrueBranch
    if (result)
    {
        for (const auto& node : m_vecTrueBranchNodes)
            node->Render(builder, L, env);
    }
    else  // 否则执行FalseBranch
    {
        for (const auto& node : m_vecFalseBranchNodes)
            node->Render(builder, L, env);
    }
}

//////////////////////////////////////////////////////////////////////////////// TemplateWhileNode

TemplateWhileNode::TemplateWhileNode(const char* source, uint32_t line, std::string&& expr)
    : m_pszSource(source), m_uLine(line)
{
    // 加上一个return使得变成一个表达式
    m_stExpression = std::move(expr);
    m_stExpression.insert(0, kReturn);
}

TemplateWhileNode::TemplateWhileNode(TemplateWhileNode&& rhs)noexcept
    : m_pszSource(rhs.m_pszSource), m_uLine(rhs.m_uLine), m_stExpression(std::move(rhs.m_stExpression)),
    m_vecNodes(std::move(rhs.m_vecNodes))
{
}

TemplateWhileNode& TemplateWhileNode::operator=(TemplateWhileNode&& rhs)noexcept
{
    m_pszSource = rhs.m_pszSource;
    m_uLine = rhs.m_uLine;
    m_stExpression = std::move(rhs.m_stExpression);
    m_vecNodes = std::move(rhs.m_vecNodes);
    return *this;
}

TemplateNodeTypes TemplateWhileNode::GetType()const noexcept
{
    return TemplateNodeTypes::While;
}

size_t TemplateWhileNode::GetNodeCount()const noexcept
{
    return m_vecNodes.size();
}

TemplateNodeBase* TemplateWhileNode::GetNodeByIndex(size_t index)const noexcept
{
    if (index >= m_vecNodes.size())
        return nullptr;
    return m_vecNodes[index].get();
}

size_t TemplateWhileNode::FindNode(TemplateNodeBase* node)const noexcept
{
    for (size_t i = 0u; i < m_vecNodes.size(); ++i)
    {
        if (m_vecNodes[i].get() == node)
            return i;
    }
    return static_cast<size_t>(-1);
}

void TemplateWhileNode::AppendNode(std::unique_ptr<TemplateNodeBase>&& p)
{
    assert(p->GetParent() == nullptr);
    m_vecNodes.emplace_back(std::move(p));
    m_vecNodes.back()->SetParent(this);
}

bool TemplateWhileNode::RemoveNode(size_t index)noexcept
{
    if (index >= m_vecNodes.size())
        return false;
    m_vecNodes.erase(m_vecNodes.begin() + index);
    return true;
}

void TemplateWhileNode::Render(std::string& builder, lua_State* L, int env)const
{
    int ret = luaL_loadbufferx(L, m_stExpression.c_str(), m_stExpression.length(), "=(while)", "t");
    if (ret != LUA_OK)
    {
        string error = SafeAssignString(lua_tostring(L, -1));
        lua_pop(L, 1);
        ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
    }

    // 设置ENV
    if (env != 0)
    {
        lua_pushvalue(L, env);
        if (!lua_setupvalue(L, -2, 1))
            lua_pop(L, 1);
    }

    // 此处，复制一份编译好的代码
    lua_pushvalue(L, -1);

#ifndef NDEBUG
    auto top = lua_gettop(L);
#endif

    bool result = false;
    do
    {
        // 执行表达式
        ret = lua_pcall(L, 0, 1, 0);  // 只取第一个返回值
        if (ret != LUA_OK)
        {
            string error = SafeAssignString(lua_tostring(L, -1));
            lua_pop(L, 1);
            ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
        }

        // 检查结果
        result = static_cast<bool>(lua_toboolean(L, -1));
        lua_pop(L, 1);

#ifndef NDEBUG
        auto currentTop = lua_gettop(L);
        assert(currentTop + 1 == top);
#endif

        if (result)
        {
            try
            {
                for (const auto &node : m_vecNodes)
                    node->Render(builder, L, env);
            }
            catch (...)
            {
                lua_pop(L, 1);  // 平衡堆栈，弹出复制的函数体
                throw;
            }

            // 此处，复制一份编译好的代码
            lua_pushvalue(L, -1);
        }
    } while (result);

    lua_pop(L, 1);  // 平衡堆栈，弹出复制的函数体

#ifndef NDEBUG
    auto currentTop = lua_gettop(L);
    assert(currentTop + 2 == top);
#endif
}

//////////////////////////////////////////////////////////////////////////////// TemplateForNode

TemplateForNode::TemplateForNode(const char* source, uint32_t line, std::string&& expr, std::vector<std::string>&& args)
    : m_pszSource(source), m_uLine(line), m_vecArgs(std::move(args))
{
    assert(m_vecArgs.size() > 0);

    // 加上一个return使得变成一个表达式
    m_stExpression = std::move(expr);
    m_stExpression.insert(0, kReturn);
}

TemplateForNode::TemplateForNode(TemplateForNode&& rhs)noexcept
    : m_pszSource(rhs.m_pszSource), m_uLine(rhs.m_uLine), m_stExpression(std::move(rhs.m_stExpression)),
    m_vecArgs(std::move(rhs.m_vecArgs)), m_vecNodes(std::move(rhs.m_vecNodes))
{
}

TemplateForNode& TemplateForNode::operator=(TemplateForNode&& rhs)noexcept
{
    m_pszSource = rhs.m_pszSource;
    m_uLine = rhs.m_uLine;
    m_stExpression = std::move(rhs.m_stExpression);
    m_vecArgs = std::move(rhs.m_vecArgs);
    m_vecNodes = std::move(rhs.m_vecNodes);
    return *this;
}

TemplateNodeTypes TemplateForNode::GetType()const noexcept
{
    return TemplateNodeTypes::For;
}

size_t TemplateForNode::GetNodeCount()const noexcept
{
    return m_vecNodes.size();
}

TemplateNodeBase* TemplateForNode::GetNodeByIndex(size_t index)const noexcept
{
    if (index >= m_vecNodes.size())
        return nullptr;
    return m_vecNodes[index].get();
}

size_t TemplateForNode::FindNode(TemplateNodeBase* node)const noexcept
{
    for (size_t i = 0u; i < m_vecNodes.size(); ++i)
    {
        if (m_vecNodes[i].get() == node)
            return i;
    }
    return static_cast<size_t>(-1);
}

void TemplateForNode::AppendNode(std::unique_ptr<TemplateNodeBase>&& p)
{
    assert(p->GetParent() == nullptr);
    m_vecNodes.emplace_back(std::move(p));
    m_vecNodes.back()->SetParent(this);
}

bool TemplateForNode::RemoveNode(size_t index)noexcept
{
    if (index >= m_vecNodes.size())
        return false;
    m_vecNodes.erase(m_vecNodes.begin() + index);
    return true;
}

void TemplateForNode::Render(std::string& builder, lua_State* L, int env)const
{
    // 获取栈顶
    int base = lua_gettop(L);

    // 保存Args
    if (env != 0)
    {
        for (const auto& name : m_vecArgs)
            lua_getfield(L, env, name.c_str());
    }
    else
    {
        for (const auto& name : m_vecArgs)
            lua_getglobal(L, name.c_str());
    }

    // 编译语句
    int ret = luaL_loadbufferx(L, m_stExpression.c_str(), m_stExpression.length(), "=(for)", "t");
    if (ret != LUA_OK)
    {
        string error = SafeAssignString(lua_tostring(L, -1));
        lua_settop(L, base);  // 平衡堆栈
        ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
    }

    // 设置ENV
    if (env != 0)
    {
        lua_pushvalue(L, env);
        if (!lua_setupvalue(L, -2, 1))
            lua_pop(L, 1);
    }

    // 执行表达式
    ret = lua_pcall(L, 0, 3, 0);  // For迭代器首次执行后会返回三个值
    if (ret != LUA_OK)
    {
        string error = SafeAssignString(lua_tostring(L, -1));
        lua_settop(L, base);  // 平衡堆栈
        ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
    }

    // 此时堆栈为
    // arg1bak, arg2bak, ..., argnbak, f, s, var
    // 调整堆栈到
    // arg1bak, arg2bak, ..., argnbak, f, s, f, s, var
    lua_pushvalue(L, -3);  // ...f, s, var, f
    lua_pushvalue(L, -3);  // ...f, s, var, f, s
    lua_pushvalue(L, -3);  // ...f, s, var, f, s, var
    lua_remove(L, lua_absindex(L, -4));  // ...f, s, f, s, var

    while (true)
    {
        // 执行迭代器
        ret = lua_pcall(L, 2, static_cast<int>(m_vecArgs.size()), 0);
        if (ret != LUA_OK)
        {
            string error = SafeAssignString(lua_tostring(L, -1));
            lua_settop(L, base);  // 平衡堆栈
            ET_THROW(LuaRuntimeException, "%s:%u: %s", m_pszSource, m_uLine, error.c_str());
        }

        // 如果执行成功，此时堆栈为
        // arg1bak, arg2bak, ..., argnbak, f, s, arg1, arg2, arg3...argn

        // 检查是否终止
        if (lua_type(L, -static_cast<int>(m_vecArgs.size())) == LUA_TNIL)
            break;

        // 依次保存结果，直到最后一个
        if (env == 0)
        {
            for (size_t i = m_vecArgs.size() - 1; i > 0; --i)
                lua_setglobal(L, m_vecArgs[i].c_str());
        }
        else
        {
            for (size_t i = m_vecArgs.size() - 1; i > 0; --i)
                lua_setfield(L, env, m_vecArgs[i].c_str());
        }

        // 最后一个需要复制并赋值
        lua_pushvalue(L, -1);
        if (env == 0)
            lua_setglobal(L, m_vecArgs[0].c_str());
        else
            lua_setfield(L, env, m_vecArgs[0].c_str());

        // 此时堆栈为
        // arg1bak, arg2bak, ..., argnbak, f, s, arg1
        assert(lua_gettop(L) - base == m_vecArgs.size() + 3);

        // 先执行下子节点
        try
        {
            for (const auto &node : m_vecNodes)
                node->Render(builder, L, env);
        }
        catch (...)
        {
            assert(lua_gettop(L) - base == m_vecArgs.size() + 3);
            lua_pop(L, 3);

            // 依次恢复值
            if (env == 0)
            {
                for (auto it = m_vecArgs.rbegin(); it != m_vecArgs.rend(); ++it)
                    lua_setglobal(L, it->c_str());
            }
            else
            {
                for (auto it = m_vecArgs.rbegin(); it != m_vecArgs.rend(); ++it)
                    lua_setfield(L, env, it->c_str());
            }

            assert(lua_gettop(L) == base);
            throw;
        }

        // 调整堆栈到
        // arg1bak, arg2bak, ..., argnbak, f, s, f, s, arg1
        lua_pushvalue(L, -3);  // ...f, s, arg1, f
        lua_pushvalue(L, -3);  // ...f, s, arg1, f, s
        lua_pushvalue(L, -3);  // ...f, s, arg1, f, s, arg1
        lua_remove(L, lua_absindex(L, -4));  // ...f, s, f, s, arg1
    }

    // 恢复备份值，此时堆栈为
    // arg1bak, arg2bak, ..., argnbak, f, s, arg1, arg2, arg3...argn
    assert(lua_gettop(L) - base == m_vecArgs.size() * 2 + 2);
    lua_pop(L, m_vecArgs.size() + 2);

    // 依次恢复值
    if (env == 0)
    {
        for (auto it = m_vecArgs.rbegin(); it != m_vecArgs.rend(); ++it)
            lua_setglobal(L, it->c_str());
    }
    else
    {
        for (auto it = m_vecArgs.rbegin(); it != m_vecArgs.rend(); ++it)
            lua_setfield(L, env, it->c_str());
    }

    assert(lua_gettop(L) == base);
}

//////////////////////////////////////////////////////////////////////////////// BuildRootNode

std::unique_ptr<TemplateBlockNode> et::BuildRootNode(TemplateParser& parser)
{
    unique_ptr<TemplateBlockNode> root;
    stack<TemplateNodeBase*> unclosed;

    root.reset(new TemplateBlockNode());
    for (size_t i = 0; i < parser.GetTokenCount(); ++i)
    {
        TemplateParser::Token& token = parser.GetTokenByIndex(i);
        TemplateNodeBase* top = (unclosed.empty() ? nullptr : unclosed.top());

        switch (token.Type)
        {
            case TemplateParser::TokenTypes::Literal:
                {
                    unique_ptr<TemplateTextNode> node;
                    node.reset(new TemplateTextNode(std::move(token.Content)));
                    top ? top->AppendNode(std::move(node)) : root->AppendNode(std::move(node));
                }
                break;
            case TemplateParser::TokenTypes::Expression:
                {
                    unique_ptr<TemplateExpressionNode> node;
                    node.reset(new TemplateExpressionNode(token.Anchor.SourceName, token.Anchor.Line,
                        std::move(token.Content)));
                    top ? top->AppendNode(std::move(node)) : root->AppendNode(std::move(node));
                }
                break;
            case TemplateParser::TokenTypes::If:
                {
                    unique_ptr<TemplateIfNode> node;
                    node.reset(new TemplateIfNode(token.Anchor.SourceName, token.Anchor.Line,
                        std::move(token.Content)));

                    auto weak = node.get();
                    top ? top->AppendNode(std::move(node)) : root->AppendNode(std::move(node));

                    // 加入未闭合队列
                    unclosed.push(weak);
                }
                break;
            case TemplateParser::TokenTypes::Else:
                if (!top || top->GetType() != TemplateNodeTypes::If)  // Else必须加插在If后面
                {
                    ET_THROW(ParseErrorException, "%s:%u:%u: Unexpected else branch", token.Anchor.SourceName,
                        token.Anchor.Line, token.Anchor.Column);
                }
                else
                {
                    auto parent = top->GetParent();
                    assert(parent);

                    unique_ptr<TemplateIfElseNode> node;
                    node.reset(new TemplateIfElseNode(*static_cast<TemplateIfNode*>(top)));

                    // 从Parent中删除
                    auto index = parent->FindNode(top);
                    assert(index == parent->GetNodeCount() - 1);  // 必然是最后一个节点
                    parent->RemoveNode(index);

                    // 取代之
                    auto weak = node.get();
                    parent->AppendNode(std::move(node));

                    // 加入未闭合队列
                    unclosed.pop();  // 去掉back
                    unclosed.push(weak);
                }
                break;
            case TemplateParser::TokenTypes::ElseIf:
                if (!top || top->GetType() != TemplateNodeTypes::If)  // Else必须加插在If后面
                {
                    ET_THROW(ParseErrorException, "%s:%u:%u: Unexpected else branch", token.Anchor.SourceName,
                        token.Anchor.Line, token.Anchor.Column);
                }
                else
                {
                    auto parent = top->GetParent();
                    assert(parent);

                    // If-Else会先构造一个闭合的Else节点
                    unique_ptr<TemplateIfElseNode> closedNode;
                    closedNode.reset(new TemplateIfElseNode(*static_cast<TemplateIfNode*>(top)));

                    // 从Parent中删除
                    auto index = parent->FindNode(top);
                    assert(index == parent->GetNodeCount() - 1);  // 必然是最后一个节点
                    parent->RemoveNode(index);

                    // 取代之
                    auto weakClosedNode = closedNode.get();
                    parent->AppendNode(std::move(closedNode));

                    // 构造一个新的If节点
                    unique_ptr<TemplateIfNode> node;
                    node.reset(new TemplateIfNode(token.Anchor.SourceName, token.Anchor.Line,
                        std::move(token.Content)));

                    auto weak = node.get();
                    weakClosedNode->AppendNode(std::move(node));

                    // 加入未闭合队列
                    unclosed.pop();  // 去掉back
                    unclosed.push(weak);
                }
                break;
            case TemplateParser::TokenTypes::For:
                {
                    unique_ptr<TemplateForNode> node;
                    node.reset(new TemplateForNode(token.Anchor.SourceName, token.Anchor.Line,
                        std::move(token.Content), std::move(token.Args)));

                    auto weak = node.get();
                    top ? top->AppendNode(std::move(node)) : root->AppendNode(std::move(node));

                    // 加入未闭合队列
                    unclosed.push(weak);
                }
                break;
            case TemplateParser::TokenTypes::While:
                {
                    unique_ptr<TemplateWhileNode> node;
                    node.reset(new TemplateWhileNode(token.Anchor.SourceName, token.Anchor.Line,
                        std::move(token.Content)));

                    auto weak = node.get();
                    top ? top->AppendNode(std::move(node)) : root->AppendNode(std::move(node));

                    // 加入未闭合队列
                    unclosed.push(weak);
                }
                break;
            case TemplateParser::TokenTypes::End:
                if (!top)
                {
                    ET_THROW(ParseErrorException, "%s:%u:%u: Unexpected block end", token.Anchor.SourceName,
                        token.Anchor.Line, token.Anchor.Column);
                }
                unclosed.pop();
                break;
            default:
                assert(false);
                break;
        }
    }

    if (!unclosed.empty())
    {
        ET_THROW(ParseErrorException, "%s:%u:%u: Unclosed block", parser.GetReader()->GetSourceName(),
            parser.GetReader()->GetLine(), parser.GetReader()->GetColumn());
    }

    parser.Clear();
    return root;
}

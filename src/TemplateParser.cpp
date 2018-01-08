/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <et/TemplateParser.hpp>
#include <et/LuaHelper.hpp>

using namespace std;
using namespace et;

namespace
{
    bool IsSpace(char ch)
    {
        return (ch > 0 && ::isspace(ch));
    }

    bool IsStartingByNewLine(const std::string& text)
    {
        for (char ch : text)
        {
            if (IsSpace(ch))
            {
                if (ch == '\n')
                    return true;
            }
            else
                break;
        }
        return false;
    }

    bool IsEndingByNewLine(const std::string& text)
    {
        for (auto it = text.rbegin(); it != text.rend(); ++it)
        {
            if (IsSpace(*it))
            {
                if (*it == '\n')
                    return true;
            }
            else
                break;
        }
        return false;
    }

    void TrimLeftUntilNewLine(std::string& text)
    {
        for (size_t i = 0; i < text.length(); ++i)
        {
            char ch = text[i];
            if (IsSpace(ch))
            {
                if (ch == '\n')
                {
                    text.erase(text.begin(), text.begin() + i);   // 不去掉最右边的换行
                    break;
                }
            }
            else
                break;
        }
    }

    void TrimRightUntilNewLine(std::string& text)
    {
        while (!text.empty())
        {
            char last = text.back();
            if (IsSpace(last))
            {
                text.pop_back();
                if (last == '\n')
                    break;
            }
            else
                break;
        }
    }
}

const TemplateParser::Token& TemplateParser::GetTokenByIndex(size_t index)const noexcept
{
    static const Token kEmptyToken {};

    if (index >= m_stTokenList.size())
        return kEmptyToken;
    return m_stTokenList[index];
}

TemplateParser::Token& TemplateParser::GetTokenByIndex(size_t index)noexcept
{
    static Token kEmptyToken {};

    if (index >= m_stTokenList.size())
    {
        assert(false);
        return kEmptyToken;
    }
    return m_stTokenList[index];
}

void TemplateParser::Clear()noexcept
{
    m_stTokenList.clear();
}

void TemplateParser::Run(TextReader& reader)
{
    ParserBase::Run(reader);

    Clear();
    m_stTokenList.reserve(16);

    Token token;
    while (ParseOuter(token))
    {
        assert(token.Type != TokenTypes::Eof);
        m_stTokenList.emplace_back(std::move(token));
    }

    if (m_bPrettify)
        Prettify();
}

void TemplateParser::SkipBlank()
{
    while (IsSpace(c))
        Next();
}

bool TemplateParser::TryAcceptIdentifierOrKeyword(std::string& out)
{
    out.clear();
    if (!(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
        return false;
    while (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
    {
        out.push_back(c);
        Next();
    }
    return true;
}

bool TemplateParser::ParseOuter(Token& result)
{
    size_t begin = GetReader()->GetPosition();
    size_t end = begin;

    result.Type = TokenTypes::Eof;
    result.Content.clear();
    result.Args.clear();
    result.Anchor = GetReader()->MakeAnchor();

    while (c != '\0')
    {
        if (c == '{')
        {
            Next();
            if (c == '%')
            {
                if (begin != end)
                {
                    Back();  // 放回'{'
                    break;
                }

                // 开始解析表达式节点
                Next();
                ParseInner(result);
                return true;
            }
            else if (c == '{')
            {
                ++end;
                continue;
            }
            else if (c != '\0')
                ++end;
        }

        ++end;
        Next();
    }

    if (begin == end)
        return false;

    assert(result.Type == TokenTypes::Eof);  // 表达式不走这个分支结束
    result.Type = TokenTypes::Literal;
    result.Content.assign(GetReader()->GetBuffer() + begin, end - begin);
    return true;
}

void TemplateParser::ParseInner(Token& result)
{
    result.Anchor = GetReader()->MakeAnchor();

    SkipBlank();

    if (TryAcceptIdentifierOrKeyword(m_stTmpBuffer))
    {
        if (m_stTmpBuffer == "end")
            result.Type = TokenTypes::End;
        else if (m_stTmpBuffer == "if")
            result.Type = TokenTypes::If;
        else if (m_stTmpBuffer == "else")
            result.Type = TokenTypes::Else;
        else if (m_stTmpBuffer == "elseif")
            result.Type = TokenTypes::ElseIf;
        else if (m_stTmpBuffer == "for")
        {
            result.Type = TokenTypes::For;

            // 出于实现简单考虑，只实现lua的for..in..循环
            // stat ::= for namelist in explist do block end
            // namelist ::= Name {',' Name}

            // 第一个参数
            SkipBlank();
            if (!TryAcceptIdentifierOrKeyword(m_stTmpBuffer))
                ET_PARSE_ERROR("Identifier expected, but found %s", PrintChar(c).c_str());
            if (IsLuaKeyword(m_stTmpBuffer))
                ET_PARSE_ERROR("Identifier expected, but found \"%s\"", m_stTmpBuffer.c_str());
            result.Args.emplace_back(std::move(m_stTmpBuffer));

            // 其他参数
            SkipBlank();
            while (TryAcceptOne(','))
            {
                SkipBlank();
                if (!TryAcceptIdentifierOrKeyword(m_stTmpBuffer))
                    ET_PARSE_ERROR("Identifier expected, but found %s", PrintChar(c).c_str());
                if (IsLuaKeyword(m_stTmpBuffer))
                    ET_PARSE_ERROR("Identifier expected, but found \"%s\"", m_stTmpBuffer.c_str());
                result.Args.emplace_back(std::move(m_stTmpBuffer));
                SkipBlank();
            }

            // "in"
            if (!TryAcceptIdentifierOrKeyword(m_stTmpBuffer))
                ET_PARSE_ERROR("\"in\" expected, but found %s", PrintChar(c).c_str());
            if (m_stTmpBuffer != "in")
                ET_PARSE_ERROR("\"in\" expected, but found \"%s\"", m_stTmpBuffer.c_str());
        }
        else if (m_stTmpBuffer == "while")
            result.Type = TokenTypes::While;
        else  // 不识别的节点，作为表达式传递给Lua
        {
            result.Type = TokenTypes::Expression;
            result.Content = std::move(m_stTmpBuffer);
        }
    }
    else
        result.Type = TokenTypes::Expression;

    if (result.Type != TokenTypes::Expression)
        SkipBlank();

    // 到这里为止，后续部分全部读取并交给Lua处理
    while (c != '\0')
    {
        if (c == '%')
        {
            Next();
            if (c == '}')
            {
                Next();

                // 去除末尾的空白
                TrimEnd(result.Content);

                bool shouldFollowingExpr = result.Type != TokenTypes::Expression && result.Type != TokenTypes::End &&
                    result.Type != TokenTypes::Else; // 除去Else和End以外，后面必然有个Expression

                if (shouldFollowingExpr)
                {
                    if (result.Content.empty())
                        ET_PARSE_ERROR("Expression required after statement");
                }
                else
                {
                    if (result.Type != TokenTypes::Expression && !result.Content.empty())
                        ET_PARSE_ERROR("Unexpected expression here");
                }
                return;
            }
            else if (c == '\0')
                break;
            else if (c != '%')
                result.Content.push_back('%');
        }

        result.Content.push_back(c);
        Next();
    }

    ET_PARSE_ERROR("Unclosed expression node");
}

void TemplateParser::Prettify()
{
    int state = 0;
    size_t left = static_cast<size_t>(-1);

    for (size_t i = 0; i < m_stTokenList.size(); ++i)
    {
        const Token& current = m_stTokenList[i];
        Token* prev = (i != 0 ? &m_stTokenList[i - 1] : nullptr);
        Token* next = (i + 1 < m_stTokenList.size() ? &m_stTokenList[i + 1] : nullptr);

        if (state == 0)
        {
            // 当前是语句，且上一个是文本
            if (current.Type != TokenTypes::Literal && current.Type != TokenTypes::Expression)
            {
                if (!prev || (prev->Type == TokenTypes::Literal && IsEndingByNewLine(prev->Content)))
                {
                    left = (prev ? i - 1 : static_cast<size_t>(-1));
                    state = 1;
                }
            }
        }
        if (state == 1)
        {
            if (!next || (next->Type == TokenTypes::Literal && IsStartingByNewLine(next->Content)))
            {
                size_t right = (next ? i + 1 : static_cast<size_t>(-1));
                if (left != static_cast<size_t>(-1))
                    TrimRightUntilNewLine(m_stTokenList[left].Content);
                if (right != static_cast<size_t>(-1))
                    TrimLeftUntilNewLine(m_stTokenList[right].Content);
                state = 0;
            }
            else if (current.Type == TokenTypes::Literal || current.Type == TokenTypes::Expression)  // 有其他文本，不剔除
                state = 0;
        }
    }

    if (state == 1)
    {
        if (left != static_cast<size_t>(-1))
            TrimRightUntilNewLine(m_stTokenList[left].Content);
    }
}

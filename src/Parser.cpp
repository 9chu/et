/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <et/Parser.hpp>

using namespace std;
using namespace et;

//////////////////////////////////////////////////////////////////////////////// TextReader

TextReader::TextReader()noexcept
{
}

TextReader::TextReader(const char* input, const char* name)
    : m_pszBuffer(input), m_ullBufferLength(strlen(input)), m_stSourceName(name)
{
}

TextReader::TextReader(const char* input, size_t len, const char* name)
    : m_pszBuffer(input), m_ullBufferLength(len), m_stSourceName(name)
{
}

bool TextReader::Back()noexcept
{
    if (m_uPosition == 0)
        return false;

    char ch = m_pszBuffer[--m_uPosition];
    if ((ch == '\r' && (m_uPosition + 1 >= m_ullBufferLength || m_pszBuffer[m_uPosition + 1] != '\n')) || ch == '\n')
    {
        --m_uLine;
        m_uColumn = 1;

        // 回溯寻找字符个数
        if (m_uPosition != 0)
        {
            const char* c = &m_pszBuffer[m_uPosition - 1];
            do
            {
                if ((*c == '\r' && *(c + 1) != '\n') || *c == '\n')
                    break;
                --c;
                ++m_uColumn;
            } while (c >= m_pszBuffer);
        }
    }
    else
        --m_uColumn;

    return true;
}

//////////////////////////////////////////////////////////////////////////////// ParserBase

std::string ParserBase::PrintChar(char ch)
{
    if (ch == '\0')
        return "<EOF>";
    else if (ch == '\'')
        return "'\''";
    else if (ch > 0 && ::isprint(ch) != 0)
        return Format("'%c'", ch);
    return Format("<%u>", static_cast<uint8_t>(ch));
}

void ParserBase::Run(TextReader& reader)
{
    m_pReader = &reader;
    c = m_pReader->Peek();
}

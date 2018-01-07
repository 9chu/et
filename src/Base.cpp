/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <et/Base.hpp>

using namespace std;
using namespace et;

//////////////////////////////////////////////////////////////////////////////// StringUtils

std::string et::Format(const char* format, ...)noexcept
{
    va_list args;
    va_start(args, format);
    auto str(FormatV(format, args));
    va_end(args);
    return str;
}

std::string et::FormatV(const char* format, std::va_list args)noexcept
{
    string ret;

    // 计算format后占用大小
    va_list ap;
    va_copy(ap, args);
    const int l = vsnprintf(nullptr, 0, format, ap);
    va_end(ap);

    if (l < 0)
        return string();

    // 尝试分配足够大小
    try
    {
        ret.resize(static_cast<size_t>(l));
    }
    catch (...)
    {
        ret.clear();
        return ret;
    }

    // 执行format
    if (l > 0)
        vsprintf(&ret[0], format, args);
    return ret;
}

void et::TrimEnd(std::string& str)noexcept
{
    while (!str.empty())
    {
        auto last = str.back();
        if (last > 0 && isspace(last))
            str.pop_back();
        else
            break;
    }
}

//////////////////////////////////////////////////////////////////////////////// Exception

Exception::Exception(const char* file, int line, const char* func, const char* format, ...)
    : m_pszFile(file), m_iLine(line), m_pszFunc(func)
{
    va_list args;
    va_start(args, format);
    auto description(FormatV(format, args));
    va_end(args);

    m_pDescription = make_shared<string>(std::move(description));
}

Exception::Exception(const Exception& rhs)noexcept
    : m_pszFile(rhs.m_pszFile), m_iLine(rhs.m_iLine), m_pszFunc(rhs.m_pszFunc), m_pDescription(rhs.m_pDescription)
{
}

Exception::Exception(Exception&& rhs)noexcept
{
    std::swap(m_pszFile, rhs.m_pszFile);
    std::swap(m_iLine, rhs.m_iLine);
    std::swap(m_pszFunc, rhs.m_pszFunc);
    std::swap(m_pDescription, rhs.m_pDescription);
}

Exception& Exception::operator=(const Exception& rhs)noexcept
{
    m_pszFile = rhs.m_pszFile;
    m_iLine = rhs.m_iLine;
    m_pszFunc = rhs.m_pszFunc;
    m_pDescription = rhs.m_pDescription;
    return *this;
}

Exception& Exception::operator=(Exception&& rhs)noexcept
{
    m_pszFile = rhs.m_pszFile;
    m_iLine = rhs.m_iLine;
    m_pszFunc = rhs.m_pszFunc;
    m_pDescription = rhs.m_pDescription;

    rhs.m_pszFile = nullptr;
    rhs.m_iLine = 0;
    rhs.m_pszFunc = nullptr;
    rhs.m_pDescription = nullptr;
    return *this;
}

const char* Exception::GetDescription()const noexcept
{
    return m_pDescription ? m_pDescription->c_str() : "";
}

const char* Exception::what()const noexcept
{
    return GetDescription();
}

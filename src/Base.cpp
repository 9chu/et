/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <et/Base.hpp>

#include <set>
#include <fstream>

using namespace std;
using namespace et;

//////////////////////////////////////////////////////////////////////////////// LuaHelper

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

bool et::IsLuaIdentifier(const char* raw)noexcept
{
    size_t len = strlen(raw);
    if (len == 0)
        return false;

    for (size_t i = 0; i < len; ++i)
    {
        char ch = raw[i];

        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_' || (i > 0 && ch >= '0' && ch <= '9')))
            return false;
    }

    return true;
}

bool et::IsLuaKeyword(const char* raw)noexcept
{
    static const KeywordList kKeywords {};

    return kKeywords(raw);
}

bool et::IsLuaKeyword(const std::string& raw)noexcept
{
    static const KeywordList kKeywords {};

    return kKeywords(raw);
}

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
        char last = str.back();
        if (last > 0 && isspace(last))
            str.pop_back();
        else
            break;
    }
}

//////////////////////////////////////////////////////////////////////////////// ReadFile

std::string et::GetFileName(const char* path)
{
    const char* lastFilenameStart = path;

    while (*path)
    {
        char c = *path;
        if (c == '\\' || c == '/')
            lastFilenameStart = path + 1;
        ++path;
    }

    return string(lastFilenameStart);
}

void et::ReadFile(std::string& out, const char* path)
{
    out.clear();

    ifstream t(path, ios::binary);
    if (!t.good())
        ET_THROW(IOException, "Open file \"{0}\" error", path);

    t.seekg(0, std::ios::end);
    if (!t.good())
        ET_THROW(IOException, "Seek to end on file \"{0}\" error", path);

    auto size = t.tellg();
    if (size < 0)
        ET_THROW(IOException, "Tellg on file \"{0}\" error", path);
    out.reserve(size);

    t.seekg(0, std::ios::beg);
    if (!t.good())
        ET_THROW(IOException, "Seek to begin on file \"{0}\" error", path);

    out.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    if (out.length() != (size_t)size)
        ET_THROW(IOException, "Read file \"{0}\" error", path);
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

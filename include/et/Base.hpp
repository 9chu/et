/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cassert>
#include <cstdint>
#include <cstdarg>
#include <cstring>
#include <stdexcept>
#include <algorithm>

#if defined(_MSC_VER)
#define ET_C_FORMAT_DECL(start, args)
#define ET_EXPORT_API __declspec(dllexport)
#elif defined(__MINGW32__)
#define ET_C_FORMAT_DECL(start, args) __attribute__((format(printf, start, args)))
#define ET_EXPORT_API __declspec(dllexport)
#else
#define ET_C_FORMAT_DECL(start, args) __attribute__((format(printf, start, args)))
#define ET_EXPORT_API
#endif

#define ET_THROW(exception, format, ...) \
    throw exception(__FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

#define ET_DEFINE_EXCEPTION(name) \
    class name : \
        public et::Exception \
    { \
    public: /* gcc和clang都不能很好的支持直接写Exception::Exception，此处为变通做法 */ \
        template <typename... Args> \
        name(const char* file, int line, const char* func, const char* format, Args... args) \
            : Exception(file, line, func, format, args...) {} \
    }

#define ET_UNUSED(x) static_cast<void>(x)

namespace et
{
    /**
     * @brief 检查值是否是Lua的标识符
     * @param raw 原始字符串
     * @return 是否是标识符
     */
    bool IsLuaIdentifier(const char* raw)noexcept;

    /**
     * @brief 检查给定的值是否是Lua的关键词
     * @param raw 字符串
     * @return 是否是关键词
     */
    bool IsLuaKeyword(const char* raw)noexcept;
    bool IsLuaKeyword(const std::string& raw)noexcept;

    /**
     * @brief 格式化文本
     * @param format 格式化参数
     * @param ... 参数列表
     * @return 格式化输出串
     */
    std::string Format(const char* format, ...)noexcept ET_C_FORMAT_DECL(1, 2);

    /**
     * @brief 格式化文本
     * @param format 格式化参数
     * @param args 参数列表
     * @return 格式化输出串
     */
    std::string FormatV(const char* format, std::va_list args)noexcept;

    /**
     * @brief 去除末尾的空白
     * @param str 字符串
     */
    void TrimEnd(std::string& str)noexcept;

    /**
     * @brief 获取文件名
     * @param path 路径
     * @return 文件名
     *
     * 该方法用于获取路径中文件的文件名
     * 允许以'/'和'\'作路径分隔符
     * 若最后以'/'或'\'结尾则被认定为文件夹，返回空值
     */
    std::string GetFileName(const char* path);

    /**
     * @brief 读取文件
     * @exception IOException 如果出现I/O错误将会抛出异常
     * @param out 输出
     * @param path 文件路径
     */
    void ReadFile(std::string& out, const char* path);

    /**
     * @brief 异常基类
     */
    class Exception :
        public std::exception
    {
    public:
        Exception()noexcept = default;
        Exception(const char* file, int line, const char* func, const char* format, ...) ET_C_FORMAT_DECL(5, 6);
        Exception(const Exception& rhs)noexcept;
        Exception(Exception&& rhs)noexcept;

        Exception& operator=(const Exception& rhs)noexcept;
        Exception& operator=(Exception&& rhs)noexcept;

    public:
        const char* GetFile()const noexcept { return m_pszFile; }
        int GetLine()const noexcept { return m_iLine; }
        const char* GetFunc()const noexcept { return m_pszFunc; }
        const char* GetDescription()const noexcept;

    public:  // for std::exception
        const char* what()const noexcept override;

    private:
        const char* m_pszFile = nullptr;
        int m_iLine = 0;
        const char* m_pszFunc = nullptr;
        std::shared_ptr<std::string> m_pDescription;
    };

    ET_DEFINE_EXCEPTION(InvalidCallException);
    ET_DEFINE_EXCEPTION(InvalidArgumentException);
    ET_DEFINE_EXCEPTION(IOException);
}

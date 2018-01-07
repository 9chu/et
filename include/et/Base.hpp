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

#ifndef _MSC_VER
#define ET_C_FORMAT_DECL(start, args) __attribute__((format(printf, start, args)))
#else
#define ET_C_FORMAT_DECL(start, args)
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

    ET_DEFINE_EXCEPTION(InvalidArgumentException);
}

/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#pragma once
#include "Base.hpp"

/**
 * @brief 解析器抛出错误宏
 *
 * 用于在ParseBase子类中使用
 */
#define ET_PARSE_ERROR(format, ...) \
    do { \
        if (GetReader()) { \
            ET_THROW(ParseErrorException, "%s:%u:%u: " format, GetReader()->GetSourceName(), GetReader()->GetLine(), \
                GetReader()->GetColumn(), ##__VA_ARGS__); \
        } else \
            ET_THROW(ParseErrorException, format, ##__VA_ARGS__); \
    } while (false)

namespace et
{
    /**
     * @brief 解析错误异常
     */
    ET_DEFINE_EXCEPTION(ParseErrorException);

    /**
     * @brief 文本读取器
     *
     * 文本读取器用于在一个缓冲区中进行逐字符的读取和回退操作，通常被解析器使用。
     *
     * 约定：
     *  - 使用UTF-8编码
     *  - 不约束行尾类型
     *  - 不持有字符串的内存
     */
    class TextReader
    {
    public:
        /**
         * @brief 定位信息
         */
        struct Anchor
        {
            const char* SourceName = nullptr;
            size_t Position = 0;
            uint32_t Line = 0;
            uint32_t Column = 0;
        };

    public:
        TextReader()noexcept;
        TextReader(const char* input, const char* name="Unknown");
        TextReader(const char* input, size_t len, const char* name="Unknown");
        TextReader(const TextReader& rhs) = default;

    public:
        /**
         * @brief 获取源名称
         */
        const char* GetSourceName()const noexcept { return m_stSourceName; }

        /**
         * @brief 获取原始缓冲区
         */
        const char* GetBuffer()const noexcept { return m_pszBuffer; }

        /**
         * @brief 获取总长度
         */
        size_t GetLength()const noexcept { return m_ullBufferLength; }

        /**
         * @brief 获取当前的读取位置
         *
         * 指示下一个将要读取的字符的位置。
         */
        size_t GetPosition()const noexcept { return m_uPosition; }

        /**
         * @brief 获取当前行号
         *
         * 行号从1开始。
         */
        uint32_t GetLine()const noexcept { return m_uLine; }

        /**
         * @brief 获取当前列号
         *
         * 指示下一个读取位置的列号。
         */
        uint32_t GetColumn()const noexcept { return m_uColumn; }

        /**
         * @brief 构造定位信息
         * @return 当前状态下的定位信息
         */
        Anchor MakeAnchor()const noexcept
        {
            Anchor anchor;
            anchor.SourceName = GetSourceName();
            anchor.Position = GetPosition();
            anchor.Line = GetLine();
            anchor.Column = GetColumn();
            return anchor;
        }

        /**
         * @brief 判断是否达到了结尾
         */
        bool IsEof()const noexcept { return m_uPosition >= GetLength(); }

        /**
         * @brief 读取一个字符，并提升读取的位置
         * @return 若遇到EOF则返回'\0'，否则返回读取的字符
         */
        char Read()noexcept
        {
            if (IsEof())
                return '\0';

            char ch = m_pszBuffer[m_uPosition];
            ++m_uPosition;
            ++m_uColumn;

            if ((ch == '\r' && Peek() != '\n') || ch == '\n')
            {
                ++m_uLine;
                m_uColumn = 1;
            }
            return ch;
        }

        /**
         * @brief 读取一个字符
         * @return 若遇到EOF则返回'\0'，否则返回读取的字符
         */
        char Peek()noexcept
        {
            if (IsEof())
                return '\0';
            return m_pszBuffer[m_uPosition];
        }

        /**
         * @brief 回退一个字符
         * @return 操作是否有效，若越界返回false。
         *
         * Back的代价较大，应当避免经常调用。
         */
        bool Back()noexcept;

    private:
        const char* m_pszBuffer = nullptr;
        size_t m_ullBufferLength = 0;
        const char* m_stSourceName = nullptr;

        size_t m_uPosition = 0;
        uint32_t m_uLine = 1;
        uint32_t m_uColumn = 1;
    };

    /**
     * @brief 解析器基类
     *
     * 提供了用于手写递归下降解析器的一些基础函数。
     */
    class ParserBase
    {
    public:
        /**
         * @brief 打印一个Char为可读形式
         * @param ch 被打印字符
         * @return 可读表示
         */
        static std::string PrintChar(char ch);

    public:
        ParserBase() = default;

    public:
        /**
         * @brief 获取当前的读取器
         * @return 若不存在则返回nullptr
         */
        TextReader* GetReader()noexcept { return m_pReader; }

        /**
         * @brief 执行解析过程
         * @param reader 读取器
         *
         * 需要子类覆盖实现，注意到实现时必须先调用Parser::Run来初始化内部状态。
         */
        virtual void Run(TextReader& reader);

    protected:
        /**
         * @brief 继续读取下一个字符
         * @return 返回当前的字符
         */
        char Next()
        {
            assert(m_pReader);

            char ch = m_pReader->Read();
            assert(ch == c);

            c = m_pReader->Peek();
            return ch;
        }

        /**
         * @brief 回退一个字符
         */
        void Back()
        {
            assert(m_pReader);

            m_pReader->Back();
            c = m_pReader->Peek();
        }

        /**
         * @brief 尝试匹配一个字符
         * @param ch 被匹配字符
         * @return 是否匹配，若能匹配则提升一个位置
         */
        bool TryAcceptOne(char ch)
        {
            if (ch == c)
            {
                Next();
                return true;
            }
            return false;
        }

        /**
         * @brief 尝试匹配一系列字符
         * @param[out] accept 匹配的字符
         * @param ch 试图匹配的字符
         * @return 若成功返回true并提升一个位置
         */
        bool TryAccept(char& accept, char ch)
        {
            if (TryAcceptOne(ch))
            {
                accept = ch;
                return true;
            }
            accept = '\0';
            return false;
        }

        template <typename... Args>
        typename std::enable_if<sizeof...(Args) != 0, bool>::type TryAccept(char& accept, char ch, Args... args)
        {
            if (TryAccept(accept, ch))
                return true;
            return TryAccept(accept, args...);
        }

        /**
         * @brief 匹配并接受一个字符
         * @exception LexicalException 不能匹配时抛出词法异常
         * @param ch 被匹配字符
         */
        void AcceptOne(char ch)
        {
            if (ch != c)
                ET_PARSE_ERROR("Expect %s, but found %s", PrintChar(ch).c_str(), PrintChar(c).c_str());
            Next();
        }

        /**
         * @brief 匹配一系列字符
         * @exception LexicalException 不能匹配时抛出词法异常
         * @param ch 被匹配的字符
         * @return 匹配的字符，即等于ch
         */
        char Accept(char ch)
        {
            if (ch != c)
                ET_PARSE_ERROR("Expect %s, but found %s", PrintChar(ch).c_str(), PrintChar(c).c_str());
            Next();
            return ch;
        }

        template <typename... Args>
        typename std::enable_if<sizeof...(Args) != 0, char>::type Accept(char ch, Args... args)
        {
            if (TryAcceptOne(ch))
                return ch;
            return Accept(args...);
        }

    protected:
        char c = '\0';  // 当前向前看且尚未读取的字符，等同于m_stReader.Peek();

    private:
        TextReader* m_pReader = nullptr;
    };
}


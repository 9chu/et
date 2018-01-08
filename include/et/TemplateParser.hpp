/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#pragma once
#include "Parser.hpp"

namespace et
{
    /**
     * @brief 模板解析器
     *
     * 一个模板由若干字节序列构成。
     * 其中可以加插以"{%"开头和以"%}"结尾的表达式节点。
     * 解析器最终会将整个文档解析，并将表达式节点构造成一颗语法树。
     *
     * 表达式节点中使用"%%"转义"%"，这使得"%%}"可以被正常解析成"%}"而不会导致解析器认为节点被解析完毕。
     * 表达式节点允许含有换行。
     */
    class TemplateParser :
        public ParserBase
    {
    public:
        /**
         * @brief Token类型
         */
        enum class TokenTypes
        {
            Eof,
            Literal,
            Expression,
            End,
            If,
            Else,
            ElseIf,
            For,
            While,
        };

        /**
         * @brief Token
         */
        struct Token
        {
            TokenTypes Type = TokenTypes::Eof;
            std::string Content;  // 表达式或者文本内容
            std::vector<std::string> Args;
            TextReader::Anchor Anchor;
        };

    public:
        /**
         * @brief 检查是否启用修饰
         *
         * 当启用Prettify时，算法会剔除由于插入节点造成的空白行。
         *
         * 例如：
         *  <foo>
         *    {% if a %}
         *      haha
         *    {% end %}
         *  </foo>
         *
         * 会被处理成
         *  <foo>
         *      haha
         *  </foo>
         *
         * 若不启用这一功能，最终会成为
         *  <foo>
         *
         *      haha
         *
         *  </foo>
         *
         * 具体规则：当发现行内只有控制节点，则删除这行（从行前的换行开始删除到行尾的换行前）。
         */
        bool IsPrettifyEnabled()const noexcept { return m_bPrettify; }

        /**
         * @brief 设置是否启用修饰
         */
        void SetPrettifyEnable(bool e)noexcept { m_bPrettify = e; }

        /**
         * @brief 获取最终解析的Token数量
         */
        size_t GetTokenCount()const noexcept { return m_stTokenList.size(); }

        /**
         * @brief 获取Token
         * @param index 索引
         * @return 若索引越界，会返回一个EOF的Token（正常情况下EOF的Token不会在TokenList中出现）
         */
        const Token& GetTokenByIndex(size_t index)const noexcept;
        Token& GetTokenByIndex(size_t index)noexcept;

        /**
         * @brief 清空Token列表
         */
        void Clear()noexcept;

    public:  // for ParserBase
        /**
         * @brief 启动解析
         * @param reader 源
         *
         * 与一些流式解析器不同，这一解析器会把所有Token解析完了置于列表中。
         * 这一做法可能相对会浪费一些内存，但是为了方便实现Prettify只能这么做。
         */
        void Run(TextReader& reader)override;

    private:
        void SkipBlank();
        bool TryAcceptIdentifierOrKeyword(std::string& out);

        bool ParseOuter(Token& result);
        void ParseInner(Token& result);

        void Prettify();

    private:
        bool m_bPrettify = true;

        // 临时变量
        std::string m_stTmpBuffer;

        // 解析器输出
        std::vector<Token> m_stTokenList;
    };
}

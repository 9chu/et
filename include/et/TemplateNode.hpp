/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#pragma once
#include "LuaHelper.hpp"
#include "TemplateParser.hpp"

namespace et
{
    /**
     * @brief 模板渲染错误
     */
    ET_DEFINE_EXCEPTION(RenderException);

    /**
     * @brief Lua运行时错误
     */
    ET_DEFINE_EXCEPTION(LuaRuntimeException);

    /**
     * @brief 模板节点类型
     */
    enum class TemplateNodeTypes
    {
        Text,
        Block,
        Expression,
        If,
        Else,
        For,
        While,
    };

    /**
     * @brief 模板节点基类
     *
     * 定义了从模板解析出来的节点的基类。
     */
    class TemplateNodeBase
    {
    public:
        virtual ~TemplateNodeBase() = default;

    public:
        /**
         * @brief 获取节点类型
         */
        virtual TemplateNodeTypes GetType()const noexcept = 0;

        /**
         * @brief 渲染节点
         * @param builder 输出字符串
         * @param L LUA环境
         */
        virtual void Render(std::string& builder, lua_State* L)const = 0;
    };

    /**
     * @brief 模板文本节点
     *
     * 定义了渲染一段文本的操作。
     * 不持有文本内容，仅引用。
     */
    class TemplateTextNode :
        public TemplateNodeBase
    {
    public:
        TemplateTextNode(const char* start, size_t len);

    public:  // for TemplateNodeBase
        TemplateNodeTypes GetType()const noexcept override;
        void Render(std::string& builder, lua_State* L)const override;

    private:
        const char* m_pszStart = nullptr;
        size_t m_ullLength = 0;
    };

    /**
     * @brief 模板块节点
     *
     * 用于存储一系列子节点。
     */
    class TemplateBlockNode :
        public TemplateNodeBase
    {
    public:
        TemplateBlockNode() = default;
        TemplateBlockNode(const TemplateBlockNode& rhs) = delete;
        TemplateBlockNode(TemplateBlockNode&& rhs)noexcept;

        TemplateBlockNode& operator=(const TemplateBlockNode& rhs) = delete;
        TemplateBlockNode& operator=(TemplateBlockNode&& rhs)noexcept;

    public:
        /**
         * @brief 获取子节点个数
         */
        size_t GetCount()const noexcept { return m_vecNodes.size(); }

        /**
         * @brief 访问节点
         * @param idx 索引
         * @return 若越界返回nullptr
         */
        TemplateNodeBase* GetByIndex(size_t idx)const noexcept;

        /**
         * @brief 追加子节点
         * @param p 子节点
         */
        void Append(std::unique_ptr<TemplateNodeBase>&& p);

    public:  // for TemplateNodeBase
        TemplateNodeTypes GetType()const noexcept override;
        void Render(std::string& builder, lua_State* L)const override;

    private:
        std::vector<std::unique_ptr<TemplateNodeBase>> m_vecNodes;
    };

    /**
     * @brief 表达式节点
     *
     * 定义了一个表达式动作。
     * 当表达式输出多个值时会直接拼接成一个字符串。
     */
    class TemplateExpressionNode :
        public TemplateNodeBase
    {
    public:
        TemplateExpressionNode(const char* source, uint32_t line, const char* start, size_t len);

    public:  // for TemplateNodeBase
        TemplateNodeTypes GetType()const noexcept override;
        void Render(std::string& builder, lua_State* L)const override;

    private:
        const char* m_pszSource = nullptr;
        uint32_t m_uLine = 0;
        std::string m_stExpression;
    };

    /**
     * @brief 构建语法树
     * @param parser 解析器
     * @return 构造结果
     */
    std::unique_ptr<TemplateBlockNode> BuildRootNode(const TemplateParser& parser);
}

/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#pragma once
#include "TemplateParser.hpp"

#include <lua.hpp>

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
        IfElse,
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
        TemplateNodeBase* GetParent()const noexcept { return m_pParent; }
        void SetParent(TemplateNodeBase* parent) { m_pParent = parent; }

    public:
        /**
         * @brief 获取节点类型
         */
        virtual TemplateNodeTypes GetType()const noexcept = 0;

        /**
         * @brief 获取节点数量
         */
        virtual size_t GetNodeCount()const noexcept;

        /**
         * @brief 通过索引获取节点
         */
        virtual TemplateNodeBase* GetNodeByIndex(size_t index)const noexcept;

        /**
         * @brief 寻找节点
         * @param node 节点指针
         * @return 索引，若不存在返回static_cast<size_t>(-1)
         */
        virtual size_t FindNode(TemplateNodeBase* node)const noexcept;

        /**
         * @brief 追加子节点
         * @param p 子节点
         */
        virtual void AppendNode(std::unique_ptr<TemplateNodeBase>&& p);

        /**
         * @brief 移除子节点
         * @param index 索引
         * @return 是否成功
         */
        virtual bool RemoveNode(size_t index)noexcept;

        /**
         * @brief 渲染节点
         * @param builder 输出字符串
         * @param L LUA环境
         * @param env 环境Table索引，当0时不设置ENV
         */
        virtual void Render(std::string& builder, lua_State* L, int env=0)const = 0;

    protected:
        TemplateNodeBase* m_pParent = nullptr;
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
        TemplateTextNode(std::string&& content);

    public:  // for TemplateNodeBase
        TemplateNodeTypes GetType()const noexcept override;
        void Render(std::string& builder, lua_State* L, int env)const override;

    private:
        std::string m_stContent;
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

    public:  // for TemplateNodeBase
        TemplateNodeTypes GetType()const noexcept override;
        size_t GetNodeCount()const noexcept override;
        TemplateNodeBase* GetNodeByIndex(size_t index)const noexcept override;
        size_t FindNode(TemplateNodeBase* node)const noexcept override;
        void AppendNode(std::unique_ptr<TemplateNodeBase>&& p)override;
        bool RemoveNode(size_t index)noexcept override;
        void Render(std::string& builder, lua_State* L, int env)const override;

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
        TemplateExpressionNode(const char* source, uint32_t line, std::string&& expr);

    public:  // for TemplateNodeBase
        TemplateNodeTypes GetType()const noexcept override;
        void Render(std::string& builder, lua_State* L, int env)const override;

    private:
        const char* m_pszSource = nullptr;
        uint32_t m_uLine = 0;
        std::string m_stExpression;
    };

    /**
     * @brief If节点
     */
    class TemplateIfNode :
        public TemplateNodeBase
    {
        friend class TemplateIfElseNode;

    public:
        TemplateIfNode(const char* source, uint32_t line, std::string&& expr);
        TemplateIfNode(const TemplateIfNode& rhs) = delete;
        TemplateIfNode(TemplateIfNode&& rhs)noexcept;

        TemplateIfNode& operator=(const TemplateIfNode& rhs) = delete;
        TemplateIfNode& operator=(TemplateIfNode&& rhs)noexcept;

    protected:
        TemplateIfNode(const char* source, uint32_t line);

    public:
        /**
         * @brief 获取子节点个数
         */
        size_t GetTrueBranchNodeCount()const noexcept { return m_vecTrueBranchNodes.size(); }

        /**
         * @brief 访问节点
         * @param idx 索引
         * @return 若越界返回nullptr
         */
        TemplateNodeBase* GetTrueBranchNodeByIndex(size_t idx)const noexcept;

    public:  // for TemplateNodeBase
        TemplateNodeTypes GetType()const noexcept override;
        size_t GetNodeCount()const noexcept override;
        TemplateNodeBase* GetNodeByIndex(size_t index)const noexcept override;
        size_t FindNode(TemplateNodeBase* node)const noexcept override;
        void AppendNode(std::unique_ptr<TemplateNodeBase>&& p)override;
        bool RemoveNode(size_t index)noexcept override;
        void Render(std::string& builder, lua_State* L, int env)const override;

    protected:
        const char* m_pszSource = nullptr;
        uint32_t m_uLine = 0;
        std::string m_stExpression;

        std::vector<std::unique_ptr<TemplateNodeBase>> m_vecTrueBranchNodes;
    };

    /**
     * @brief IfElse节点
     *
     * If-Else节点由If节点扩展而来。
     */
    class TemplateIfElseNode :
        public TemplateIfNode
    {
    public:
        TemplateIfElseNode(TemplateIfNode& origin);
        TemplateIfElseNode(const TemplateIfElseNode& rhs) = delete;
        TemplateIfElseNode(TemplateIfElseNode&& rhs)noexcept;

        TemplateIfElseNode& operator=(const TemplateIfElseNode& rhs) = delete;
        TemplateIfElseNode& operator=(TemplateIfElseNode&& rhs)noexcept;

    public:
        /**
         * @brief 获取子节点个数
         */
        size_t GetFalseBranchNodeCount()const noexcept { return m_vecFalseBranchNodes.size(); }

        /**
         * @brief 访问节点
         * @param idx 索引
         * @return 若越界返回nullptr
         */
        TemplateNodeBase* GetFalseBranchNodeByIndex(size_t idx)const noexcept;

    public:  // for TemplateNodeBase
        TemplateNodeTypes GetType()const noexcept override;
        size_t GetNodeCount()const noexcept override;
        TemplateNodeBase* GetNodeByIndex(size_t index)const noexcept override;
        size_t FindNode(TemplateNodeBase* node)const noexcept override;
        void AppendNode(std::unique_ptr<TemplateNodeBase>&& p)override;
        bool RemoveNode(size_t index)noexcept override;
        void Render(std::string& builder, lua_State* L, int env)const override;

    private:
        std::vector<std::unique_ptr<TemplateNodeBase>> m_vecFalseBranchNodes;
    };

    /**
     * @brief While节点
     */
    class TemplateWhileNode :
        public TemplateNodeBase
    {
    public:
        TemplateWhileNode(const char* source, uint32_t line, std::string&& expr);
        TemplateWhileNode(const TemplateWhileNode& rhs) = delete;
        TemplateWhileNode(TemplateWhileNode&& rhs)noexcept;

        TemplateWhileNode& operator=(const TemplateWhileNode& rhs) = delete;
        TemplateWhileNode& operator=(TemplateWhileNode&& rhs)noexcept;

    public:  // for TemplateNodeBase
        TemplateNodeTypes GetType()const noexcept override;
        size_t GetNodeCount()const noexcept override;
        TemplateNodeBase* GetNodeByIndex(size_t index)const noexcept override;
        size_t FindNode(TemplateNodeBase* node)const noexcept override;
        void AppendNode(std::unique_ptr<TemplateNodeBase>&& p)override;
        bool RemoveNode(size_t index)noexcept override;
        void Render(std::string& builder, lua_State* L, int env)const override;

    private:
        const char* m_pszSource = nullptr;
        uint32_t m_uLine = 0;
        std::string m_stExpression;

        std::vector<std::unique_ptr<TemplateNodeBase>> m_vecNodes;
    };

    /**
     * @brief For节点
     */
    class TemplateForNode :
        public TemplateNodeBase
    {
    public:
        TemplateForNode(const char* source, uint32_t line, std::string&& expr, std::vector<std::string>&& args);
        TemplateForNode(const TemplateWhileNode& rhs) = delete;
        TemplateForNode(TemplateForNode&& rhs)noexcept;

        TemplateForNode& operator=(const TemplateForNode& rhs) = delete;
        TemplateForNode& operator=(TemplateForNode&& rhs)noexcept;

    public:  // for TemplateNodeBase
        TemplateNodeTypes GetType()const noexcept override;
        size_t GetNodeCount()const noexcept override;
        TemplateNodeBase* GetNodeByIndex(size_t index)const noexcept override;
        size_t FindNode(TemplateNodeBase* node)const noexcept override;
        void AppendNode(std::unique_ptr<TemplateNodeBase>&& p)override;
        bool RemoveNode(size_t index)noexcept override;
        void Render(std::string& builder, lua_State* L, int env)const override;

    private:
        const char* m_pszSource = nullptr;
        uint32_t m_uLine = 0;
        std::string m_stExpression;
        std::vector<std::string> m_vecArgs;

        std::vector<std::unique_ptr<TemplateNodeBase>> m_vecNodes;
    };

    /**
     * @brief 构建语法树
     * @param parser 解析器
     * @return 构造结果
     *
     * 操作完成后Parser内的Token会被清空。
     */
    std::unique_ptr<TemplateBlockNode> BuildRootNode(TemplateParser& parser);
}

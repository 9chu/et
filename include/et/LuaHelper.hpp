/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#pragma once
#include "Base.hpp"

#include <lua.hpp>

namespace et
{
    namespace LuaHelper
    {
        /**
         * @brief 检查值是否是Lua的标识符
         * @param raw 原始字符串
         * @return 是否是标识符
         */
        bool IsIdentifier(const char* raw)noexcept;

        /**
         * @brief 检查给定的值是否是Lua的关键词
         * @param raw 字符串
         * @return 是否是关键词
         */
        bool IsKeyword(const char* raw)noexcept;
        bool IsKeyword(const std::string& raw)noexcept;

        /**
         * @brief 字符串转为Lua字面量
         * @param raw 原始字符串
         * @return 转义结果
         *
         * 方法会对输入的字符串进行转义：
         *  - 加上双引号
         *  - 替换转义字符（包括\a \b \f \n \r \t \v 单引号 双引号）
         */
        std::string BuildString(const char* raw);

        /**
         * @brief 打印Lua值类型为字面量
         * @exception InvalidArgumentException 当出现不支持类型时抛出异常
         * @param builder 输出字符串
         * @param L Lua栈
         * @param idx 值索引
         * @return 即builder的引用
         *
         * 方法会转义并打印Lua的nil/boolean/number/string/table类型的值，如果出现其他类型会抛出异常。
         */
        std::string& BuildValue(std::string& builder, lua_State* L, int idx);
    }
}

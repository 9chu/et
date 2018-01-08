/**
 * @file
 * @author chu
 * @date 18/1/8
 */
#pragma once
#include <string>

#include <lua.hpp>

namespace et
{
    /**
     * @brief 从文本渲染
     * @param[out] out 渲染结果输出
     * @param L 虚拟机环境
     * @param input 输入串
     * @param sourceName 源名称
     * @param env 环境Index
     */
    void RenderString(std::string& out, lua_State* L, const char* input, const char* sourceName="Unknown", int env=0);

    /**
     * @brief 从文件渲染
     * @param[out] out 渲染结果输出
     * @param L 虚拟机环境
     * @param path 输入文件路径
     * @param env 环境Index
     * @return 渲染结果
     */
    void RenderFile(std::string& out, lua_State* L, const char* path, int env=0);

    /**
     * @brief 向Lua注册库
     * @param L 虚拟机环境
     * @param name 库名称
     */
    void RegisterLibrary(lua_State* L, const char* name="et");
}

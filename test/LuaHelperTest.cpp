/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <gtest/gtest.h>

#include <et/TemplateNode.hpp>

using namespace std;
using namespace et;

TEST(LuaHelper, IsIdentifier)
{
    EXPECT_EQ(true, IsLuaIdentifier("a"));
    EXPECT_EQ(true, IsLuaIdentifier("z"));
    EXPECT_EQ(true, IsLuaIdentifier("A"));
    EXPECT_EQ(true, IsLuaIdentifier("Z"));
    EXPECT_EQ(true, IsLuaIdentifier("_"));
    EXPECT_EQ(true, IsLuaIdentifier("_a"));
    EXPECT_EQ(true, IsLuaIdentifier("_z"));
    EXPECT_EQ(true, IsLuaIdentifier("_A"));
    EXPECT_EQ(true, IsLuaIdentifier("_Z"));
    EXPECT_EQ(true, IsLuaIdentifier("_0"));
    EXPECT_EQ(true, IsLuaIdentifier("_9"));

    EXPECT_EQ(false, IsLuaIdentifier(""));
    EXPECT_EQ(false, IsLuaIdentifier("0"));
    EXPECT_EQ(false, IsLuaIdentifier("9"));
    EXPECT_EQ(false, IsLuaIdentifier("a "));
    EXPECT_EQ(false, IsLuaIdentifier("a b"));
}

TEST(LuaHelper, BuildString)
{
    EXPECT_EQ("\"\"", LuaHelper::BuildString(""));
    EXPECT_EQ("\"a\"", LuaHelper::BuildString("a"));
    EXPECT_EQ("\"一\"", LuaHelper::BuildString("一"));
    EXPECT_EQ("\"\\a\\b\\f\\n\\r\\t\\v\"", LuaHelper::BuildString("\a\b\f\n\r\t\v"));
    EXPECT_EQ("\"\\\'\\\"\"", LuaHelper::BuildString("\'\""));
}

TEST(LuaHelper, BuildValue)
{
    lua_State* L = luaL_newstate();
    ASSERT_NE(nullptr, L);

    string builder;

    builder.clear();
    lua_settop(L, 0);
    lua_newtable(L);
    EXPECT_EQ("{}", LuaHelper::BuildValue(builder, L, -1));
    EXPECT_EQ(lua_gettop(L), 1);

    builder.clear();
    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "a");
    EXPECT_EQ("{a=1}", LuaHelper::BuildValue(builder, L, -1));
    EXPECT_EQ(lua_gettop(L), 1);

    builder.clear();
    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushnil(L);
    lua_setfield(L, -2, "a");
    EXPECT_EQ("{}", LuaHelper::BuildValue(builder, L, -1));
    EXPECT_EQ(lua_gettop(L), 1);

    builder.clear();
    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "0");
    EXPECT_EQ("{[\"0\"]=1}", LuaHelper::BuildValue(builder, L, -1));
    EXPECT_EQ(lua_gettop(L), 1);

    builder.clear();
    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "_");
    EXPECT_EQ("{_=true}", LuaHelper::BuildValue(builder, L, -1));
    EXPECT_EQ(lua_gettop(L), 1);

    builder.clear();
    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushboolean(L, 0);
    lua_setfield(L, -2, "_");
    EXPECT_EQ("{_=false}", LuaHelper::BuildValue(builder, L, -1));
    EXPECT_EQ(lua_gettop(L), 1);

    /* 这个例子不能这么写，Hash遍历顺序是不确定的
    builder.clear();
    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "a");
    lua_pushinteger(L, 1);
    lua_setfield(L, -2, "b");
    EXPECT_EQ("{b=1, a=0}", LuaHelper::BuildValue(builder, L, -1));
    EXPECT_EQ(lua_gettop(L), 1);
     */

    builder.clear();
    lua_settop(L, 0);
    lua_newtable(L);
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_setfield(L, -2, "a");
    lua_setfield(L, -2, "b");
    EXPECT_EQ("{b={a=0}}", LuaHelper::BuildValue(builder, L, -1));
    EXPECT_EQ(lua_gettop(L), 1);

    builder.clear();
    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushlightuserdata(L, nullptr);
    lua_setfield(L, -2, "a");
    EXPECT_THROW(LuaHelper::BuildValue(builder, L, -1), InvalidArgumentException);
    EXPECT_EQ(lua_gettop(L), 1);

    builder.clear();
    lua_settop(L, 0);
    lua_newtable(L);
    lua_newtable(L);
    lua_pushlightuserdata(L, nullptr);
    lua_pushinteger(L, 1);
    lua_rawset(L, -3);
    lua_setfield(L, -2, "b");
    EXPECT_THROW(LuaHelper::BuildValue(builder, L, -1), InvalidArgumentException);
    EXPECT_EQ(lua_gettop(L), 1);

    lua_close(L);
}

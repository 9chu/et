/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <gtest/gtest.h>

#include <et/TemplateNode.hpp>

using namespace std;
using namespace et;

TEST(TemplateNodeTest, TemplateTextNode)
{
    string builder;

    TemplateTextNode node1(nullptr, 0);
    builder.clear();
    node1.Render(builder, nullptr);
    EXPECT_EQ("", builder);

    TemplateTextNode node2("1", 1);
    builder.clear();
    node2.Render(builder, nullptr);
    EXPECT_EQ("1", builder);

    TemplateTextNode node3("\r\n12345678\n", 11);
    builder.clear();
    node3.Render(builder, nullptr);
    EXPECT_EQ("\r\n12345678\n", builder);

    TemplateTextNode node4("ABCDEFG", 3);
    builder.clear();
    node4.Render(builder, nullptr);
    EXPECT_EQ("ABC", builder);
}

TEST(TemplateNodeTest, TemplateExpressionNode)
{
    lua_State* L = luaL_newstate();
    ASSERT_NE(nullptr, L);

    string expr;
    string builder;

    expr = "";
    TemplateExpressionNode node1("test", 1, nullptr, 0);
    builder.clear();
    node1.Render(builder, L);
    EXPECT_EQ("", builder);

    expr = "nil";
    TemplateExpressionNode node2("test", 1, expr.c_str(), expr.length());
    builder.clear();
    node2.Render(builder, L);
    EXPECT_EQ("", builder);

    expr = "1";
    TemplateExpressionNode node3("test", 1, expr.c_str(), expr.length());
    builder.clear();
    node3.Render(builder, L);
    EXPECT_EQ("1", builder);

    expr = "'hello world'";
    TemplateExpressionNode node4("test", 1, expr.c_str(), expr.length());
    builder.clear();
    node4.Render(builder, L);
    EXPECT_EQ("hello world", builder);

    expr = "'\\t'";
    TemplateExpressionNode node5("test", 1, expr.c_str(), expr.length());
    builder.clear();
    node5.Render(builder, L);
    EXPECT_EQ("\t", builder);

    expr = "true,false";
    TemplateExpressionNode node6("test", 1, expr.c_str(), expr.length());
    builder.clear();
    node6.Render(builder, L);
    EXPECT_EQ("truefalse", builder);

    expr = "1,\"+\",2";
    TemplateExpressionNode node7("test", 1, expr.c_str(), expr.length());
    builder.clear();
    node7.Render(builder, L);
    EXPECT_EQ("1+2", builder);

    expr = "{}";
    TemplateExpressionNode node8("test", 1, expr.c_str(), expr.length());
    builder.clear();
    EXPECT_THROW(node8.Render(builder, L), RenderException);

    lua_close(L);
}

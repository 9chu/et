/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <gtest/gtest.h>

#include <et/TemplateNode.hpp>

using namespace std;
using namespace et;

#define DO_PARSE_AND_BUILD(code) \
    string source = code; \
    TextReader reader(source.c_str(), source.length()); \
    TemplateParser parser; \
    parser.Run(reader); \
    auto root = BuildRootNode(parser); \
    string result; \
    root->Render(result, L, 0);

TEST(TemplateNodeTest, TemplateTextNode)
{
    string builder;

    TemplateTextNode node1("");
    builder.clear();
    node1.Render(builder, nullptr, 0);
    EXPECT_EQ("", builder);

    TemplateTextNode node2("1");
    builder.clear();
    node2.Render(builder, nullptr, 0);
    EXPECT_EQ("1", builder);

    TemplateTextNode node3("\r\n12345678\n");
    builder.clear();
    node3.Render(builder, nullptr, 0);
    EXPECT_EQ("\r\n12345678\n", builder);
}

TEST(TemplateNodeTest, TemplateExpressionNode)
{
    lua_State* L = luaL_newstate();
    ASSERT_NE(nullptr, L);

    string expr;
    string builder;

    expr = "";
    TemplateExpressionNode node1("test", 1, "");
    builder.clear();
    node1.Render(builder, L, 0);
    EXPECT_EQ("", builder);

    expr = "nil";
    TemplateExpressionNode node2("test", 1, std::move(expr));
    builder.clear();
    node2.Render(builder, L, 0);
    EXPECT_EQ("", builder);

    expr = "1";
    TemplateExpressionNode node3("test", 1, std::move(expr));
    builder.clear();
    node3.Render(builder, L, 0);
    EXPECT_EQ("1", builder);

    expr = "'hello world'";
    TemplateExpressionNode node4("test", 1, std::move(expr));
    builder.clear();
    node4.Render(builder, L, 0);
    EXPECT_EQ("hello world", builder);

    expr = "'\\t'";
    TemplateExpressionNode node5("test", 1, std::move(expr));
    builder.clear();
    node5.Render(builder, L, 0);
    EXPECT_EQ("\t", builder);

    expr = "true,false";
    TemplateExpressionNode node6("test", 1, std::move(expr));
    builder.clear();
    node6.Render(builder, L, 0);
    EXPECT_EQ("truefalse", builder);

    expr = "1,\"+\",2";
    TemplateExpressionNode node7("test", 1, std::move(expr));
    builder.clear();
    node7.Render(builder, L, 0);
    EXPECT_EQ("1+2", builder);

    expr = "{}";
    TemplateExpressionNode node8("test", 1, std::move(expr));
    builder.clear();
    EXPECT_THROW(node8.Render(builder, L, 0), RenderException);

    lua_close(L);
}

TEST(TemplateNodeTest, BuildRootNotde)
{
    string builder;

    lua_State* L = luaL_newstate();
    ASSERT_NE(nullptr, L);
    luaL_openlibs(L);

    {
        DO_PARSE_AND_BUILD("{% 1 %}");
        EXPECT_EQ("1", result);
    }

    {
        DO_PARSE_AND_BUILD("{% 1+ 1 %}");
        EXPECT_EQ("2", result);
    }

    {
        DO_PARSE_AND_BUILD("{%nil %}");
        EXPECT_EQ("", result);
    }

    {
        DO_PARSE_AND_BUILD("{% \"123\" %}");
        EXPECT_EQ("123", result);
    }

    {
        DO_PARSE_AND_BUILD("{% \"123\\n\" %}");
        EXPECT_EQ("123\n", result);
    }

    {
        DO_PARSE_AND_BUILD("{% if true %}hello world!{% end %}");
        EXPECT_EQ("hello world!", result);
    }

    {
        DO_PARSE_AND_BUILD("{% if false %}hello world!{% end %}");
        EXPECT_EQ("", result);
    }

    {
        DO_PARSE_AND_BUILD("{% if true %}hello{%else%}world!{% end %}");
        EXPECT_EQ("hello", result);
    }

    {
        DO_PARSE_AND_BUILD("{% if false %}hello{%else%}world!{% end %}");
        EXPECT_EQ("world!", result);
    }

    {
        DO_PARSE_AND_BUILD("{% if false %}hello{%elseif true%}world{%else%}!{% end %}");
        EXPECT_EQ("world", result);
    }

    {
        DO_PARSE_AND_BUILD("{% if false %}hello{%elseif false%}world{%else%}!{% end %}");
        EXPECT_EQ("!", result);
    }

    {
        DO_PARSE_AND_BUILD("{%if nil%}a{%elseif 1%}b{%if true%}c{%else%}d{%end%}e{%elseif true%}f{%else%}g{%end%}");
        EXPECT_EQ("bce", result);
    }

    {
        DO_PARSE_AND_BUILD("{% i = 0 %}{% while i < 3 %}_{% i = i + 1 %}{% end %}");
        EXPECT_EQ("___", result);
    }

    {
        DO_PARSE_AND_BUILD("{% i = 0 %}{% while i < -1 %}_{% i = i + 1 %}{% end %}");
        EXPECT_EQ("", result);
    }

    {
        DO_PARSE_AND_BUILD("{% a={1,2} %}{% for _,v in ipairs(a) %}{% v %}{% end %}");
        EXPECT_EQ("12", result);
    }

    {
        DO_PARSE_AND_BUILD("{% a={1,2}; v=10 %}{%v%}{% for _,v in ipairs(a) %}{%v%}{% end %}{% v %}");
        EXPECT_EQ("101210", result);
    }

    {
        DO_PARSE_AND_BUILD("{% a={2,3}; v=10 %}{%v%}{% for _ in ipairs(a) %}{%_%}{% end %}{% v %}");
        EXPECT_EQ("101210", result);
    }

    {
        EXPECT_THROW(DO_PARSE_AND_BUILD("{% while true %}{% non_exists() %}{% end %}"), LuaRuntimeException);
    }

    {
        EXPECT_THROW(DO_PARSE_AND_BUILD("{%a={2,3}%}{%for _ in ipairs(a)%}{%foo(v)%}{%end%}"), LuaRuntimeException);
    }

    {
        EXPECT_THROW(DO_PARSE_AND_BUILD("{% if %}123"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE_AND_BUILD("{% end %}123"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE_AND_BUILD("{% if %}123{% end %} {% end %}"), ParseErrorException);
    }

    lua_close(L);
}

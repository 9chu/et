/**
 * @file
 * @author chu
 * @date 2018/1/7
 */
#include <gtest/gtest.h>

#include <et/TemplateParser.hpp>

using namespace std;
using namespace et;

#define DO_PARSE(code) \
    string source = code; \
    TextReader reader(source.c_str(), source.length()); \
    TemplateParser parser; \
    parser.Run(reader);

TEST(TemplateParserTest, ParseOuter)
{
    {
        DO_PARSE("");
        EXPECT_EQ(0ull, parser.GetTokenCount());
    }

    {
        DO_PARSE("1");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Literal, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("1", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Literal, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("{", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{{");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Literal, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("{{", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("123{123");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Literal, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("123{123", parser.GetTokenByIndex(0).Content);
    }

    {
        EXPECT_THROW(DO_PARSE("{%"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{{%"), ParseErrorException);
    }
}

TEST(TemplateParserTest, ParseInner)
{
    {
        DO_PARSE("{%1%}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("1", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% 1 %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("1", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{%1 %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("1", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{%%}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% % %%% %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("% %%", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{%%%%%%}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("%%", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{%%1%}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("%1", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% end %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::End, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% if x%}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::If, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("x", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% if %x%}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::If, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("%x", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% else %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Else, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% elseif x %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::ElseIf, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("x", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% while b %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::While, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("b", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% ifx %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("ifx", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% for a in b %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::For, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(1ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("a", parser.GetTokenByIndex(0).Args[0]);
        EXPECT_EQ("b", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% for a,b in c %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::For, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(2ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("a", parser.GetTokenByIndex(0).Args[0]);
        EXPECT_EQ("b", parser.GetTokenByIndex(0).Args[1]);
        EXPECT_EQ("c", parser.GetTokenByIndex(0).Content);
    }

    {
        DO_PARSE("{% for a , b in c %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::For, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(2ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("a", parser.GetTokenByIndex(0).Args[0]);
        EXPECT_EQ("b", parser.GetTokenByIndex(0).Args[1]);
        EXPECT_EQ("c", parser.GetTokenByIndex(0).Content);
    }

    {
        EXPECT_THROW(DO_PARSE("{% end %"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{% end %%"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{% end "), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{% end x %}"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{% else x %}"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{% if%}"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{% if %}"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{% for in %}"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{% for a, in %}"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{% for in %}"), ParseErrorException);
    }

    {
        EXPECT_THROW(DO_PARSE("{% for if in b %}"), ParseErrorException);
    }
}

TEST(TemplateParserTest, Mixed)
{
    {
        DO_PARSE("a{%1%}b");
        EXPECT_EQ(3ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Literal, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("a", parser.GetTokenByIndex(0).Content);
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(1).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(1).Args.size());
        EXPECT_EQ("1", parser.GetTokenByIndex(1).Content);
        EXPECT_EQ(TemplateParser::TokenTypes::Literal, parser.GetTokenByIndex(2).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(2).Args.size());
        EXPECT_EQ("b", parser.GetTokenByIndex(2).Content);
    }

    {
        DO_PARSE("a{{%1%}b");
        EXPECT_EQ(3ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Literal, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("a{", parser.GetTokenByIndex(0).Content);
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(1).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(1).Args.size());
        EXPECT_EQ("1", parser.GetTokenByIndex(1).Content);
        EXPECT_EQ(TemplateParser::TokenTypes::Literal, parser.GetTokenByIndex(2).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(2).Args.size());
        EXPECT_EQ("b", parser.GetTokenByIndex(2).Content);
    }

    {
        DO_PARSE("a{{%1%a%%%}b");
        EXPECT_EQ(3ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Literal, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("a{", parser.GetTokenByIndex(0).Content);
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(1).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(1).Args.size());
        EXPECT_EQ("1%a%", parser.GetTokenByIndex(1).Content);
        EXPECT_EQ(TemplateParser::TokenTypes::Literal, parser.GetTokenByIndex(2).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(2).Args.size());
        EXPECT_EQ("b", parser.GetTokenByIndex(2).Content);
    }

    {
        DO_PARSE("{% { % {% %}");
        EXPECT_EQ(1ull, parser.GetTokenCount());
        EXPECT_EQ(TemplateParser::TokenTypes::Expression, parser.GetTokenByIndex(0).Type);
        EXPECT_EQ(0ull, parser.GetTokenByIndex(0).Args.size());
        EXPECT_EQ("{ % {%", parser.GetTokenByIndex(0).Content);
    }

    {
        EXPECT_THROW(DO_PARSE("1{% {% % }"), ParseErrorException);
    }
}

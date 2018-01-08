# et

简单的文本模板渲染器。

使用C++编写核心逻辑，配合LUA使用。

[PYTHON版本](https://github.com/9chu/et-py)

## Example

```lua
et = require("et")
et.render_string("{%a%} {%b%}", {a="hello", b="world!"})
```

OUTPUT:

```
hello world!
```

## 语法

- 支持 for ... in ... end 迭代器语法
    - 例如：`{% for a in et.range(0, 1) %}{% a %}{% end %}`
- 支持 while ... end 语法
    - 例如：`{% a = 10 %}{% while i < a %}{% i %}{% i = i + 1 %}{% end %}`
- 支持 if ... elseif ... else ... end 条件语法
- 支持渲染一般表达式
- 支持渲染时自动剔除纯表达式产生的空白行

## API

- et.render_string(input: string, [sourceName: string], [env: table]) -> string

    渲染一段模板文本。

- et.render_file(path: string, [env: table]) -> string

    从文件渲染一段模板文本。

- et.dump_string(value: string) -> string

    将一个Lua字符串转义表示。

- et.dump_value(value: any) -> string

    将一个Lua值（nil、boolean、number、string、table）转义表示。

- et.range(start: number|integer, end: number|integer, step: number|integer) -> iter: func, nil, init: number|integer

    构造一个迭代器。

## LICENSE

MIT LICENSE

# C--LanguageParser
 The C-- language parser.

LR(1)-codes 目录内是第三次实验（自下而上的语法分析）的所有内容，主要有下列文件：

1. util.h
编译器各组件的公共头文件，主要是定义了各种类型（如符号种类、各种表的类型等）。
2. util.cpp
编译器公共头文件中部分工具函数的实现。
3. lexer.h
词法分析器的头文件，主要是定义了 Lexer 类及其所包含的数据成员、函数成员。
4. lexer.cpp
词法分析器的实现文件，主要是实现了 Lexer 类的各个函数。
5. parser.h
语法分析器的头文件，主要是定义了 Parser 类及其所包含的数据成员、函数成员。
6. parser.cpp
语法分析器的实现文件，主要是实现了 Parser 类的各个函数，并且编写了主界面。
7. LL-grammar.txt
C-- 语言的 LL(1) 语法定义文件。
8. LR-grammar.txt
C-- 语言的 LR(1) 语法定义文件。
9. LR-grammar.tbl
预先计算出的 C-- 语言的 LR(1) 分析表文件。
10. test.cmm
C-- 语言测试源文件。
11. out.txt
对 test.cmm 文件进行语法分析得到的分析过程文件。
12. parser
macOS 下编译得到的可执行文件，Windows 下可按照下面的命令自行编译运行。

【附录】
A. 编译方法:
运行下面的命令进行编译
g++ -O3 -std=c++17 util.cpp lexer.cpp parser.cpp -o parser

B. 运行方法:
语法分析器的使用命令为
./parser <filename> [options]}
	其中 filename 指文件名，options 是设置选项，有下列选项:
	a. -h, --help 打印帮助信息；
	b. -LL, --LL(1) 设置分析方式为 LL(1)（默认使用 LR(1) 分析）；
	c. -g, --grammar <filename> 设置输入的语法文件文件名（默认使用内置四则运算语法）；
	d. -ti, --table-in <filename> 设置使用的分析表的文件名；
	e. -to, --table-out <filename> 设置输出的分析表文件名（LL(1) 模式下默认为 LL.tbl，LR(1) 模式下默认为 LR.tbl）；
	f. -o, --output <filename> 设置输出的分析过程文件名（默认使用 out.txt）。
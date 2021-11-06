#ifndef LEXER_H
#define LEXER_H

#include "util.h"

class Lexer
{
    // ------------------------- 公有成员 -------------------------
    public:
        // 扫描字符指针类型
        using ScanPointer = std::string::iterator;

        // ------------------------ 构造函数 ------------------------
        // 默认构造函数
        Lexer();
        // 复制构造(标记删除)
        Lexer(const Lexer & other) = delete;

        // ------------------------ 析构函数 ------------------------
        ~Lexer();

        // ------------------------ 成员函数 ------------------------
        // 打开源文件
        bool openFile(const std::string & srcName);
        // 关闭源文件
        void closeFile();
        // 重新回到开头
        void rewind();
        // 是否已经扫描到结尾
        bool eof();
        // 解析返回下一个 Token
        Types::TokenPair getNextToken();
        // 错误处理
        void errorProcess(const Types::LexerError & error);
        // 获取源文件名称
        std::string getSrcName();
        // 获取输入缓冲区
        std::string getInBuf();
        // 获取当前位置
        Types::FilePos getFilePos();

    // ------------------------- 私有成员 -------------------------
    private:
        // 源文件名
        std::string srcName;
        // 源文件流
        std::fstream srcStream;
        // 输入缓冲区
        std::string inBuf;
        // 缓冲区字符指针
        ScanPointer chPointer;
        // 当前字符在文件中的位置，用于错误处理
        Types::FilePos filePos;

        // 语言的关键字
        const Types::KeywordsTable keywords = \
            Types::KeywordsTable({
                "break", "case", "char", "const", 
                "continue", "default", 
                "do", "double", "else", "enum", "float", 
                "for", "goto", "if", "int", "long", 
                "return", "short", "signed", "sizeof", "static", "struct", 
                "switch", "typedef", "unsigned", 
                "void", "while"
            });
        
        // 运算符
        const Types::OperatorCharTable opChars = \
            Types::OperatorCharTable({
                '!', '%', '&', '*', '+', 
                '-', '.', '/', '<', '=',
                '>', '^', '|', '~'
            });

        // 分隔符
        const Types::DelimCharTable delimChars = \
            Types::DelimCharTable({
                {'"', Types::TokenType::DELIM_DBQUOTE},
                {'#', Types::TokenType::DELIM_SHARP},
                {'\'', Types::TokenType::DELIM_SGQUOTE},
                {'(', Types::TokenType::DELIM_LPAR},
                {')', Types::TokenType::DELIM_RPAR},
                {',', Types::TokenType::DELIM_COMMA},
                {':', Types::TokenType::DELIM_COLON},
                {';', Types::TokenType::DELIM_SEMICOLON},
                {'[', Types::TokenType::DELIM_LSQBRACKET},
                {']', Types::TokenType::DELIM_RSQBRACKET},
                {'{', Types::TokenType::DELIM_LCURBRACE},
                {'}', Types::TokenType::DELIM_RCURBRACE},
                {'?', Types::TokenType::DELIM_QUESTION} });

        // ------------------------ 成员函数 ------------------------
        // 判断是否是关键字
	    std::pair<bool, size_t> isKeyword(const std::string & token);

        // 判断是否是运算符字符
        bool isOpChar(const char & c);

        // 判断是否是分隔符字符
        bool isDelimChar(const char & c);
};

#endif
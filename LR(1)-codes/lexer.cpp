#include "lexer.h"

// 构造函数
Lexer::Lexer()
: chPointer(inBuf.begin()), filePos(std::make_pair<size_t, size_t>(0, 1)) {}

// 析构函数
Lexer::~Lexer() 
{
    closeFile();
}

// 关联文件
bool Lexer::openFile(const std::string & srcName)
{
    this -> srcName = srcName;
    // 打开源文件
    srcStream.open(srcName, std::ios_base::in);
    return srcStream.is_open();
}

// 关闭文件
void Lexer::closeFile()
{
    if(srcStream.is_open())
    {
        srcStream.close();
        srcName = "";
    }
}

// 回到文件开头
void Lexer::rewind()
{
    srcStream.clear();
    srcStream.seekg(0);
    inBuf = "";
    chPointer = inBuf.begin();
    filePos = std::make_pair<size_t, size_t>(0, 1);
}

// 判断词法分析是否结束
bool Lexer::eof()
{
    // 文件流中没有行没有读取，且输入缓冲区为空或已经分析到结尾
    return srcStream.eof() && (chPointer == inBuf.end() || inBuf.empty());
}

Types::TokenPair Lexer::getNextToken()
{
    // ---------- 工具函数 ----------
    // 从文件流中读一行到缓冲区
    // 读取成功则返回 true,
    // 若已经到达文件流结尾，则返回 false
    auto readLine = [this]() -> bool
    {
        if(srcStream.eof())
        {
            return false;
        }
        std::getline(srcStream, inBuf);
        // 增加行数
        filePos.first++;
        // 回到行首
        chPointer = inBuf.begin();
        filePos.second = 1;
        return true;
    };

    // 扫描指针向前推进一个字符
    // 前进成功则返回 true,
    // 若已经到达文件流尾部，则返回 false
    auto goForward = [this, readLine]() -> bool
    {
        // 仍然是保证缓冲区不为空
        while(inBuf.empty() || chPointer == inBuf.end())
        {
            if(!readLine())
            {
                return false;
            }
        }
        chPointer++;
        filePos.second++;
        return true;
    };

    // 向前看 k 个字符，但指针不往前推进
    auto peekForward = [this](size_t k = 1) -> char
    {
        if(chPointer + k > inBuf.end())
        {
            return '\0';
        }
        return *(chPointer + k);
    };

    // 跳过错误单词
    auto skipErrorInToken = [this, goForward]() -> void
    {
        while(!isOpChar(*chPointer) && !isDelimChar(*chPointer)
            && !std::isspace(*chPointer) && chPointer != inBuf.end())
        {
            goForward();
        }
    };

    // ---------- 分析各个类型的函数 ----------
    // 处理标识符和关键字
    auto processIDKWD = [this, goForward]() -> Types::TokenPair
    {
        // 先设置成标识符类型
        Types::TokenType tokenType = Types::TokenType::IDENTIFIER;
        std::string token = "";
        // 如果仍然是下划线、字母或数字
        while(chPointer != inBuf.end() 
            && (std::isalnum(*chPointer) || *chPointer == '_'))
        {
            token.push_back(*chPointer);
            goForward();
        }
        // 标识符还需判定是否为关键字
        auto keywordResult = Lexer::isKeyword(token);
        // 是关键字，则把关键字的序号作为二元组的内容
        if(keywordResult.first)
        {
            tokenType = Types::TokenType::KEYWORD;
            return std::make_pair(tokenType, std::any(token));
        }
        // 否则就是普通的标识符，需要填写标识符表
        // 判定是否出现过同名标识符
        auto idResult = Shared::inIDTable(token);
        // 没出现过，则插入表中
        if(!idResult.first)
        {
            Shared::idTable.push_back(token);
            return std::make_pair(tokenType, std::any(Shared::idTable.size() - 1));
        }
        return std::make_pair(tokenType, std::any(idResult.second));
    };

    // 处理常数
    auto processNUMCONST = [this, goForward, skipErrorInToken]() -> Types::TokenPair
    {
        // 初始化状态
        Types::TokenType tokenType = Types::TokenType::INT_CONST;
        std::string token = "";

        enum class NUM_STATE
        {
            INT_INIT, INT_GOT_0, INT_GOT_1TO9,
            FRAC, EXP_INIT, EXP_GOT_SGN, EXP_NUM,
            DONE
        };

        NUM_STATE state = NUM_STATE::INT_INIT;
        while(tokenType != Types::TokenType::ERROR)
        {
            // 读取第一个字符
            if(state == NUM_STATE::INT_INIT)
            {
                // 第一个字符为 0 -> 类型为整型，进入读到零的分析部分
                if(*chPointer == '0')
                {
                    token.push_back(*chPointer);
                    state = NUM_STATE::INT_GOT_0;
                    goForward();
                }
                // 第一个字符为 1-9 -> 类型为整型，进入读到 1-9 的分析部分
                else if(*chPointer >= '1' && *chPointer <= '9')
                {
                    token.push_back(*chPointer);
                    state = NUM_STATE::INT_GOT_1TO9;
                    goForward();
                }
                // 应该不会发生
                else
                {
                    tokenType = Types::TokenType::ERROR;
                    break;
                }
            }
            // 数字部分第一个字符为 0 的情况
            else if(state == NUM_STATE::INT_GOT_0)
            {
                // 接受 . -> 类型为浮点型，进入小数部分分析
                if(*chPointer == '.')
                {
                    token.push_back(*chPointer);
                    tokenType = Types::TokenType::FLOAT_CONST;
                    state = NUM_STATE::FRAC;
                    goForward();
                }
                // 运算符或分隔符 -> 类型为整型，分析结束
                else if(Lexer::isOpChar(*chPointer) 
                    || Lexer::isDelimChar(*chPointer)
                    || std::isspace(*chPointer) 
                    || inBuf.empty() || chPointer == inBuf.end())
                {
                    tokenType = Types::TokenType::INT_CONST;
                    state = NUM_STATE::DONE;
                    break;
                }
                // 读到 E|e -> 类型为整型，进入指数部分分析
                else if(*chPointer == 'E' || *chPointer == 'e')
                {
                    token.push_back(*chPointer);
                    state = NUM_STATE::EXP_INIT;
                    goForward();
                }
                // 其它字母数字不接受
                else
                {
                    tokenType = Types::TokenType::ERROR;
                    break;
                }
            }
            // 得到的第一个字符为 1-9
            else if(state == NUM_STATE::INT_GOT_1TO9)
            {
                // 读到 0-9 -> 类型为整型，保持
                if(std::isdigit(*chPointer))
                {
                    //state = NUM_STATE::INT_GOT_1TO9;
                    token.push_back(*chPointer);
                    goForward();
                }
                // 接受 . -> 类型为浮点型，进入小数部分分析
                else if(*chPointer == '.')
                {
                    token.push_back(*chPointer);
                    tokenType = Types::TokenType::FLOAT_CONST;
                    state = NUM_STATE::FRAC;
                    goForward();
                }
                // 运算符或分隔符 -> 类型为整型，分析结束
                else if(Lexer::isOpChar(*chPointer) 
                    || Lexer::isDelimChar(*chPointer)
                    || *chPointer == ' ' 
                    || inBuf.empty() || chPointer == inBuf.end())
                {
                    tokenType = Types::TokenType::INT_CONST;
                    state = NUM_STATE::DONE;
                    break;
                }
                // 读到 E|e -> 类型为整型，进入指数部分分析
                else if(*chPointer == 'E' || *chPointer == 'e')
                {
                    token.push_back(*chPointer);
                    state = NUM_STATE::EXP_INIT;
                    goForward();
                }
                // 其它字母数字不接受
                else
                {
                    tokenType = Types::TokenType::ERROR;
                    break;
                }
            }
            // 分析小数部分
            else if(state == NUM_STATE::FRAC)
            {
                // 读取 0-9 -> 类型为浮点数，继续分析
                if(std::isdigit(*chPointer))
                {
                    token.push_back(*chPointer);
                    goForward();
                }
                // 运算符或分隔符 -> 类型为浮点型，分析结束
                else if(Lexer::isOpChar(*chPointer) 
                    || Lexer::isDelimChar(*chPointer)
                    || *chPointer == ' ' 
                    || inBuf.empty() || chPointer == inBuf.end())
                {
                    tokenType = Types::TokenType::FLOAT_CONST;
                    state = NUM_STATE::DONE;
                    break;
                }
                // 读到 E|e -> 类型为浮点型，进入指数部分分析
                else if(*chPointer == 'E' || *chPointer == 'e')
                {
                    token.push_back(*chPointer);
                    state = NUM_STATE::EXP_INIT;
                    goForward();
                }
                // 其它字母数字不接受
                else
                {
                    tokenType = Types::TokenType::ERROR;
                    break;
                }
            }
            // 分析指数部分
            else if(state == NUM_STATE::EXP_INIT)
            {
                // 如果读到正负号，加入，转到分析指数符号部分
                if(*chPointer == '+' || *chPointer == '-')
                {
                    token.push_back(*chPointer);
                    state = NUM_STATE::EXP_GOT_SGN;
                    goForward();
                }
                // 如果是数字，直接进入分析指数值部分
                else if(std::isdigit(*chPointer))
                {
                    state = NUM_STATE::EXP_NUM;
                }
                else
                {
                    tokenType = Types::TokenType::ERROR;
                    break;
                }
            }
            // 分析指数符号部分
            else if(state == NUM_STATE::EXP_GOT_SGN)
            {
                if(std::isdigit(*chPointer))
                {
                    token.push_back(*chPointer);
                    state = NUM_STATE::EXP_NUM;
                    goForward();
                }
                // 其它字母数字不接受
                else
                {
                    tokenType = Types::TokenType::ERROR;
                    break;
                }
            }
            else if(state == NUM_STATE::EXP_NUM)
            {
                if(std::isdigit(*chPointer))
                {
                    token.push_back(*chPointer);
                    state = NUM_STATE::EXP_NUM;
                    goForward();
                }
                // 运算符或分隔符 -> 分析结束
                else if(Lexer::isOpChar(*chPointer) 
                    || Lexer::isDelimChar(*chPointer)
                    || std::isspace(*chPointer) 
                    || inBuf.empty() || chPointer == inBuf.end())
                {
                    state = NUM_STATE::DONE;
                    break;
                }
                // 其它字母数字不接受
                else
                {
                    tokenType = Types::TokenType::ERROR;
                    break;
                }
            }
            else
            {
                tokenType = Types::TokenType::ERROR;
                break;
            }
        }

        if(tokenType == Types::TokenType::ERROR || state != NUM_STATE::DONE)
        {
            tokenType = Types::TokenType::ERROR;
            std::string message = "invalid digit '";
            message.push_back(*chPointer);
            message += std::string("' in decimal constant");
            auto result = std::make_pair(
                tokenType, 
                std::any( Types::LexerError(filePos, message) )
                );
            skipErrorInToken();
            return result;
        }
        // 填常数表
        auto constResult = Shared::inConstTable(token);
        // 如果没出现过，则插入
        if(!constResult.first)
        {
            Shared::constTable.push_back(token);
            return std::make_pair(tokenType,
                std::any(Shared::constTable.size() - 1)
            );
        }
        return std::make_pair(tokenType, 
            std::any(constResult.second)
        );
    };

    // 处理字符常量
    auto processCHARCONST = [this, goForward, skipErrorInToken]() -> Types::TokenPair
    {
        Types::TokenType tokenType = Types::TokenType::CHAR_CONST;
        std::string token = "";
        // 将单引号送入
        token.push_back(*chPointer);
        // 下一个字符
        goForward();
        if(chPointer == inBuf.end())
        {
            tokenType = Types::TokenType::ERROR;
            std::string message = "unclosed single quote mark";
            auto result = std::make_pair(tokenType, 
                std::any(Types::LexerError(filePos, message))
            );
            skipErrorInToken();
            return result;
        }
        // 遇到反斜线
        else if(*chPointer == '\\')
        {
            // 送入反斜线
            token.push_back(*chPointer);
            goForward();

            if(chPointer == inBuf.end())
            {
                tokenType = Types::TokenType::ERROR;
                std::string message = "unclosed single quote mark";
                auto result = std::make_pair(tokenType, 
                    std::any(Types::LexerError(filePos, message))
                );
                skipErrorInToken();
                return result;
            }
        }
        token.push_back(*chPointer);
        goForward();
        if(chPointer == inBuf.end())
        {
            tokenType = Types::TokenType::ERROR;
            std::string message = "unclosed single quote mark";
            auto result = std::make_pair(tokenType, 
                std::any(Types::LexerError(filePos, message))
            );
            skipErrorInToken();
            return result;
        }
        // 不是单引号
        else if(*chPointer != '\'')
        {
            tokenType = Types::TokenType::ERROR;
            std::string message = "invalid character constant";
            auto result = std::make_pair(tokenType, 
                std::any(Types::LexerError(filePos, message))
            );
            skipErrorInToken();
            return result;
        }
        else
        {
            token.push_back(*chPointer);
            goForward();
        }
        // 填常数表
        auto constResult = Shared::inConstTable(token);
        // 如果没出现过，则插入
        if(!constResult.first)
        {
            Shared::constTable.push_back(token);
            return std::make_pair(tokenType, 
                std::any(Shared::constTable.size() - 1)
            );
        }
        return std::make_pair(tokenType, 
            std::any(constResult.second)
        );
    };

    // 处理字符串字面量
    auto processSTRLITERAL = [this, goForward, skipErrorInToken]() -> Types::TokenPair
    {
        Types::TokenType tokenType = Types::TokenType::STR_LITERAL;
        std::string token = "";
        // 双引号
        token.push_back(*chPointer);
        goForward();
        while(chPointer != inBuf.end() && !(*chPointer == '"' && *(chPointer - 1) != '\\'))
        {
            token.push_back(*chPointer);
            goForward();
        }
        if(chPointer == inBuf.end())
        {
            tokenType = Types::TokenType::ERROR;
            std::string message = "unclosed double quote mark";
            skipErrorInToken();
            return std::make_pair(tokenType, 
                std::any(Types::LexerError(filePos, message))
            );
        }
        // 双引号进入
        else
        {
            token.push_back(*chPointer);
            goForward();
        }
        // 填常数表
        auto constResult = Shared::inConstTable(token);
        // 如果没出现过，则插入
        if(!constResult.first)
        {
            Shared::constTable.push_back(token);
            return std::make_pair(tokenType,
                std::any(Shared::constTable.size() - 1)
            );
        }
        return std::make_pair(tokenType, 
            std::any(constResult.second)
        );
    };
    
    // 处理各类运算符
    auto processOP = [this, goForward]() -> Types::TokenPair
    {
        // 初始化状态
        Types::TokenType tokenType = Types::TokenType::INIT;
        std::string token = "";
        // -------- 运算符 --------
		// 简单运算符
		// + - * / % > < ! & | ~ ^ . =
		// 复合运算符
		// -> ++ -- << >> <= >= == != && || += -= *= /= %= &= ^= |= <<= >>= ::

        // + ++ +=
        if(*chPointer == '+')
        {
            tokenType = Types::TokenType::OP_ADD;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '+')
            {
                tokenType = Types::TokenType::OP_INC;
                token.push_back(*chPointer);
                goForward();
            }
            else if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_ADDASN;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // - -> -- -=
        else if(*chPointer == '-')
        {
            tokenType = Types::TokenType::OP_SUB;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '>')
            {
                tokenType = Types::TokenType::OP_ARROW;
                token.push_back(*chPointer);
                goForward();
            }
            else if(*chPointer == '-')
            {
                tokenType = Types::TokenType::OP_DEC;
                token.push_back(*chPointer);
                goForward();
            }
            else if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_SUBASN;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // * *=
        else if(*chPointer == '*')
        {
            tokenType = Types::TokenType::OP_MUL;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_MULASN;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // / /=
        else if(*chPointer == '/')
        {
            tokenType = Types::TokenType::OP_DIV;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_DIVASN;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // % %=
        else if(*chPointer == '%')
        {
            tokenType = Types::TokenType::OP_MOD;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_MODASN;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // > >> >= >>=
        else if(*chPointer == '>')
        {
            tokenType = Types::TokenType::OP_GT;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '>')
            {
                tokenType = Types::TokenType::OP_SHR;
                token.push_back(*chPointer);
                goForward();
                if(*chPointer == '=')
                {
                    tokenType = Types::TokenType::OP_SHRASN;
                    token.push_back(*chPointer);
                    goForward();
                }
            }
            else if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_GE;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // < << <= <<=
        else if(*chPointer == '<')
        {
            tokenType = Types::TokenType::OP_LT;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '<')
            {
                tokenType = Types::TokenType::OP_SHL;
                token.push_back(*chPointer);
                goForward();
                if(*chPointer == '=')
                {
                    tokenType = Types::TokenType::OP_SHLASN;
                    token.push_back(*chPointer);
                    goForward();
                }
            }
            else if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_LE;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // ! !=
        else if(*chPointer == '!')
        {
            tokenType = Types::TokenType::OP_LNOT;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_NEQ;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // & && &=
        else if(*chPointer == '&')
        {
            tokenType = Types::TokenType::OP_AND;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '&')
            {
                tokenType = Types::TokenType::OP_LAND;
                token.push_back(*chPointer);
                goForward();
            }
            else if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_ANDASN;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // | || |=
        else if(*chPointer == '|')
        {
            tokenType = Types::TokenType::OP_OR;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '|')
            {
                tokenType = Types::TokenType::OP_LOR;
                token.push_back(*chPointer);
                goForward();
            }
            else if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_ORASN;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // ~
        else if(*chPointer == '~')
        {
            tokenType = Types::TokenType::OP_NOT;
            token.push_back(*chPointer);
            goForward();
        }
        // ^ ^=
        else if(*chPointer == '^')
        {
            tokenType = Types::TokenType::OP_XOR;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_XORASN;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // .
        else if(*chPointer == '.')
        {
            tokenType = Types::TokenType::OP_DOT;
            token.push_back(*chPointer);
            goForward();
        }
        // = ==
        else if(*chPointer == '=')
        {
            tokenType = Types::TokenType::OP_ASN;
            token.push_back(*chPointer);
            goForward();
            if(*chPointer == '=')
            {
                tokenType = Types::TokenType::OP_EQ;
                token.push_back(*chPointer);
                goForward();
            }
        }
        // :: 在分隔符处处理
        return std::make_pair(tokenType, std::any(token));
    };

    // 处理分隔符
    auto processDELIM = [this, goForward, peekForward,
        processCHARCONST, processSTRLITERAL]() -> Types::TokenPair
    {
        // 初始化状态
        Types::TokenType tokenType = Types::TokenType::INIT;
        std::string token = "";
        
        if(*chPointer == ':' && peekForward() == ':') 
        {
            tokenType = Types::TokenType::OP_SCOPE;
            token.push_back(*chPointer);
            goForward();
            token.push_back(*chPointer);
            goForward();
            return std::make_pair(tokenType, std::any(token));
        }
        else if(*chPointer == '\'')
        {
            return processCHARCONST();
        }
        else if(*chPointer == '"')
        {
            return processSTRLITERAL();
        }
        char c = *chPointer;
        token.push_back(c);
        goForward();
        return std::make_pair(Lexer::delimChars.at(c), std::any(token));
    };

    // ---------- 词法分析开始 ----------    
    // ---------- 跳过非实义字符 ----------
    // 如果缓冲区为空或前向指针到达结尾
    while(inBuf.empty() || chPointer == inBuf.end())
    {
        // 尝试读取一行文件
        // 如果最后一行已经读进来了，直接返回
        if(!readLine())
        {
            return std::make_pair(Types::TokenType::ENDOFFILE, std::any("$"));
        }
    }
    // 跳过所有空格，回车等空白符
    while(std::isspace(*chPointer) || *chPointer == '\\')
    {
        if(!goForward())
        {
            return std::make_pair(Types::TokenType::INIT, std::any(nullptr));
        }
    }
    // 处理注释等
    // 单行注释，直接移到行尾
    if(*chPointer == '/' && peekForward() == '/')
    {
        chPointer = inBuf.end();
        filePos.second = inBuf.size() + 1;
    }
    // 多行注释，需要找到匹配的结束符
    else if(*chPointer == '/' && peekForward() == '*')
    {
        while(!(*chPointer == '*' && peekForward() == '/'))
        {
            if(!goForward())
            {
                return std::make_pair(Types::TokenType::INIT, std::any(nullptr));
            }
        }
        // 跳过结束符
        if(!(goForward() && goForward()))
        {
            auto result = std::make_pair(
                Types::TokenType::ERROR, 
                std::any(Types::LexerError(filePos, "unmatched multi-line comment"))
            );
            skipErrorInToken();
            return result;
        }
    }

    // ---------- 处理各类实义字符 ----------
    // 遇到下划线或字母——处理标识符或关键字
    if(std::isalpha(*chPointer) || *chPointer == '_')
    {
        return processIDKWD();
    }
    //  遇到数字——处理常数
    else if(std::isdigit(*chPointer))
    {
        return processNUMCONST();
    }
    // 是运算符
    else if(Lexer::isOpChar(*chPointer))
    {
        return processOP();
    }
    // 是分隔符
    else if(Lexer::isDelimChar(*chPointer))
    {
        return processDELIM();
    }
    // 是空白符
    else if(std::isspace(*chPointer) || inBuf.empty() || chPointer == inBuf.end())
    {
        return std::make_pair(Types::TokenType::INIT, std::any(nullptr));
    }
    else
    {
        auto result = std::make_pair(
                Types::TokenType::ERROR, 
                std::any(Types::LexerError(filePos, "invalid character"))
            );
        skipErrorInToken();
        return result;
    }
}

void Lexer::errorProcess(const Types::LexerError & error)
{
    std::cout << "\033[1m" << srcName << ":" 
        << error.first.first << ":" 
        << error.first.second << ": (Lexer) \033[31merror: \033[0m\033[1m" 
        << error.second << "\033[0m" << std::endl;

    std::cout << "    " << inBuf << std::endl;
    std::cout << "    ";
    for(size_t i = 1; i < error.first.second; i++)
    {
        std::cout << (inBuf[i - 1] == '\t' ? '\t' : ' ');
    }
    std::cout << "\033[1;2m^\033[0m" << std::endl;
}

// 获取文件名
std::string Lexer::getSrcName()
{
    return srcName;
}

// 获取输入缓冲区
std::string Lexer::getInBuf()
{
    return inBuf;
}

// 获取当前文件位置
Types::FilePos Lexer::getFilePos()
{
    return filePos;
}

// 判断是否是关键字
std::pair<bool, size_t> Lexer::isKeyword(const std::string & token)
{
    for(size_t i = 0; i < keywords.size(); i++)
    {
        if(keywords[i] == token)
        {
            return std::pair<bool, size_t>(true, i);
        }
    }
    return std::pair<bool, size_t>(false, keywords.size());
}

// 判断是否是运算符字符
bool Lexer::isOpChar(const char & c)
{
    return opChars.find(c) != opChars.end();
}

// 判断是否是分隔符字符
bool Lexer::isDelimChar(const char & c)
{
    return delimChars.find(c) != delimChars.end();
}

#ifdef INDEPENDENT_LEXER
int main(int argc, char * argv[])
{
    auto printUsage = []() -> void
    {
        std::cout << "Usage:\n  Lexer <filename> [options]" << std::endl;
        std::cout << "Options:\n  -h, --help\t\t\t Print help." << std::endl;
        std::cout << "  -t, --token\t\t\t Set output token file name." << std::endl;
        std::cout << "  -i, --identifier\t\t Set output identifier file name." << std::endl;
        std::cout << "  -c, --const\t\t\t Set output const file name." << std::endl;
    };

    auto outputToken = [](std::ostream & out, Types::TokenPair & token) -> void
    {
        if(token.first == Types::TokenType::INIT)
        {
            out << "INIT." << std::endl;
        }
        else if(token.first == Types::TokenType::ERROR)
        {
            out << "ERROR." << std::endl;
        }
        else if(token.first == Types::TokenType::ENDOFFILE)
        {
            out << "EOF." << std::endl;
        }
        else if(token.first == Types::TokenType::KEYWORD)
        {
            // 输出 token
            out << std::any_cast<std::string>(token.second) << "\t\t(" 
            // 输出 类型
                << Shared::typeStrings.at(token.first) << ", "
            // 输出 值
                << std::any_cast<std::string>(token.second) << ")" << std::endl;
        }
        // 标识符、常量都是返回的表内下标
        else if(token.first == Types::TokenType::IDENTIFIER)
        {
            out << Shared::idTable.at(std::any_cast<size_t>(token.second)) << "\t\t("
                << Shared::typeStrings.at(token.first) << ", "
                << std::any_cast<size_t>(token.second) << ")" << std::endl;
        }
        else if(token.first >= Types::TokenType::INT_CONST 
            && token.first <= Types::TokenType::STR_LITERAL)
        {
            out << Shared::constTable.at(std::any_cast<size_t>(token.second)) << "\t\t("
                << Shared::typeStrings.at(token.first) << ", "
                << std::any_cast<size_t>(token.second) << ")" << std::endl;
        }
        // 运算符和分隔符都是返回的内容
        else if(token.first >= Types::TokenType::OP_ADD 
            && token.first <= Types::TokenType::DELIM_QUESTION)
        {
            out << std::any_cast<std::string>(token.second) << "\t\t("
                << Shared::typeStrings.at(token.first) << ", "
                << std::any_cast<std::string>(token.second) << ")" << std::endl;
        }
        else
        {
            out << "DEFAULT." << std::endl;
        }
    };

    Lexer lexer;
    std::string srcFileName, 
        tokenFileName = "token.txt", 
        idFileName = "id.txt", 
        constFileName = "const.txt";
    // 输出文件流
    std::fstream tokenStream, idStream, constStream;
    
    enum class FlagIndex
    {
        SET_TOKENFILE,
        SET_IDFILE,
        SET_CONSTFILE
    };

    // 设置相关 Flags
    std::bitset<3> setFileFlags = 0;

    if(argc <= 1 || argc % 2 != 0)
    {
        std::cout << "\033[1m(Lexer)\033[0m \033[1;31merror:\033[0m Wrong usage!" << std::endl;
        printUsage();
        exit(1);
    }

    for(int i = 1; i < argc; i++)
    {
        std::string cmd = std::string(argv[i]);
        if(cmd == "-h" || cmd == "--help")
        {
            printUsage();
            exit(0);
        }
        else if(cmd == "-t" || cmd == "--token")
        {
            setFileFlags.set(size_t(FlagIndex::SET_TOKENFILE));
        }
        else if(cmd == "-i" || cmd == "--identifier")
        {
            setFileFlags.set(size_t(FlagIndex::SET_IDFILE));
        }
        else if(cmd == "-c" || cmd == "--const")
        {
            setFileFlags.set(size_t(FlagIndex::SET_CONSTFILE));
        }
        else
        {
            if(i == 1)
            {
                srcFileName = cmd;
            }
            // 设置 token 文件
            if(setFileFlags.test(size_t(FlagIndex::SET_TOKENFILE)))
            {
                tokenFileName = cmd;
                setFileFlags.set(size_t(FlagIndex::SET_TOKENFILE), false);
            }
            else if(setFileFlags.test(size_t(FlagIndex::SET_IDFILE)))
            {
                idFileName = cmd;
                setFileFlags.set(size_t(FlagIndex::SET_IDFILE), false);
            }
            else if(setFileFlags.test(size_t(FlagIndex::SET_CONSTFILE)))
            {
                constFileName = cmd;
                setFileFlags.set(size_t(FlagIndex::SET_CONSTFILE), false);
            }
        }
    }

    // 参数不对
    if(setFileFlags.any())
    {
        std::cout << "\033[1m(Lexer)\033[0m \033[1;31merror:\033[0m Wrong usage!" << std::endl;
        printUsage();
        exit(1);
    }
    // 打不开文件
    if(!lexer.linkFile(srcFileName))
    {
        std::cout << "\033[1m(Lexer)\033[0m \033[1;31merror:\033[0m Can't open file: " << srcFileName << std::endl;
        exit(1);
    }

    std::vector<Types::TokenPair> tokens;
    size_t errorCount = 0;
    while(!lexer.eof())
    {
        auto result = lexer.getNextToken();
        if(result.first == Types::TokenType::ERROR)
        {
            lexer.errorProcess(std::any_cast<Types::LexerError>(result.second));
            errorCount++;
        }
        else if(result.first != Types::TokenType::INIT)
        {
            tokens.emplace_back(result);
        }
    }
    // 错误统计
    if(errorCount > 0)
    {
        std::cout << errorCount << " error(s) generated." << std::endl;
    }

    // 开始输出文件
    tokenStream.open(tokenFileName, std::ios_base::out);
    idStream.open(idFileName, std::ios_base::out);
    constStream.open(constFileName, std::ios_base::out);

    if(!tokenStream.is_open() 
        || !idStream.is_open() 
        || !constStream.is_open())
    {
        std::cout << "\033[1m(Lexer)\033[0m \033[1;31merror:\033[0m Can't open file: " << argv[1] << std::endl;
        exit(1);
    }

    // 输出 token 表
    if(errorCount == 0)
    {
        for(size_t i = 0; i < tokens.size(); i++)
        {
            outputToken(std::cout, tokens[i]);
        }
    }
    for(size_t i = 0; i < tokens.size(); i++)
    {
        outputToken(tokenStream, tokens[i]);
    }
    // 输出标识符表
    for(auto & i : Shared::idTable)
    {
        idStream << i << std::endl;
    }
    // 输出常量表
    for(auto & i : Shared::constTable)
    {
        constStream << i << std::endl;
    }

    tokenStream.close();
    idStream.close();
    constStream.close();

    return 0;
}
#endif
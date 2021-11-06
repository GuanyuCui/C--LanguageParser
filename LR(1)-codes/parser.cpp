#include "parser.h"

Parser::Parser(){}

Parser::~Parser(){}

// 打开文件
bool Parser::openFile(const std::string & srcName)
{
    return lexer.openFile(srcName);
}

// 关闭文件
void Parser::closeFile()
{
    lexer.closeFile();
}

// 读取文法
bool Parser::readGrammar(const std::string & grmFileName)
{
    // 打开文件
    std::fstream grmStream;
    grmStream.open(grmFileName);
    // 打不开文件
    if(!grmStream.is_open())
    {
        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;35mwarning:\033[0m\033[1m Can't open grammar file: " 
            << grmFileName << ", use default grammar instead.\033[0m" << std::endl;
        return true;
    }

    // 四元组
    Grammar::NonTerminals nonTerminals;
    Grammar::Terminals terminals;
    Grammar::StartSymbolType startSymbol;
    Grammar::Productions productions;

    // 循环读取
    while(!grmStream.eof())
    {
        // 一行
        std::string lineString;
        // 读进来一行
        std::getline(grmStream, lineString);
        if(lineString.empty())
        {
            continue;
        }
        // 建立字符串流
        std::stringstream lineStream(lineString);
        // 当前符号
        Grammar::SymbolType nowSymbol;

        // 读取左端
        lineStream >> nowSymbol;
        if(nowSymbol.size() >= 2 && nowSymbol[0] == '/' && nowSymbol[1] == '/')
        {
            continue;
        }
        // 第一行确定开始符号
        if(startSymbol == "")
        {
            startSymbol = nowSymbol;
        }
        // 确定左端
        Grammar::NonTerminalType leftPart = nowSymbol;
        // 左端不能为空
        if(leftPart == "\"\"" || leftPart == "''")
        {
            return false;
        }
        // 左端一定是非终结符
        nonTerminals.insert(leftPart);
        terminals.insert(leftPart);

        // 应该是 ->
        lineStream >> nowSymbol;
        if(nowSymbol != "->")
        {
            return false;
        }

        // 读取右端
        std::vector<Grammar::SymbolType> rightPart;
        while(lineStream >> nowSymbol)
        {
            if(nowSymbol.size() >= 2 && nowSymbol[0] == '/' && nowSymbol[1] == '/')
            {
                break;
            }
            // 空字符
            if(nowSymbol == "\"\"" || nowSymbol == "''")
            {
                nowSymbol = "";
            }
            rightPart.push_back(nowSymbol);
            terminals.insert(nowSymbol);
        }
        // 加入其中
        productions[leftPart].push_back(rightPart);
    }

    // 保留所有符号，然后去掉终结符即为非终结符
    // 开始去掉所有非终结符
    for(const auto & nonTerminal : nonTerminals)
    {
        terminals.erase(nonTerminal);
    }
    // EOF 也是终结符
    terminals.insert(Shared::endOfFileChar);

    // 增广文法
    if(startSymbol == "<EXTEND-GRAMMAR-START>" && productions[startSymbol].size() > 1)
    {
        std::cout << startSymbol << "is reserved." << std::endl;
        return false;
    }
    productions["<EXTEND-GRAMMAR-START>"] = {{startSymbol}};
    startSymbol = "<EXTEND-GRAMMAR-START>";
    nonTerminals.insert(startSymbol);
    grammar = {nonTerminals, terminals, startSymbol, productions};

    return true;
}

// ------------------------ LL(1) 语法分析 ------------------------
// 计算 LL(1) 解析表
bool Parser::calcLL1ParseTable()
{
    // 两个表
    FirstTableType tmpFirstTable;
    FollowTableType tmpFollowTable;

    // 最大迭代次数
    size_t maxIteration = 1e6;

    // 根据当前 FIRST 表，得到一串符号的 FIRST
    auto firstOfSymbols = [this, &tmpFirstTable](const std::vector<Grammar::SymbolType> & symbols) -> std::set<Grammar::TerminalType>
    {
        if(symbols.empty())
        {
            return {""};
        }
        // 一开始，空集合
        std::set<Grammar::TerminalType> s = {};
        for(size_t i = 0; i < symbols.size(); i++)
        {
            std::set<Grammar::TerminalType> firstOfThisSymbol;
            // 如果是终结符
            if(this -> grammar.isTerminal(symbols[i]))
            {
                firstOfThisSymbol.insert(symbols[i]);
            }
            // 查表得到当前符号的 First
            else if(grammar.isNonTerminal(symbols[i]))
            {
                // 没有对应产生式
                if(tmpFirstTable.find(symbols[i]) == tmpFirstTable.end())
                {
                    std::cout << "Exception: In function firstOfSymbols." << std::endl;
                    std::cout << symbols[i] << ": No corresponding production." << std::endl;
                    return {};
                }
                firstOfThisSymbol = tmpFirstTable[symbols[i]];
            }
            else
            {
                std::cout << "Exception: In function firstOfSymbols." << std::endl;
                std::cout << symbols[i] << ": Neither a terminal nor a non-terminal." << std::endl;
                return {};
            }

            // 将除去 空字 的 First 加入
            s.insert(firstOfThisSymbol.begin(), firstOfThisSymbol.end());
            s.erase("");
            // 当前字的 First 不含空，则停止
            if(firstOfThisSymbol.find("") == firstOfThisSymbol.end())
            {
                break;
            }
            // 最后一个还含有空，则加入空
            if(i == symbols.size() - 1)
            {
                s.insert("");
            }
        }
        return s;
    };

    // 计算 FISRT 表
    auto calcFirstTable = [this, maxIteration, &tmpFirstTable, firstOfSymbols]() -> bool
    {
        size_t iter = 0;
        // 初始化
        for(const auto & symbol : grammar.nonTerminals)
        {
            // 各符号都是空集合
            tmpFirstTable[ symbol ] = {};
        }
        // 判断是否有更新
        bool modifiedFlag = true;
        while(modifiedFlag)
        {
            iter++;
            modifiedFlag = false;
            // 对每个产生式
            for(const auto & production : this -> grammar.productions)
            {
                // 左半部
                const auto & leftPart = production.first;
                // 对于右侧每一个产生式
                for(const auto & rightPart : production.second)
                {
                    // 原集合大小
                    size_t oldSize = tmpFirstTable[leftPart].size();
                    // 计算它们的 First
                    std::set<Grammar::TerminalType> firstOfRightPart = firstOfSymbols(rightPart);
                    // 合并集合
                    tmpFirstTable[leftPart].insert(firstOfRightPart.begin(), firstOfRightPart.end());
                    // 增加符号后的大小
                    size_t newSize = tmpFirstTable[leftPart].size();
                    if(newSize != oldSize)
                    {
                        modifiedFlag = true;
                    }
                }
            }
            if(iter > maxIteration)
            {
                return false;
            }
        }
        firstTable = tmpFirstTable;
        return true;
    };

    // 计算 FOLLOW 表
    auto calcFollowTable = [this, maxIteration, &tmpFirstTable, &tmpFollowTable, firstOfSymbols]() -> bool
    {
        size_t iter = 0;
        // 初始化
        for(const auto & symbol : grammar.nonTerminals)
        {
            // 各符号都是空集合
            tmpFollowTable[ symbol ] = {};
        }
        // Follow 表起始字符为结束字符
        tmpFollowTable[grammar.startSymbol] = {Shared::endOfFileChar};
        // 判断是否有更新
        bool modifiedFlag = true;
        while(modifiedFlag)
        {
            iter++;
            modifiedFlag = false;
            // 对每一个产生式
            for(const auto & production : grammar.productions)
            {
                // 计算左部、右部
                const auto & leftPart = production.first;
                for(const auto & rightPart : production.second)
                {
                    // 考虑每个产生式右边的非终结符
                    for(size_t i = 0; i < rightPart.size(); i++)
                    {
                        // 跳过终结符
                        if(grammar.isTerminal(rightPart[i]))
                        {
                            continue;
                        }
                        // 既不是终结符也不是非终结符
                        if(!grammar.isNonTerminal(rightPart[i]))
                        {
                            std::cout << "Exception: In function calcFollowTable." << std::endl;
                            return false;
                        }
                        size_t oldSize = tmpFollowTable[rightPart[i]].size();
                        // 获取右边的符号
                        std::vector<Grammar::SymbolType> rightSymbols(rightPart.begin() + i + 1, rightPart.end());
                        // 计算右边的符号串的 First
                        std::set<Grammar::TerminalType> firstOfRightSymbols = firstOfSymbols(rightSymbols);
                        // 并进去
                        tmpFollowTable[rightPart[i]].insert(firstOfRightSymbols.begin(), firstOfRightSymbols.end());
                        // 如果有空串，则去掉，并且把产生式左边的符号的 Follow 加进去
                        if(tmpFollowTable[rightPart[i]].find("") != tmpFollowTable[rightPart[i]].end())
                        {
                            tmpFollowTable[rightPart[i]].erase("");
                            tmpFollowTable[rightPart[i]].insert(tmpFollowTable[leftPart].begin(), tmpFollowTable[leftPart].end());
                        }
                        // 新的大小
                        size_t newSize = tmpFollowTable[rightPart[i]].size();
                        if(newSize != oldSize)
                        {
                            modifiedFlag = true;
                        }
                    }
                }
            }
            if(iter > maxIteration)
            {
                return false;
            }
        }
        followTable = tmpFollowTable;
        return true;
    };

    // 计算预测分析表
    auto calcParseTable = [this, &tmpFirstTable, &tmpFollowTable, firstOfSymbols]() -> bool
    {
        // 开始填临时分析表
        LL1ParseTableType tmpParseTable;
        // 考虑每一条产生式
        for(const auto & production : grammar.productions)
        {
            // 左侧
            const auto & leftPart = production.first;
            // 右侧每一条
            for(const auto & rightPart : production.second)
            {
                // 产生式
                Grammar::ProductionType p(leftPart, rightPart);
                // 计算右部符号的 First 集合
                const std::set<Grammar::TerminalType> & firstOfRightSymbols = firstOfSymbols(rightPart);
                // 考虑每一个 First 中的符号 a
                for(const auto & firstTerminal : firstOfRightSymbols)
                {
                    // 如果有空串，则要计算左边的 Follow
                    if(firstTerminal == "")
                    {
                        // 左边符号的 Follow
                        const auto & followOfLeftSymbol = followTable[leftPart];
                        //  b in Follow(A), 将 A -> alpha 加入 M[A, b]
                        for(const auto & followTerminal : followOfLeftSymbol)
                        {
                            if(
                                // 如果 M[A, b] 没有值
                                tmpParseTable.find(
                                    std::make_pair(leftPart, followTerminal)
                                ) == tmpParseTable.end()
                            )
                            {
                                // 加入
                                tmpParseTable[std::make_pair(leftPart, followTerminal)] = p;
                            }
                            // 如果有值，但不冲突
                            else if(tmpParseTable[std::make_pair(leftPart, followTerminal)] == p)
                            {

                            }
                            // 冲突，这不是 LL(1) 文法，返回
                            else
                            {
                                std::cout << "Action conflict: " << leftPart << " " << followTerminal << std::endl; 
                                return false;
                            }
                        }
                    }
                    else
                    {
                        if(
                            // 如果 M[A, a] 没有值
                            tmpParseTable.find(
                                std::make_pair(leftPart, firstTerminal)
                            ) == tmpParseTable.end()
                        )
                        {
                            // 加入
                            tmpParseTable[std::make_pair(leftPart, firstTerminal)] = p;
                        }
                        // 如果有值，但不冲突
                        else if(tmpParseTable[std::make_pair(leftPart, firstTerminal)] == p)
                        {

                        }
                        // 冲突，这不是 LL(1) 文法，返回
                        else
                        {
                            std::cout << "Action conflict: " << leftPart << " " << firstTerminal << std::endl; 
                            return false;
                        }
                    }
                }
            }
        }
        this -> LL1parseTable = tmpParseTable;
        return true;
    };

    // 计算 First
    if(!calcFirstTable())
    {
        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Build First-Table failed. (max iteration exceeded)\033[0m " << std::endl;
        return false;
    }
    // 计算 Follow
    if(!calcFollowTable())
    {
        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Build Follow-Table failed. (max iteration exceeded)\033[0m " << std::endl;
        return false;
    }
    // 计算 ParseTable
    if(!calcParseTable())
    {
        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Build Parse-Table failed. (not LL(1) grammar)\033[0m" << std::endl;
        return false;
    }
    return true;
}

// 打印 LL(1) 表格
void Parser::printLL1InternalTables(std::ostream & out)
{
    out << "This is an LL(1) grammar." << std::endl << std::endl;
    this -> printFirstTable(out);
    this -> printFollowTable(out);
    this -> printLL1ParseTable(out);
}

// LL(1) 语法分析
size_t Parser::LL1Parse(std::ostream & out)
{
    // 重置
    lexer.rewind();
    // 分析栈
    std::vector<std::string> parseStack;

    // 打印分析栈
    auto printParseStack = [&parseStack, &out]() -> void
    {
        out << "Parse stack: ";
        for(const auto & symbol : parseStack)
        {
            out << symbol << " ";
        }
        out << std::endl;
    };

    // 打印 token
    auto printToken = [&out](Types::TokenPair token) -> void
    {
        out << Shared::typeStrings.at(token.first) << " ";
        if(token.first >= Types::TokenType::INIT 
            && token.first <= Types::TokenType::ENDOFFILE )
        {
            // out;
        }
        else if(token.first == Types::TokenType::KEYWORD )
        {
            out << "( " << std::any_cast<std::string>(token.second) << " )";
        }
        else if(token.first == Types::TokenType::IDENTIFIER )
        {
            out << "( " << Shared::idTable.at(std::any_cast<size_t>(token.second)) << " )";
        }
        else if(token.first >= Types::TokenType::INT_CONST
            && token.first <= Types::TokenType::STR_LITERAL )
        {
            out << "( " << Shared::constTable.at(std::any_cast<size_t>(token.second)) << " )";
        }
        else if(token.first >= Types::TokenType::OP_ADD
            && token.first <= Types::TokenType::OP_SCOPE )
        {
            out << "( " << std::any_cast<std::string>(token.second) << " )";
        }
        else if(token.first >= Types::TokenType::DELIM_DBQUOTE
            && token.first <= Types::TokenType::DELIM_QUESTION )
        {
            out << "( " << std::any_cast<std::string>(token.second) << " )";
        }
    };

    // 初始化入栈
    parseStack.push_back(Shared::endOfFileChar);
    parseStack.push_back(grammar.startSymbol);

    // 错误
    size_t errorCount = 0;

    // 指向第一个词
    auto token = lexer.getNextToken();
    while(true)
    {
        // 跳过非实义符号
        if(token.first == Types::TokenType::INIT)
        {
            token = lexer.getNextToken();
            continue;
        }

        // 打印栈
        printParseStack();
        out << "Now token: ";
        printToken(token);
        out << std::endl;

        // 如果词法错误，处理
        if(token.first == Types::TokenType::ERROR)
        {
            errorCount++;
            lexer.errorProcess(std::any_cast<Types::LexerError>(token.second));
            // 跳过本词
            token = lexer.getNextToken();
        }
        // 栈已经空了
        else if(parseStack.back() == Shared::endOfFileChar)
        {
            // 对上了
            if(token.first == Types::TokenType::ENDOFFILE)
            {
                out << "Successfully finished." << std::endl;
                break;
            }
            else
            {
                errorCount++;
                this -> errorProcess(Types::ParserError(lexer.getFilePos(), "unexpected end of file"));
                break;
            }
        }
        // 非终结符，需要根据预测分析表
        if(grammar.isNonTerminal(parseStack.back()))
        {
            std::string tokenTypeStr = Shared::typeStrings.at(token.first);
            // 替换掉关键词
            if(tokenTypeStr == "KEYWORD")
            {
                tokenTypeStr = std::any_cast<std::string>(token.second);
            }
            auto tableTermIter = LL1parseTable.find(std::make_pair(parseStack.back(), tokenTypeStr));
            // 找到了
            if(tableTermIter != LL1parseTable.end())
            {
                // 反向压入产生式
                const auto & rightPart = tableTermIter -> second.second;
                out << "Use rule: " << parseStack.back() << " -> ";
                for(const auto & symbol : rightPart)
                {
                    if(symbol == "")
                    {
                        out << "\"\"" << " ";
                    }
                    else
                    {
                        out << symbol << " ";
                    }
                }
                out << std::endl << std::endl;
                parseStack.pop_back();
                for(auto i = rightPart.rbegin(); i != rightPart.rend(); i++)
                {
                    // 跳过 ""
                    if(*i != "")
                    {
                        parseStack.push_back(*i);
                    }
                }
            }
            else
            {
                errorCount++;
                // 错误信息
                std::string errorMessage = "unexpected token: " + tokenTypeStr;
                errorMessage += ", expected: " + parseStack.back();
                this -> errorProcess(Types::ParserError(lexer.getFilePos(), errorMessage));
                break;
            }
        }
        // 终结符
        else if(grammar.isTerminal(parseStack.back()))
        {
            std::string tokenTypeStr = Shared::typeStrings.at(token.first);
            // 替换掉关键词
            if(tokenTypeStr == "KEYWORD")
            {
                tokenTypeStr = std::any_cast<std::string>(token.second);
            }
            // 对上了
            if(parseStack.back() == tokenTypeStr)
            {
                out << "Use rule: Pop stack" << std::endl << std::endl;
                parseStack.pop_back();
                token = lexer.getNextToken();
            }
            else
            {
                errorCount++;
                // 错误信息
                std::string errorMessage = "unexpected token: " + tokenTypeStr;
                errorMessage += ", expected: " + parseStack.back();
                this -> errorProcess(Types::ParserError(lexer.getFilePos(), errorMessage));
                break;
            }
        }
        else
        {
            errorCount++;
            // 错误信息
            std::string errorMessage = "unexpected token: " + Shared::typeStrings.at(token.first);
            errorMessage += ", expected: " + parseStack.back();
            this -> errorProcess(Types::ParserError(lexer.getFilePos(), errorMessage));
            break;
        }
    }
    return errorCount;
}

// 保存 LL(1) 预测分析表
bool Parser::saveLL1ParseTable(const std::string & fileName)
{
    std::cout << "Saving parsing table..." << std::endl;
    // 打开文件
    std::fstream outStream(fileName, std::ios::out);
    if(!outStream.is_open())
    {
        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Can't open output file: " 
            << fileName << "\033[0m" << std::endl;
        return false;
    }
    printLL1ParseTable(outStream);
    outStream.close();
    return true;
}

// 读取 LL(1) 预测分析表
bool Parser::readLL1ParseTable(const std::string & fileName)
{
    std::cout << "Reading parsing table..." << std::endl;
    // 打开文件
    std::fstream fileStream;
    fileStream.open(fileName);
    // 打不开文件
    if(!fileStream.is_open())
    {
        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Can't open grammar file: " 
            << fileName << "\033[0m" << std::endl;
        return false;
    }

    // 循环读取
    while(!fileStream.eof())
    {
        // 一行
        std::string lineString;
        // 读进来一行
        std::getline(fileStream, lineString);
        if(lineString.empty())
        {
            continue;
        }
        // 建立字符串流
        std::stringstream lineStream(lineString);
        // 当前符号
        std::string nowSymbol;

        // 非终结符
        Grammar::NonTerminalType NT;
        // 终结符
        Grammar::TerminalType T;
        // 产生式
        Grammar::ProductionType P;

        // 读取 M [ 部分
        lineStream >> nowSymbol;
        if(nowSymbol != "M")
            return false;
        
        lineStream >> nowSymbol;
        if(nowSymbol != "[")
            return false;

        // 读取非终结符
        lineStream >> NT;
        if(!grammar.isNonTerminal(NT))
            return false;

        // 读取 ,
        lineStream >> nowSymbol;
        if(nowSymbol != ",")
            return false;
        
        // 读取终结符
        lineStream >> T;
        if(T == "\"\"")
            T = "";
        if(!grammar.isTerminal(T))
            return false;
        
        // 读取 ] = 部分
        lineStream >> nowSymbol;
        if(nowSymbol != "]")
            return false;

        lineStream >> nowSymbol;
        if(nowSymbol != "=")
            return false;
        
        // 读取左端
        Grammar::NonTerminalType leftPart;
        lineStream >> leftPart;
        if(!grammar.isNonTerminal(leftPart))
            return false;
        // 读取箭头
        lineStream >> nowSymbol;
        if(nowSymbol != "->")
            return false;
        // 读取右端
        std::vector<Grammar::SymbolType> rightPart;
        while(lineStream >> nowSymbol)
        {
            // 空字符
            if(nowSymbol == "\"\"" || nowSymbol == "''")
            {
                nowSymbol = "";
            }
            rightPart.push_back(nowSymbol);
        }
        P = {leftPart, rightPart};
        LL1parseTable[std::make_pair(NT, T)] = P;
    }
    std::cout << "Done." << std::endl;
    return true;
}

// ------------------------ LR(1) 语法分析 ------------------------
// 计算 LR(1) 解析表
bool Parser::calcLRParseTable()
{
    // -------------------- FIRST 表和 FOLLOW 表的填写 --------------------
    // 两个表
    FirstTableType tmpFirstTable;
    FollowTableType tmpFollowTable;

    // 最大迭代次数
    size_t maxIteration = 1e6;

    // 根据当前 FIRST 表，得到一串符号的 FIRST
    auto firstOfSymbols = [this, &tmpFirstTable](const std::vector<Grammar::SymbolType> & symbols) -> std::set<Grammar::TerminalType>
    {
        if(symbols.empty())
        {
            return {""};
        }
        // 一开始，空集合
        std::set<Grammar::TerminalType> s = {};
        for(size_t i = 0; i < symbols.size(); i++)
        {
            std::set<Grammar::TerminalType> firstOfThisSymbol;
            // 如果是终结符
            if(this -> grammar.isTerminal(symbols[i]))
            {
                firstOfThisSymbol.insert(symbols[i]);
            }
            // 查表得到当前符号的 First
            else if(grammar.isNonTerminal(symbols[i]))
            {
                // 没有对应产生式
                if(tmpFirstTable.find(symbols[i]) == tmpFirstTable.end())
                {
                    std::cout << "Exception: In function firstOfSymbols." << std::endl;
                    std::cout << symbols[i] << ": No corresponding production." << std::endl;
                    return {};
                }
                firstOfThisSymbol = tmpFirstTable[symbols[i]];
            }
            else
            {
                std::cout << "Exception: In function firstOfSymbols." << std::endl;
                std::cout << symbols[i] << ": Neither a terminal nor a non-terminal." << std::endl;
                return {};
            }

            // 将除去 空字 的 First 加入
            s.insert(firstOfThisSymbol.begin(), firstOfThisSymbol.end());
            s.erase("");
            // 当前字的 First 不含空，则停止
            if(firstOfThisSymbol.find("") == firstOfThisSymbol.end())
            {
                break;
            }
            // 最后一个还含有空，则加入空
            if(i == symbols.size() - 1)
            {
                s.insert("");
            }
        }
        return s;
    };

    // 计算 FISRT 表
    auto calcFirstTable = [this, maxIteration, &tmpFirstTable, firstOfSymbols]() -> bool
    {
        size_t iter = 0;
        // 初始化
        for(const auto & symbol : grammar.nonTerminals)
        {
            // 各符号都是空集合
            tmpFirstTable[ symbol ] = {};
        }
        // 判断是否有更新
        bool modifiedFlag = true;
        while(modifiedFlag)
        {
            iter++;
            modifiedFlag = false;
            // 对每个产生式
            for(const auto & production : this -> grammar.productions)
            {
                // 左半部
                const auto & leftPart = production.first;
                // 对于右侧每一个产生式
                for(const auto & rightPart : production.second)
                {
                    // 原集合大小
                    size_t oldSize = tmpFirstTable[leftPart].size();
                    // 计算它们的 First
                    std::set<Grammar::TerminalType> firstOfRightPart = firstOfSymbols(rightPart);
                    // 合并集合
                    tmpFirstTable[leftPart].insert(firstOfRightPart.begin(), firstOfRightPart.end());
                    // 增加符号后的大小
                    size_t newSize = tmpFirstTable[leftPart].size();
                    if(newSize != oldSize)
                    {
                        modifiedFlag = true;
                    }
                }
            }
            if(iter > maxIteration)
            {
                return false;
            }
        }
        firstTable = tmpFirstTable;
        return true;
    };

    // 计算 FOLLOW 表
    auto calcFollowTable = [this, maxIteration, &tmpFirstTable, &tmpFollowTable, firstOfSymbols]() -> bool
    {
        size_t iter = 0;
        // 初始化
        for(const auto & symbol : grammar.nonTerminals)
        {
            // 各符号都是空集合
            tmpFollowTable[ symbol ] = {};
        }
        // Follow 表起始字符为结束字符
        tmpFollowTable[grammar.startSymbol] = {Shared::endOfFileChar};
        // 判断是否有更新
        bool modifiedFlag = true;
        while(modifiedFlag)
        {
            iter++;
            modifiedFlag = false;
            // 对每一个产生式
            for(const auto & production : grammar.productions)
            {
                // 计算左部、右部
                const auto & leftPart = production.first;
                for(const auto & rightPart : production.second)
                {
                    // 考虑每个产生式右边的非终结符
                    for(size_t i = 0; i < rightPart.size(); i++)
                    {
                        // 跳过终结符
                        if(grammar.isTerminal(rightPart[i]))
                        {
                            continue;
                        }
                        // 既不是终结符也不是非终结符
                        if(!grammar.isNonTerminal(rightPart[i]))
                        {
                            std::cout << "Exception: In function calcFollowTable." << std::endl;
                            return false;
                        }
                        size_t oldSize = tmpFollowTable[rightPart[i]].size();
                        // 获取右边的符号
                        std::vector<Grammar::SymbolType> rightSymbols(rightPart.begin() + i + 1, rightPart.end());
                        // 计算右边的符号串的 First
                        std::set<Grammar::TerminalType> firstOfRightSymbols = firstOfSymbols(rightSymbols);
                        // 并进去
                        tmpFollowTable[rightPart[i]].insert(firstOfRightSymbols.begin(), firstOfRightSymbols.end());
                        // 如果有空串，则去掉，并且把产生式左边的符号的 Follow 加进去
                        if(tmpFollowTable[rightPart[i]].find("") != tmpFollowTable[rightPart[i]].end())
                        {
                            tmpFollowTable[rightPart[i]].erase("");
                            tmpFollowTable[rightPart[i]].insert(tmpFollowTable[leftPart].begin(), tmpFollowTable[leftPart].end());
                        }
                        // 新的大小
                        size_t newSize = tmpFollowTable[rightPart[i]].size();
                        if(newSize != oldSize)
                        {
                            modifiedFlag = true;
                        }
                    }
                }
            }
            if(iter > maxIteration)
            {
                return false;
            }
        }
        followTable = tmpFollowTable;
        return true;
    };

    // 先计算一下整个文法的 FIRST 和 FOLLOW
    // 计算 First
    if(!calcFirstTable())
    {
        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Build First-Table failed. (max iteration exceeded)\033[0m " << std::endl;
        return false;
    }
    // 计算 Follow
    if(!calcFollowTable())
    {
        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Build Follow-Table failed. (max iteration exceeded)\033[0m " << std::endl;
        return false;
    }

    // -------------------- LR(1) 开始 --------------------
    // 打印一个项目
    auto printTerm = [this](std::ostream & out, const LRTerm & term) -> void
    {
        // 打印左端
        out << "[ " << term.first.first;
        out << " -> ";
        const auto & rightPart = this -> grammar.productions[term.first.first][term.first.second];
        size_t dotPos = term.second.first;
        for(size_t i = 0; i < dotPos; i++)
        {
            out << rightPart[i] << " ";
        }
        out << "· ";
        for(size_t i = dotPos; i < rightPart.size(); i++)
        {
            out << rightPart[i] << " ";
        }
        out << ", " << term.second.second << " ]" << std::endl;
    };
    
    // 项目（初始仅一个）
    LRTerm startTerm = {{grammar.startSymbol, 0}, {0, Shared::endOfFileChar}};
    std::vector<LRTerm> LRTerms = {startTerm};
    
    // 项目到编号
    std::map<LRTerm, size_t> termToIndex = {{startTerm, 0}};

    // 每个非终结符开头项目的序号
    std::map<Grammar::NonTerminalType, std::set<size_t>> NTStartTerms = {{grammar.startSymbol, {0}}};

    // 动态地计算 LR(1) 项目集的闭包
    //（即如果这个项目还没有遇到过，则需要扩展出来，边产生边计算）
    // 计算规则:
    //      1. I 的任何项目都属于 CLOSURE(I)
    //      2. 若项目 [ A -> α⋅Bβ, a ] 属于 CLOSURE(I), 
    //          B -> r 是文法中的一条规则, b 属于 FIRST(βa), 
    //          则 [ B -> ⋅r, b ] 也属于CLOSURE(I)
    auto closure = [this, firstOfSymbols, &LRTerms, &termToIndex, &NTStartTerms](const std::set<size_t> & _i) -> std::set<size_t>
    {
        // 首先本身都属于闭包
        std::set<size_t> indices = _i;
        bool modified = true;
        // 直到当前项目集不再扩大
        while(modified)
        {
            modified = false;
            // 扫描每个项目，看看后一个字符是不是非终结符，且能够扩展
            for(const auto & i : indices)
            {
                // 产生式左部
                const auto & leftPart = LRTerms[i].first.first;
                // 产生式右部
                const auto & rightPart = grammar.productions[leftPart][LRTerms[i].first.second];
                // 点在最后
                if(rightPart.size() <= LRTerms[i].second.first)
                {
                    continue;
                }
                // 下一个符号是非终结符
                const std::string & nextSymbol = rightPart[LRTerms[i].second.first];
                if(grammar.isNonTerminal(nextSymbol))
                {
                    // 旧 size，判断大小是否改变
                    size_t oldSize = indices.size();
                    // 求出下一个非终结符 B 的后面部分 β
                    std::vector<Grammar::SymbolType> betaA = \
                        std::vector<Grammar::SymbolType>
                        (
                            rightPart.begin() + LRTerms[i].second.first + 1,
                            rightPart.end()
                        );
                    // β 后面并上 a
                    betaA.push_back(LRTerms[i].second.second);
                    // 求出后面部分的首符号集 FIRST(βa)
                    auto firstOfBetaA = firstOfSymbols(betaA);

                    // 先找到所有 B -> r 产生式，然后
                    // 对于 b 属于 FIRST(βa)，将所有 [ B -> ⋅r, b ] 加入CLOSURE
                    // 注意更新
                    // 对每一条 B -> r 产生式
                    for(size_t i = 0; i < grammar.productions.at(nextSymbol).size(); i++)
                    {
                        for(const auto b : firstOfBetaA)
                        {
                            // 新生成的项目
                            LRTerm newTerm = {{nextSymbol, i}, {0, b}};
                            // 没有过，加入
                            if(termToIndex.find(newTerm) == termToIndex.end())
                            {
                                LRTerms.push_back(newTerm);
                                termToIndex[newTerm] = LRTerms.size() - 1;
                                NTStartTerms[nextSymbol].insert(LRTerms.size() - 1); 
                            }
                            indices.insert(termToIndex.at(newTerm));
                        }
                    }
                    
                    if(indices.size() != oldSize)
                    {
                        modified = true;
                    }
                }
            }

        }
        return indices;
    };

    // 计算 LR(0) 的自动机的同时 BFS 产生分析表
    auto calcFAAndParseTable = [this, &LRTerms, &termToIndex, &NTStartTerms, printTerm, closure]() -> bool
    {
        LRParseTableType tmpLRparseTable;

        // LR 分析 FA 中的节点
        // 即为若干产生式的集合
        struct LRFANode
        {
            std::set<size_t> termIndices;
        };

        // 各项目集族（FA 的节点）的比较函数
        class FANodeCmp
        {
            public:
                bool operator()(const LRFANode & nodeA, const LRFANode & nodeB) const
                {
                    return nodeA.termIndices < nodeB.termIndices;
                }
        };

        // 打印一个节点
        auto printNode = [this, &LRTerms, printTerm](std::ostream & out, const LRFANode & node) -> void
        {
            for(const auto & i : node.termIndices)
            {
                printTerm(out, LRTerms[i]);
            }
        };

        // 打印动作表项
        auto printAction = [this](std::ostream & out, const ActionTableTerm & term) -> void
        {
            if(term.first == ACTION::SHIFT)
            {
                out << "SHIFT " << std::any_cast<size_t>(term.second);
            }
            else if(term.first == ACTION::REDUCE)
            {
                out << "REDUCE ";
                const auto & production = std::any_cast<std::pair<Grammar::NonTerminalType, size_t>>(term.second);
                const auto & leftPart = production.first;
                const auto & rightPart = grammar.productions.at(leftPart)[production.second];
                out << leftPart << " -> ";
                for(const auto & symbol : rightPart)
                {
                    out << (symbol == "" ? "\"\"" : symbol) << " ";
                }
            }
            else if(term.first == ACTION::GOTO)
            {
                out << "GOTO " << std::any_cast<size_t>(term.second);
            }
            else if(term.first == ACTION::ACCEPT)
            {
                out << "ACCEPT";
            }
            out << std::endl;
        };

        // 动作表项比较
        auto actionEq = [this](const std::pair<ACTION, std::any> & actionA, const std::pair<ACTION, std::any> & actionB) -> bool
        {
            if(actionA.first != actionB.first)
                return false;
            if(actionA.first == ACTION::ACCEPT)
                return true;
            else if(actionA.first == ACTION::GOTO || actionA.first == ACTION::SHIFT)
                return std::any_cast<size_t>(actionA.second) == std::any_cast<size_t>(actionB.second);
            else if(actionA.first == ACTION::REDUCE)
                return std::any_cast<std::pair<Grammar::NonTerminalType, size_t>>(actionA.second) ==
                    std::any_cast<std::pair<Grammar::NonTerminalType, size_t>>(actionB.second);
            else return false;
        };

        // 冲突处理办法
        enum class HANDLINGFLAG 
        {
            KEEP, 
            OVERWRITE, 
            ABORT
        };
        // 冲突处理
        auto conflictHandling = [this, printAction]( 
            const ActionTableTerm & inTable, const ActionTableTerm & generated) -> HANDLINGFLAG
            {
                std::cout << "In table: ";
                printAction(std::cout, inTable);
                std::cout << "Generated: ";
                printAction(std::cout, generated);
                std::cout << "[K]eep, [O]verwrite, [A]bort: ";

                std::string cmd;
                std::cin >> cmd;

                if(cmd == "k" || cmd == "K")
                {
                    return HANDLINGFLAG::KEEP;
                }
                else if(cmd == "o" || cmd == "O")
                {
                    return HANDLINGFLAG::OVERWRITE;
                }
                else return HANDLINGFLAG::ABORT;
            };

        // 节点到序号的映射
        std::map<LRFANode, size_t, FANodeCmp> nodeToIndex;
        // 节点队列
        std::queue<LRFANode> q;

        // 起始节点
        LRFANode startNode;
        // 扩展成闭包
        startNode.termIndices = closure(NTStartTerms.at(grammar.startSymbol));
        // 起始节点入队列
        q.push(startNode);

        // 打印节点 0
        std::cout << "Nodes:" << std::endl;
        std::cout << "Node 0:" << std::endl;
        printNode(std::cout, startNode);
        std::cout << std::endl;

        // 当队列不空
        while(!q.empty())
        {
            // 节点出队
            LRFANode node = q.front();
            q.pop();

            // 节点加入节点-序号表
            if(nodeToIndex.find(node) == nodeToIndex.end())
            {
                nodeToIndex[node] = nodeToIndex.size();
            }
            // 获取当前节点的序号
            size_t nowNodeIndex = nodeToIndex.at(node);

            // 记录出边上标签（移进项目中下一个符号）到项目号集合的映射
            std::map<Grammar::SymbolType, std::set<size_t>> outLabelToIndices;
            // 考虑每一项
            for(const size_t & termIndex : node.termIndices)
            {
                // 本项目的产生式的左半部分
                const auto & leftPart = LRTerms[termIndex].first.first;
                // 产生式的右半部分
                const auto & rightPart = grammar.productions.at(leftPart)[LRTerms[termIndex].first.second];
                // 点的位置
                const size_t & dotPos = LRTerms[termIndex].second.first;
                // 展望符号
                const auto & lookAheadSymbol = LRTerms[termIndex].second.second;
                // 如果本项目是归约项目，先判断是否为接受项目，然后填表
                if(dotPos >= rightPart.size())
                {
                    // 接受
                    if(grammar.isStartSymbol(leftPart))
                    {
                        tmpLRparseTable[std::make_pair(nowNodeIndex, lookAheadSymbol)] = \
                            std::make_pair
                                (
                                    ACTION::ACCEPT,
                                    // 产生式
                                    std::any(nullptr)
                                );
                    }
                    else if
                    (
                        (tmpLRparseTable.find(std::make_pair(nowNodeIndex, lookAheadSymbol)) == tmpLRparseTable.end())
                        ||
                        (
                            tmpLRparseTable.find(std::make_pair(nowNodeIndex, lookAheadSymbol)) != tmpLRparseTable.end()
                            &&
                            actionEq
                            (
                                tmpLRparseTable.at(std::make_pair(nowNodeIndex, lookAheadSymbol)),
                                std::make_pair
                                (
                                    ACTION::REDUCE,
                                    // 产生式
                                    std::any(LRTerms[termIndex].first)
                                )
                            ) 
                        )
                    )
                    {
                        tmpLRparseTable[std::make_pair(nowNodeIndex, lookAheadSymbol)] = \
                            std::make_pair
                                (
                                    ACTION::REDUCE,
                                    // 产生式
                                    std::any(LRTerms[termIndex].first)
                                );
                    }
                    else
                    {
                        // 手动冲突处理
                        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;35mwarning:\033[0m conflict at (" << nowNodeIndex << ", " << lookAheadSymbol << ")" << std::endl;
                        HANDLINGFLAG flag = conflictHandling
                        (
                            tmpLRparseTable.at(std::make_pair(nowNodeIndex, lookAheadSymbol)), 
                            std::make_pair
                                (
                                    ACTION::REDUCE,
                                    // 产生式
                                    std::any(LRTerms[termIndex].first)
                                )
                        );
                        if(flag == HANDLINGFLAG::OVERWRITE)
                        {
                            tmpLRparseTable.at(std::make_pair(nowNodeIndex, lookAheadSymbol)) = 
                                std::make_pair
                                    (
                                        ACTION::REDUCE,
                                        // 产生式
                                        std::any(LRTerms[termIndex].first)
                                    );
                        }
                        else if(flag == HANDLINGFLAG::ABORT)
                        {
                            std::cout << "Abort." << std::endl;
                            return false;
                        }
                    }
                }
                // 本项目是移进项目，则记录下一个符号，填到出边表里面
                else
                {
                    const auto & nextSymbol = rightPart[dotPos];
                    // 计算下一项
                    auto nextTerm = LRTerms[termIndex];
                    // 点往后移动
                    nextTerm.second.first++;
                    // 新项目要更新
                    if(termToIndex.find(nextTerm) == termToIndex.end())
                    {
                        LRTerms.push_back(nextTerm);
                        termToIndex[nextTerm] = LRTerms.size() - 1;
                        NTStartTerms[nextSymbol].insert(LRTerms.size() - 1); 
                    }
                    // 找到编号
                    size_t nextTermIndex = termToIndex.at(nextTerm);
                    if(outLabelToIndices.find(nextSymbol) == outLabelToIndices.end())
                    {
                        outLabelToIndices[nextSymbol] = std::set<size_t>();
                    }
                    // 加入编号
                    outLabelToIndices[nextSymbol].insert(nextTermIndex);
                }
            }
            
            // 记录新节点编号、入队新节点，根据出边表填分析表中移进、转移项目
            for(const auto & symbolIndices : outLabelToIndices)
            {
                // 新节点
                LRFANode newNode;
                newNode.termIndices = closure(symbolIndices.second);
                
                // 如果未访问过，节点加入节点-序号表
                if(nodeToIndex.find(newNode) == nodeToIndex.end())
                {
                    nodeToIndex[newNode] = nodeToIndex.size();
                    // 入队
                    q.push(newNode);
                    // 打印节点
                    std::cout << "Node " << nodeToIndex.at(newNode) << " :" << std::endl;
                    printNode(std::cout, newNode);
                    std::cout << std::endl;
                }
                // 获取当前节点的序号
                size_t newNodeIndex = nodeToIndex.at(newNode);

                // 填表
                // 如果是终结符 - 移进
                if(grammar.isTerminal(symbolIndices.first))
                {
                    if
                    (
                        (tmpLRparseTable.find(std::make_pair(nowNodeIndex, symbolIndices.first)) == tmpLRparseTable.end())
                        ||
                        (
                            tmpLRparseTable.find(std::make_pair(nowNodeIndex, symbolIndices.first)) != tmpLRparseTable.end()
                            &&
                            actionEq
                            (
                                tmpLRparseTable.at(std::make_pair(nowNodeIndex, symbolIndices.first)),
                                std::make_pair
                                (
                                    ACTION::SHIFT,
                                    // 下一节点
                                    std::any(newNodeIndex)
                                )
                            ) 
                        )
                    )
                    {
                        tmpLRparseTable[std::make_pair(nowNodeIndex, symbolIndices.first)] = \
                            std::make_pair
                                (
                                    ACTION::SHIFT,
                                    // 下一节点
                                    std::any(newNodeIndex)
                                );
                    }
                    else
                    {
                        // 手动冲突处理
                        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;35mwarning:\033[0m conflict at (" << nowNodeIndex << ", " << symbolIndices.first << ")" << std::endl;
                        HANDLINGFLAG flag = conflictHandling
                        (
                            tmpLRparseTable.at(std::make_pair(nowNodeIndex, symbolIndices.first)), 
                            std::make_pair
                                (
                                    ACTION::SHIFT,
                                    // 下一节点
                                    std::any(newNodeIndex)
                                )
                        );
                        if(flag == HANDLINGFLAG::OVERWRITE)
                        {
                            tmpLRparseTable.at(std::make_pair(nowNodeIndex, symbolIndices.first)) = 
                                std::make_pair
                                    (
                                        ACTION::SHIFT,
                                        // 下一节点
                                        std::any(newNodeIndex)
                                    );
                        }
                        else if(flag == HANDLINGFLAG::ABORT)
                        {
                            std::cout << "Abort." << std::endl;
                            return false;
                        }
                    }
                }
                // 是非终结符 - 转移
                else
                // (grammar.isNonTerminal(symbolIndices.first))
                {
                    if
                    (
                        (tmpLRparseTable.find(std::make_pair(nowNodeIndex, symbolIndices.first)) == tmpLRparseTable.end())
                        ||
                        (
                            tmpLRparseTable.find(std::make_pair(nowNodeIndex, symbolIndices.first)) != tmpLRparseTable.end()
                            &&
                            actionEq
                            (
                                tmpLRparseTable.at(std::make_pair(nowNodeIndex, symbolIndices.first)),
                                std::make_pair
                                (
                                    ACTION::GOTO,
                                    // 下一节点
                                    std::any(newNodeIndex)
                                )
                            ) 
                        )
                    )
                    {
                        tmpLRparseTable[std::make_pair(nowNodeIndex, symbolIndices.first)] = \
                            std::make_pair
                                (
                                    ACTION::GOTO,
                                    // 下一节点
                                    std::any(newNodeIndex)
                                );
                    }
                    else
                    {
                        // 手动冲突处理
                        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;35mwarning:\033[0m conflict at (" << nowNodeIndex << ", " << symbolIndices.first << ")" << std::endl;
                        HANDLINGFLAG flag = conflictHandling
                        (
                            tmpLRparseTable.at(std::make_pair(nowNodeIndex, symbolIndices.first)), 
                            std::make_pair
                                (
                                    ACTION::GOTO,
                                    // 下一节点
                                    std::any(newNodeIndex)
                                )
                        );
                        if(flag == HANDLINGFLAG::OVERWRITE)
                        {
                            tmpLRparseTable.at(std::make_pair(nowNodeIndex, symbolIndices.first)) = 
                                std::make_pair
                                    (
                                        ACTION::GOTO,
                                        // 下一节点
                                        std::any(newNodeIndex)
                                    );
                        }
                        else if(flag == HANDLINGFLAG::ABORT)
                        {
                            std::cout << "Abort." << std::endl;
                            return false;
                        }
                    }
                }
            }
        }
        
        LRparseTable = tmpLRparseTable;
        return true;
    };
    
    return calcFAAndParseTable();
}

// 打印 LR 表格
void Parser::printLRInternalTables(std::ostream & out)
{
    // out << "This is an LR(0) grammar." << std::endl;
    for(const auto & tableTerm : LRparseTable)
    {
        out << "M [ " << tableTerm.first.first 
            << " , " << (tableTerm.first.second == "" ? "\"\"" : tableTerm.first.second) << " ] = ";

        if(tableTerm.second.first == ACTION::SHIFT)
        {
            out << "SHIFT " << std::any_cast<size_t>(tableTerm.second.second);
        }
        else if(tableTerm.second.first == ACTION::REDUCE)
        {
            out << "REDUCE ";
            const auto & production = std::any_cast<std::pair<Grammar::NonTerminalType, size_t>>(tableTerm.second.second);
            const auto & leftPart = production.first;
            const auto & rightPart = grammar.productions.at(leftPart)[production.second];
            out << leftPart << " -> ";
            for(const auto & symbol : rightPart)
            {
                out << (symbol == "" ? "\"\"" : symbol) << " ";
            }
        }
        else if(tableTerm.second.first == ACTION::GOTO)
        {
            out << "GOTO " << std::any_cast<size_t>(tableTerm.second.second);
        }
        else if(tableTerm.second.first == ACTION::ACCEPT)
        {
            out << "ACCEPT";
        }
        out << std::endl;
    }
}

// LR 分析
size_t Parser::LRParse(std::ostream & out)
{
    // 重置
    lexer.rewind();
    // 分析栈
    // 状态栈
    std::vector<size_t> stateStack;
    // 符号栈
    std::vector<std::string> symbolStack;

    // 打印栈
    auto printStack = [&stateStack, &symbolStack , &out]() -> void
    {
        out << "State stack: ";
        for(const auto & symbol : stateStack)
        {
            out << symbol << " ";
        }
        out << std::endl;

        out << "Symbol stack: ";
        for(const auto & symbol : symbolStack)
        {
            out << symbol << " ";
        }
        out << std::endl;
    };

    // 打印 token
    auto printToken = [&out](Types::TokenPair token) -> void
    {
        out << Shared::typeStrings.at(token.first) << " ";
        if(token.first >= Types::TokenType::INIT 
            && token.first <= Types::TokenType::ENDOFFILE )
        {
            // out;
        }
        else if(token.first == Types::TokenType::KEYWORD )
        {
            out << "( " << std::any_cast<std::string>(token.second) << " )";
        }
        else if(token.first == Types::TokenType::IDENTIFIER )
        {
            out << "( " << Shared::idTable.at(std::any_cast<size_t>(token.second)) << " )";
        }
        else if(token.first >= Types::TokenType::INT_CONST
            && token.first <= Types::TokenType::STR_LITERAL )
        {
            out << "( " << Shared::constTable.at(std::any_cast<size_t>(token.second)) << " )";
        }
        else if(token.first >= Types::TokenType::OP_ADD
            && token.first <= Types::TokenType::OP_SCOPE )
        {
            out << "( " << std::any_cast<std::string>(token.second) << " )";
        }
        else if(token.first >= Types::TokenType::DELIM_DBQUOTE
            && token.first <= Types::TokenType::DELIM_QUESTION )
        {
            out << "( " << std::any_cast<std::string>(token.second) << " )";
        }
    };

    // 初始化入栈
    stateStack.push_back(0);
    symbolStack.push_back(Shared::endOfFileChar);

    // 错误
    size_t errorCount = 0;

    // 指向第一个词
    auto token = lexer.getNextToken();
    while(true)
    {
        // 跳过非实义符号
        if(token.first == Types::TokenType::INIT)
        {
            token = lexer.getNextToken();
            continue;
        }

        // 打印栈
        printStack();
        out << "Now token: ";
        printToken(token);
        out << std::endl;

        // 如果词法错误，处理
        if(token.first == Types::TokenType::ERROR)
        {
            errorCount++;
            lexer.errorProcess(std::any_cast<Types::LexerError>(token.second));
            // 跳过本词
            token = lexer.getNextToken();
        }
        else
        {
            std::string tokenTypeStr = Shared::typeStrings.at(token.first);
            // 替换掉关键词
            if(tokenTypeStr == "KEYWORD")
            {
                tokenTypeStr = std::any_cast<std::string>(token.second);
            }
            // 查表
            if(
                LRparseTable.find(
                    std::make_pair(stateStack.back(), tokenTypeStr)
                )
                == LRparseTable.end() 
            )
            {
                // 语法错误.
                errorCount++;
                // 错误信息
                std::string errorMessage = "unexpected token: " + tokenTypeStr;
                this -> errorProcess(Types::ParserError(lexer.getFilePos(), errorMessage));
                break;
            }
            else
            {
                auto action = LRparseTable[std::make_pair(stateStack.back(), tokenTypeStr)];
                // 接受
                if(action.first == ACTION::ACCEPT)
                {
                    out << "Use rule: Accept." << std::endl;
                    break;
                }
                // 移进
                else if(action.first == ACTION::SHIFT)
                {
                    out << "Use rule: Shift " << std::any_cast<size_t>(action.second) << std::endl << std::endl;
                    // 状态进栈
                    stateStack.push_back(std::any_cast<size_t>(action.second));
                    // 符号进栈
                    symbolStack.push_back(tokenTypeStr);
                    token = lexer.getNextToken();
                }
                // 归约
                else if(action.first == ACTION::REDUCE)
                {
                    const auto & production = std::any_cast<std::pair<Grammar::NonTerminalType, size_t>>(action.second);
                    const auto & leftPart = production.first;
                    const auto & rightPart = grammar.productions[leftPart][production.second];
                    out << "Use rule: Reduce " << leftPart << " -> ";
                    for(size_t i = 0; i < rightPart.size(); i++)
                    {
                        out << rightPart[i] << " ";
                    }
                    out << std::endl;

                    // 右部有几个符号
                    size_t lenRightPart = rightPart.size();
                    // 同时弹栈
                    for(size_t i = 0; i < lenRightPart; i++)
                    {
                        stateStack.pop_back();
                        symbolStack.pop_back();
                    }
                    // 接着要转移
                    if(
                        (LRparseTable.find(
                            std::make_pair(stateStack.back(), leftPart)
                        )
                        == LRparseTable.end()) ||
                        (LRparseTable.find(
                            std::make_pair(stateStack.back(), leftPart)
                        )
                        != LRparseTable.end() && 
                            LRparseTable[std::make_pair(stateStack.back(), leftPart)].first != ACTION::GOTO)
                    )
                    {
                        // 语法错误.
                        errorCount++;
                        // 错误信息
                        std::string errorMessage = "GOTO error: " + leftPart;
                        this -> errorProcess(Types::ParserError(lexer.getFilePos(), errorMessage));
                        break;
                    }
                    else
                    {
                        action = LRparseTable[std::make_pair(stateStack.back(), leftPart)];
                        out << "Use rule: Goto " << std::any_cast<size_t>(action.second) << std::endl << std::endl;
                        // 状态进栈
                        stateStack.push_back(std::any_cast<size_t>(action.second));
                        // 符号进栈
                        symbolStack.push_back(leftPart);
                    }
                }
                else
                {
                    // 语法错误.
                    errorCount++;
                    // 错误信息
                    std::string errorMessage = "Invalid table term! ";
                    this -> errorProcess(Types::ParserError(lexer.getFilePos(), errorMessage));
                    break;
                }
            }
        }
    }
    return errorCount;
}

// 保存 LR(1) 预测分析表
bool Parser::saveLRParseTable(const std::string & fileName)
{
    std::cout << "Saving parsing table..." << std::endl;
    // 打开文件
    std::fstream outStream(fileName, std::ios::out);
    if(!outStream.is_open())
    {
        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Can't open output file: " 
            << fileName << "\033[0m" << std::endl;
        return false;
    }
    printLRInternalTables(outStream);
    outStream.close();
    std::cout << "Done." << std::endl;
    return true;
}

// 读取 LR(1) 预测分析表
bool Parser::readLRParseTable(const std::string & fileName)
{
    std::cout << "Reading parsing table..." << std::endl;
    // 打开文件
    std::fstream fileStream;
    fileStream.open(fileName);
    // 打不开文件
    if(!fileStream.is_open())
    {
        std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Can't open grammar file: " 
            << fileName << "\033[0m" << std::endl;
        return false;
    }

    // 循环读取
    while(!fileStream.eof())
    {
        // 一行
        std::string lineString;
        // 读进来一行
        std::getline(fileStream, lineString);
        if(lineString.empty())
        {
            continue;
        }
        // 建立字符串流
        std::stringstream lineStream(lineString);
        // 当前符号
        std::string nowSymbol;

        // 状态
        size_t state = 0;
        // 符号
        Grammar::SymbolType symbol;
        // 动作
        std::pair<ACTION, std::any> action;

        // 读取 M [ 部分
        lineStream >> nowSymbol;
        if(nowSymbol != "M")
            return false;
        
        lineStream >> nowSymbol;
        if(nowSymbol != "[")
            return false;

        // 读取一个状态
        lineStream >> state;

        // 读取 ,
        lineStream >> nowSymbol;
        if(nowSymbol != ",")
            return false;
        
        // 读取符号
        lineStream >> symbol;
        if(symbol == "\"\"")
            symbol = "";
        if(!grammar.isNonTerminal(symbol) && !grammar.isTerminal(symbol))
            return false;
        
        // 读取 ] = 部分
        lineStream >> nowSymbol;
        if(nowSymbol != "]")
            return false;

        lineStream >> nowSymbol;
        if(nowSymbol != "=")
            return false;
        
        // 读取动作
        lineStream >> nowSymbol;

        if(nowSymbol == "ACCEPT")
        {
            action.first = ACTION::ACCEPT;
            action.second = std::any(nullptr);
            LRparseTable[std::make_pair(state, symbol)] = action;
        }
        else if(nowSymbol == "SHIFT")
        {
            // 读取下一状态
            size_t shiftState = 0;
            lineStream >> shiftState;
            action.first = ACTION::SHIFT;
            action.second = std::any(shiftState);
            LRparseTable[std::make_pair(state, symbol)] = action;
        }
        else if(nowSymbol == "GOTO")
        {
            // 读取下一状态
            size_t shiftState = 0;
            lineStream >> shiftState;
            action.first = ACTION::GOTO;
            action.second = std::any(shiftState);
            LRparseTable[std::make_pair(state, symbol)] = action;
        }
        else if(nowSymbol == "REDUCE")
        {
            // 读取左端
            Grammar::NonTerminalType leftPart;
            lineStream >> leftPart;
            if(!grammar.isNonTerminal(leftPart))
                return false;
            // 读取箭头
            lineStream >> nowSymbol;
            if(nowSymbol != "->")
                return false;
            // 读取右端
            std::vector<Grammar::SymbolType> rightPart;
            while(lineStream >> nowSymbol)
            {
                // 空字符
                if(nowSymbol == "\"\"" || nowSymbol == "''")
                {
                    nowSymbol = "";
                }
                rightPart.push_back(nowSymbol);
            }
            // 是第几个
            size_t index = 0;
            size_t total = grammar.productions.at(leftPart).size();
            for(index = 0; index < total; index++)
            {
                if(grammar.productions.at(leftPart)[index] == rightPart)
                {
                    break;
                }
            }
            if(index == total)
                return false;
            action.first = ACTION::REDUCE;
            action.second = std::any(std::make_pair(leftPart, index));
            LRparseTable[std::make_pair(state, symbol)] = action;
        }
    }
    std::cout << "Done." << std::endl;
    return true;
}

// ------------------------ 其它函数 ------------------------
// 错误处理
void Parser::errorProcess(const Types::ParserError & error)
{
    // 行列
    size_t row = error.first.first, col = error.first.second - 1;

    std::cout << "\033[1m" << lexer.getSrcName() << ":" 
        << row << ":" 
        << col << ": (Parser) \033[31merror: \033[0m\033[1m" 
        << error.second << "\033[0m" << std::endl;

    std::cout << "    " << lexer.getInBuf() << std::endl;
    std::cout << "    ";
    for(size_t i = 1; i < col; i++)
    {
        std::cout << (lexer.getInBuf()[i - 1] == '\t' ? '\t' : ' ');
    }
    std::cout << "\033[1;2m^\033[0m" << std::endl;
}

// 打印 FIRST 表
void Parser::printFirstTable(std::ostream & out)
{
    out << "FIRST Table:" << std::endl;
    for(const auto & i : firstTable)
    {
        out << "FIRST(" << i.first << "): ";
        for(const auto & j : i.second)
        {
            if(j == "")
            {
                out << "\"\"" << " ";
            }
            else
            {
                out << j << " ";
            }
        }
        out << std::endl;
    }
    out << std::endl;
};

// 打印 FOLLOW 表
void Parser::printFollowTable(std::ostream & out)
{
    out << "FOLLOW Table:" << std::endl;
    for(const auto & i : followTable)
    {
        out << "FOLLOW(" << i.first << "): ";
        for(const auto & j : i.second)
        {
            out << j << " ";
        }
        out << std::endl;
    }
    out << std::endl;
};

// 打印预测分析表
void Parser::printLL1ParseTable(std::ostream & out)
{
    // out << "LL(1) Parse Table:" << std::endl;
    for(const auto & tableTerm : this -> LL1parseTable)
    {
        out << "M [ " << tableTerm.first.first << " , " << tableTerm.first.second << " ] = ";
        out << tableTerm.second.first << " -> ";
        for(const auto & rightSymbol : tableTerm.second.second)
        {
            if(rightSymbol == "")
            {
                out << "\"\"" << " ";
            }
            else
            {
                out << rightSymbol << " "; 
            }
        }
        out << std::endl;
    }
    out << std::endl;
};

#define INDEPENDENT_PARSER
#ifdef INDEPENDENT_PARSER
int main(int argc, char * argv[])
{
    auto printUsage = []() -> void
    {
        std::cout << "Usage:\n  ./parser <filename> [options]" << std::endl;
        std::cout << "Options:\n  -h, --help\t\t\t Print help." << std::endl;
        std::cout << "  -LL, --LL(1)\t\t\t Use LL(1) parsing. (Default: Use LR(1) parsing.)" << std::endl;
        std::cout << "  -g, --grammar\t\t\t Set input grammar file name. (Default: Use internal grammar.)" << std::endl;
        std::cout << "  -ti, --table-in\t\t Use pre-calculated parsing table." << std::endl;
        std::cout << "  -to, --table-out\t\t Set output parsing table file name. (Default: 'LL(1).tbl' if LL(1) parsing, 'LR(1).tbl' if LR(1) parsing.)" << std::endl;
        std::cout << "  -o, --output\t\t\t Set output file name. (Default: 'out.txt'.)" << std::endl;
    };

    Parser parser;
    // 源文件名，语法文件名，输入/输出分析表文件名，输出文件名
    std::string srcFileName, grammarFileName, inputTableFileName, outputTableFileName, outputFileName = "out.txt";
    // 输出文件流
    std::fstream outStream;
    
    enum class FlagIndex
    {
        // 设置语法文件名
        SET_GRAMMARFILE,
        // 设置输出文件名
        SET_OUTPUTFILE,
        // 设置 LL(1) 分析
        SET_LLPARSE,
        // 设置使用的输入/输出分析表
        SET_INPUTTABLEFILE,
        SET_OUTPUTTABLEFILE
    };

    // 设置相关 Flags
    std::bitset<8> setFlags = 0;

    if(argc <= 1)
    {
        std::cout << "\033[1m(Parser)\033[0m \033[1;31merror:\033[0m Wrong usage!" << std::endl;
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
        else if(cmd == "-LL" || cmd == "--LL(1)")
        {
            setFlags.set(size_t(FlagIndex::SET_LLPARSE));
        }
        else if(cmd == "-g" || cmd == "--grammar")
        {
            setFlags.set(size_t(FlagIndex::SET_GRAMMARFILE));
        }
        else if(cmd == "-ti" || cmd == "--table-in")
        {
            setFlags.set(size_t(FlagIndex::SET_INPUTTABLEFILE));
        }
        else if(cmd == "-to" || cmd == "--table-out")
        {
            setFlags.set(size_t(FlagIndex::SET_OUTPUTTABLEFILE));
        }
        else if(cmd == "-o" || cmd == "--output")
        {
            setFlags.set(size_t(FlagIndex::SET_OUTPUTFILE));
        }
        else if
            (
                (cmd.size() >= 1 && cmd[0] == '-') ||
                (cmd.size() >= 2 && cmd[0] == '-' && cmd[1] == '-')
            )
        {
            std::cout << "\033[1m(Parser)\033[0m \033[1;31merror:\033[0m Wrong parameter: " << cmd << std::endl;
            printUsage();
            exit(1);
        }
        else
        {
            if(i == 1)
            {
                srcFileName = cmd;
            }
            // 设置 token 文件
            if(setFlags.test(size_t(FlagIndex::SET_GRAMMARFILE)))
            {
                grammarFileName = cmd;
                setFlags.set(size_t(FlagIndex::SET_GRAMMARFILE), false);
            }
            // 设置输出文件
            if(setFlags.test(size_t(FlagIndex::SET_OUTPUTFILE)))
            {
                outputFileName = cmd;
                setFlags.set(size_t(FlagIndex::SET_OUTPUTFILE), false);
            }
            // 设置表格
            if(setFlags.test(size_t(FlagIndex::SET_INPUTTABLEFILE)))
            {
                inputTableFileName = cmd;
                setFlags.set(size_t(FlagIndex::SET_INPUTTABLEFILE), false);
            }
            if(setFlags.test(size_t(FlagIndex::SET_OUTPUTTABLEFILE)))
            {
                outputTableFileName = cmd;
                setFlags.set(size_t(FlagIndex::SET_OUTPUTTABLEFILE), false);
            }
        }
    }

    // 打不开文件
    if(!parser.openFile(srcFileName))
    {
        std::cout << "\033[1m(Parser)\033[0m \033[1;31merror:\033[0m\033[1m Can't open source file: " 
            << srcFileName << "\033[0m" << std::endl;
        exit(1);
    }
    outStream.open(outputFileName, std::ios::out);
    if(!outStream.is_open())
    {
        std::cout << "\033[1m(Parser)\033[0m \033[1;31merror:\033[0m\033[1m Can't open output file: " 
            << outputFileName << "\033[0m" << std::endl;
        exit(1);
    }

    // 读取语法
    if(grammarFileName != "")
    {
        if(!parser.readGrammar(grammarFileName))
        {
            std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Invalid grammar. \033[0m" << std::endl; 
            exit(1);
        }
    }
    // 计算预测分析表
    size_t errorCount = 0;

    // 使用 LL(1) 分析
    if(setFlags.test(size_t(FlagIndex::SET_LLPARSE)))
    {
        // 读取文件
        if(!inputTableFileName.empty())
        {
            if(!parser.readLL1ParseTable(inputTableFileName))
            {
                std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Invalid LL(1) grammar. \033[0m" << std::endl;
                exit(1);
            }
        }
        else if(!parser.calcLL1ParseTable())
        {
            std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Invalid LL(1) grammar. \033[0m" << std::endl;
            exit(1);
        }

        // 打印一下
        //parser.printLL1InternalTables(std::cout);
        //parser.printLL1InternalTables(outStream);
        // 保存分析表
        if(inputTableFileName.empty() || !outputTableFileName.empty())
        {
            parser.saveLL1ParseTable(outputTableFileName.empty() ? "LL.tbl" : outputTableFileName);
        }
        std::cout << std::endl;
        outStream << std::endl;
        std::cout << "This is an LL(1) grammar." << std::endl;
        outStream << "This is an LL(1) grammar." << std::endl;
        std::cout << "Begin parsing..." << std::endl << std::endl;
        outStream << "Begin parsing..." << std::endl << std::endl;
        // 分析
        errorCount = parser.LL1Parse(std::cout);
        if(errorCount > 0)
        {
            std::cout << errorCount << " error(s) generated." << std::endl;
            exit(1);
        }
        parser.LL1Parse(outStream);
    }
    else
    {
        if(!inputTableFileName.empty())
        {
            if(!parser.readLRParseTable(inputTableFileName))
            {
                std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Invalid LR grammar. \033[0m" << std::endl;
                exit(1);
            }
        }
        else if(!parser.calcLRParseTable())
        {
            std::cout << "\033[1m(Parser Generator)\033[0m \033[1;31merror:\033[0m\033[1m Invalid LR grammar. \033[0m" << std::endl;
            exit(1);
        }

        // 打印一下
        //parser.printLRInternalTables(std::cout);
        //parser.printLRInternalTables(outStream);
        // 保存
        if(inputTableFileName.empty() || !outputTableFileName.empty())
        {
            parser.saveLRParseTable(outputTableFileName.empty() ? "LR.tbl" : outputTableFileName);
        }
        std::cout << std::endl;
        outStream << std::endl;
        std::cout << "This is an LR(1) grammar." << std::endl;
        outStream << "This is an LR(1) grammar." << std::endl;
        std::cout << "Begin parsing..." << std::endl << std::endl;
        outStream << "Begin parsing..." << std::endl << std::endl;
        // 分析
        errorCount = parser.LRParse(std::cout);
        if(errorCount > 0)
        {
            std::cout << errorCount << " error(s) generated." << std::endl;
            exit(1);
        }
        parser.LRParse(outStream);
    }
    return 0;
}
#endif
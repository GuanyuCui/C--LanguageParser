#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// 文法结构体
struct Grammar
{
    using SymbolType = std::string;
    // 非终结符
    using NonTerminalType = SymbolType;
    using NonTerminals = std::unordered_set<NonTerminalType>;

    // 终结符
    using TerminalType = SymbolType;
    using Terminals = std::unordered_set<TerminalType>;

    // 起始符号
    using StartSymbolType = NonTerminalType;

    // 产生式(压缩版，键为左端非终结符，值为该非终结符所有产生式的右端)
    using Productions = std::unordered_map<NonTerminalType, std::vector<std::vector<SymbolType>>>;

    // 一条产生式(二元组，左端和右端符号)
    using ProductionType = std::pair<NonTerminalType, std::vector<SymbolType>>;

    // 四元组
    // 非终结符
    NonTerminals nonTerminals;
    // 终结符
    Terminals terminals;
    // 开始符号
    StartSymbolType startSymbol;
    // 产生式
    Productions productions;

    // 判断是否非终结符
    bool isNonTerminal(const NonTerminalType & symbol)
    {
        return (nonTerminals.find(symbol) != nonTerminals.end());
    }
    // 判断是否终结符
    bool isTerminal(const TerminalType & symbol)
    {
        return (terminals.find(symbol) != terminals.end() || symbol == "");
    }
    // 判断是否开始符号
    bool isStartSymbol(const StartSymbolType & symbol)
    {
        return symbol == startSymbol;
    }
};

class Parser
{
    // ------------------------- 公有成员 -------------------------
    // ------------------------- LL(1) 分析 -------------------------
        // LL(1) 预测分析表类型
        using LL1ParseTableType = std::map<
            std::pair<Grammar::NonTerminalType, Grammar::TerminalType>, 
            Grammar::ProductionType>;
        // 两种表类型
        using FirstTableType = std::map< Grammar::NonTerminalType, std::set<Grammar::TerminalType> >;
        using FollowTableType = std::map< Grammar::NonTerminalType, std::set<Grammar::TerminalType> >;
    // ------------------------- LR 分析 -------------------------
        // LR 项目
        // (产生式，其它信息)
        // 产生式 := (左侧符号，同一左端符号中第几个产生式)
        // 其它信息 := (点的位置，展望符号)
        using LRTerm = std::pair<std::pair<Grammar::NonTerminalType, size_t>, std::pair<size_t, Grammar::SymbolType>>;

        // 四类动作
        enum class ACTION
        {
            SHIFT,
            REDUCE,
            GOTO, 
            ACCEPT
        };
        using ActionTableTerm = std::pair<ACTION, std::any>;

        // LR(0) 分析表
        // (状态, 符号) -> (ACTION, 附加信息)
        // 附加信息如下一状态、使用什么产生式归约等
        using LRParseTableType = std::map<
            std::pair<size_t, Grammar::SymbolType>, ActionTableTerm
        >;
    public:
        
        // ------------------------ 构造函数 ------------------------
        // 默认构造函数
        Parser();
        // 复制构造(标记删除)
        Parser(const Parser & other) = delete;

        // ------------------------ 析构函数 ------------------------
        ~Parser();

        // ------------------------ 成员函数 ------------------------
        // 关联文件
        bool openFile(const std::string & srcName);
        // 关闭文件
        void closeFile();
        // 读取语法
        bool readGrammar(const std::string & grmName);
        // ------------------------ LL(1) 语法分析 ------------------------
        // 计算 LL(1) 预测分析表
        bool calcLL1ParseTable();
        // 打印内部表格
        void printLL1InternalTables(std::ostream & out = std::cout);
        // LL(1) 解析
        size_t LL1Parse(std::ostream & out = std::cout);
        // 保存 LL(1) 分析表
        bool saveLL1ParseTable(const std::string & fileName);
        // 读取 LL(1) 分析表
        bool readLL1ParseTable(const std::string & fileName);
        // ------------------------ LR(1) 语法分析 ------------------------
        // 计算 LR 解析表
        bool calcLRParseTable();
        // 打印内部表格
        void printLRInternalTables(std::ostream & out = std::cout);
        // LR 解析
        size_t LRParse(std::ostream & out = std::cout);
        // 保存 LR(1) 分析表
        bool saveLRParseTable(const std::string & fileName);
        // 读取 LR(1) 分析表
        bool readLRParseTable(const std::string & fileName);
        // 错误处理
        void errorProcess(const Types::ParserError & error);
    private:
        // 打印 FIRST
        void printFirstTable(std::ostream & out);
        // 打印 FOLLOW
        void printFollowTable(std::ostream & out);
        // 打印预测分析表
        void printLL1ParseTable(std::ostream & out);

        // 词法分析器
        Lexer lexer;
        // 上下文无关文法
        Grammar grammar = 
        {
            // 非终结符
            {"S", "E", "T", "G", "F", "H"},
            // 终结符
            {"OP_ADD", "OP_SUB", "OP_MUL", "OP_DIV", "DELIM_LPAR", "DELIM_RPAR", "IDENTIFIER", "", Shared::endOfFileChar},
            // 起始符号
            "S",
            // 产生式集合
            {
                { "S", {{"E"}} },
                { "E", {{"T", "G"}} },
                { "G", {{"OP_ADD", "T", "G"}, {"OP_SUB", "T", "G"}, {""}} },
                { "T", {{"F", "H"}} },
                { "H", {{"OP_MUL", "F", "H"}, {"OP_DIV", "F", "H"}, {""}} },
                { "F", {{"DELIM_LPAR", "E", "DELIM_RPAR"}, {"IDENTIFIER"}} }
            }
        };
        
        // ------------------------ LL(1) 语法分析 ------------------------
        // 两个表
        FirstTableType firstTable;
        FollowTableType followTable;
        // LL(1) 预测分析表
        LL1ParseTableType LL1parseTable;
        // ------------------------ LR(0) 语法分析 ------------------------
        // LR 动作表
        LRParseTableType LRparseTable;
};

#endif
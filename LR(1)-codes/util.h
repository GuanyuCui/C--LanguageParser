#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <utility>
#include <any>
#include <bitset>
#include <string>
#include <vector>
#include <stack>
#include <queue>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

namespace Types
{
	// ------------------------- 词法分析 -------------------------

	// Token 类型: 枚举类
	enum class TokenType
	{
		// 初始化状态
		INIT,
		// 错误状态
		ERROR,
		// 文件尾状态
		ENDOFFILE,
		// -------- 关键字 --------
		KEYWORD,
		// -------- 标识符 --------
		IDENTIFIER,
		// -------- 常量 --------
		// 整型常量
		INT_CONST,
		// 浮点常量
		FLOAT_CONST,
		// 字符常量
		CHAR_CONST,
		// 字符串字面量
		STR_LITERAL,
		// -------- 运算符 --------
		// 简单运算符
		// +
		OP_ADD,
		// -
		OP_SUB,
		// *
		OP_MUL,
		// /
		OP_DIV,
		// %
		OP_MOD,
		// >
		OP_GT,
		// <
		OP_LT,
		// !
		OP_LNOT,
		// &
		OP_AND,
		// |
		OP_OR,
		// ~
		OP_NOT,
		// ^
		OP_XOR,
		// .
		OP_DOT,
		// =
		OP_ASN,
		// 复合运算符
		// ->
		OP_ARROW,
		// ++
		OP_INC,
		// --
		OP_DEC,
		// <<
		OP_SHL,
		// >>
		OP_SHR,
		// <=
		OP_LE,
		// >=
		OP_GE,
		// ==
		OP_EQ,
		// !=
		OP_NEQ,
		// &&
		OP_LAND,
		// ||
		OP_LOR,
		// +=
		OP_ADDASN,
		// -=
		OP_SUBASN,
		// *=
		OP_MULASN,
		// /=
		OP_DIVASN,
		// %=
		OP_MODASN,
		// &=
		OP_ANDASN,
		// ^=
		OP_XORASN,
		// |=
		OP_ORASN,
		// <<=
		OP_SHLASN,
		// >>=
		OP_SHRASN,
		// ::
		OP_SCOPE,
		// -------- 分隔符 --------
		// "
		DELIM_DBQUOTE,
		// #
		DELIM_SHARP,
		// '
		DELIM_SGQUOTE,
		// (
		DELIM_LPAR,
		// )
		DELIM_RPAR,
		// ,
		DELIM_COMMA,
		// :
		DELIM_COLON,
		// ;
		DELIM_SEMICOLON,
		// [
		DELIM_LSQBRACKET,
		// ]
		DELIM_RSQBRACKET,
		// {
		DELIM_LCURBRACE,
		// }
		DELIM_RCURBRACE,
		// ?
		DELIM_QUESTION
	};
	
	// 文件中位置: <所在行, 所在列> 对
	using FilePos = std::pair<size_t, size_t>;
	// 分词结果: <类型-内容> 对
    using TokenPair = std::pair<Types::TokenType, std::any>;
	// Lexer 错误: <文件位置, 错误信息> 对
    using LexerError = std::pair<FilePos, std::string>;
	
	// 类型枚举-类型字符串
	using TypeStringTable = std::unordered_map<Types::TokenType, std::string>;
	// 关键字表
	using KeywordsTable = std::vector<std::string>;
	// 运算符字符表
	using OperatorCharTable = std::unordered_set<char>;
	// 分隔符字符-分隔符类型名
	using DelimCharTable = std::unordered_map<char, Types::TokenType>;
	// 标识符名称表
	using IdentifierTable = std::vector<std::string>;
	// 常数表
	using ConstTable = std::vector<std::string>;
	// 符号表
	using SymbolTable = std::vector<std::string>;

	// ------------------------- 语法分析 -------------------------
	// Parser 错误
	using ParserError = std::pair<FilePos, std::string>;
}

namespace Shared
{
	// 仅定义，防止重复
	extern Types::IdentifierTable idTable;
	extern Types::ConstTable constTable;
	extern Types::SymbolTable symbolTable;

	// 终止符
	const std::string endOfFileChar = "(EOF)";

	// 枚举类型到类型名字符串表
	const Types::TypeStringTable typeStrings = \
		Types::TypeStringTable({
			// 初始化状态
			{Types::TokenType::INIT, "INIT"},
			// 错误状态
			{Types::TokenType::ERROR, "ERROR"},
			// 文件结束
			{Types::TokenType::ENDOFFILE, endOfFileChar},
			// -------- 关键字 --------
			{Types::TokenType::KEYWORD, "KEYWORD"},
			// -------- 标识符 --------
			{Types::TokenType::IDENTIFIER, "IDENTIFIER"},
			// -------- 常量 --------
			// 整型常量
			{Types::TokenType::INT_CONST, "INT_CONST"},
			// 浮点常量
			{Types::TokenType::FLOAT_CONST, "FLOAT_CONST"},
			// 字符常量
			{Types::TokenType::CHAR_CONST, "CHAR_CONST"},
			// 字符串字面量
			{Types::TokenType::STR_LITERAL, "STR_LITERAL"},
			// -------- 运算符 --------
			// 简单运算符
			// +
			{Types::TokenType::OP_ADD, "OP_ADD"},
			// -
			{Types::TokenType::OP_SUB, "OP_SUB"},
			// *
			{Types::TokenType::OP_MUL, "OP_MUL"},
			// /
			{Types::TokenType::OP_DIV, "OP_DIV"},
			// %
			{Types::TokenType::OP_MOD, "OP_MOD"},
			// >
			{Types::TokenType::OP_GT, "OP_GT"},
			// <
			{Types::TokenType::OP_LT, "OP_LT"},
			// !
			{Types::TokenType::OP_LNOT, "OP_LNOT"},
			// &
			{Types::TokenType::OP_AND, "OP_AND"},
			// |
			{Types::TokenType::OP_OR, "OP_OR"},
			// ~
			{Types::TokenType::OP_NOT, "OP_NOT"},
			// ^
			{Types::TokenType::OP_XOR, "OP_XOR"},
			// .
			{Types::TokenType::OP_DOT, "OP_DOT"},
			// =
			{Types::TokenType::OP_ASN, "OP_ASN"},
			// 复合运算符
			// ->
			{Types::TokenType::OP_ARROW, "OP_ARROW"},
			// ++
			{Types::TokenType::OP_INC, "OP_INC"},
			// --
			{Types::TokenType::OP_DEC, "OP_DEC"},
			// <<
			{Types::TokenType::OP_SHL, "OP_SHL"},
			// >>
			{Types::TokenType::OP_SHR, "OP_SHR"},
			// <=
			{Types::TokenType::OP_LE, "OP_LE"},
			// >=
			{Types::TokenType::OP_GE, "OP_GE"},
			// ==
			{Types::TokenType::OP_EQ, "OP_EQ"},
			// !=
			{Types::TokenType::OP_NEQ, "OP_NEQ"},
			// &&
			{Types::TokenType::OP_LAND, "OP_LAND"},
			// ||
			{Types::TokenType::OP_LOR, "OP_LOR"},
			// +=
			{Types::TokenType::OP_ADDASN, "OP_ADDASN"},
			// -=
			{Types::TokenType::OP_SUBASN, "OP_SUBASN"},
			// *=
			{Types::TokenType::OP_MULASN, "OP_MULASN"},
			// /=
			{Types::TokenType::OP_DIVASN, "OP_DIVASN"},
			// %=
			{Types::TokenType::OP_MODASN, "OP_MODASN"},
			// &=
			{Types::TokenType::OP_ANDASN, "OP_ANDASN"},
			// ^=
			{Types::TokenType::OP_XORASN, "OP_XORASN"},
			// |=
			{Types::TokenType::OP_ORASN, "OP_ORASN"},
			// <<=
			{Types::TokenType::OP_SHLASN, "OP_SHLASN"},
			// >>=
			{Types::TokenType::OP_SHRASN, "OP_SHRASN"},
			// ::
			{Types::TokenType::OP_SCOPE, "OP_SCOPE"},
			// -------- 分隔符 --------
			// "
			{Types::TokenType::DELIM_DBQUOTE, "DELIM_DBQUOTE"},
			// #
			{Types::TokenType::DELIM_SHARP, "DELIM_SHARP"},
			// '
			{Types::TokenType::DELIM_SGQUOTE, "DELIM_SGQUOTE"},
			// (
			{Types::TokenType::DELIM_LPAR, "DELIM_LPAR"},
			// )
			{Types::TokenType::DELIM_RPAR, "DELIM_RPAR"},
			// ,
			{Types::TokenType::DELIM_COMMA, "DELIM_COMMA"},
			// :
			{Types::TokenType::DELIM_COLON, "DELIM_COLON"},
			// ;
			{Types::TokenType::DELIM_SEMICOLON, "DELIM_SEMICOLON"},
			// [
			{Types::TokenType::DELIM_LSQBRACKET, "DELIM_LSQBRACKET"},
			// ]
			{Types::TokenType::DELIM_RSQBRACKET, "DELIM_RSQBRACKET"},
			// {
			{Types::TokenType::DELIM_LCURBRACE, "DELIM_LCURBRACE"},
			// }
			{Types::TokenType::DELIM_RCURBRACE, "DELIM_RCURBRACE"},
			// ?
			{Types::TokenType::DELIM_QUESTION, "DELIM_QUESTION"} });

	// 判断符号是否在标识符表出现过
	std::pair<bool, size_t> inIDTable(const std::string & identifier);

	// 判断符号是否在符号表出现过
	std::pair<bool, size_t> inSymbolTable(const std::string & symbol);

	// 判断符号是否在常数表出现过
	std::pair<bool, size_t> inConstTable(const std::string & constant);
}

#endif
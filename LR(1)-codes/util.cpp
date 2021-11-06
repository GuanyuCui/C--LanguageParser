#include "util.h"

namespace Shared
{
    Types::IdentifierTable idTable;
	Types::ConstTable constTable;
	Types::SymbolTable symbolTable;

	// 判断符号是否在标识符表出现过
	std::pair<bool, size_t> inIDTable(const std::string & identifier)
	{
		for(size_t i = 0; i < idTable.size(); i++)
		{
			if(idTable[i] == identifier)
			{
				return std::pair<bool, size_t>(true, i);
			}
		}
		return std::pair<bool, size_t>(false, idTable.size());
	}

	// 判断符号是否在符号表出现过
	std::pair<bool, size_t> inSymbolTable(const std::string & symbol)
	{
		for(size_t i = 0; i < symbolTable.size(); i++)
		{
			if(symbolTable[i] == symbol)
			{
				return std::pair<bool, size_t>(true, i);
			}
		}
		return std::pair<bool, size_t>(false, symbolTable.size());
	}
	// 判断符号是否在常数表出现过
	std::pair<bool, size_t> inConstTable(const std::string & constant)
	{
		for(size_t i = 0; i < constTable.size(); i++)
		{
			if(constTable[i] == constant)
			{
				return std::pair<bool, size_t>(true, i);
			}
		}
		return std::pair<bool, size_t>(false, constTable.size());
	}
}
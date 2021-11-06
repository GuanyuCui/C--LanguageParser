#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "parser.h"

class Compiler
{
    public:
        Compiler();
        Compiler(const Compiler & other) = delete;
        ~Compiler();

        void linkFile(const std::string & srcName);
        void parse();
    private:
        Parser parser;
        //
};

#endif
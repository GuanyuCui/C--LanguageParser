#include <iomanip>
#include <iostream>
#include <vector>
#include <cmath>
#include <bitset>


#include "compiler.h"

int main(int argc, char * argv[])
{
    Compiler c0mp1ler;
    //c0mp1ler.linkFile();
    //c0mp1ler.lexicalAnalysis();
    c0mp1ler.parse();
    // c0mp1ler.semanticAnalysis();
    return 0;
}
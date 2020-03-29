#include <Graph500Data.hpp>
#include <iostream>

using namespace graph_tools;

int Graph500Data::Test(int argc, char *argv [])
{
    auto data = Graph500Data::Generate(10, 1);    
    return 0;
}

__attribute__((weak))
int main(int argc, char *argv [])
{
    return Graph500Data::Test(argc, argv);
}

#include "Application.hpp"

#include <iostream>

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    try
    {
        Application application;

        application.run(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

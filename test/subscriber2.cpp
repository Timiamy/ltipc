#include <iostream>
#include <Subscriber.hpp>
#include <typedef.hpp>



int main(int argc, char const *argv[])
{

    (void) argc;
    (void) argv;

    ltsm::Subscriber<Test1,2, 1> sub2;

    while (true)
    {
        Test1 recv = sub2.Recevie();
        std::cout << "Subscriber2 receive:" << std::endl;
        std::cout << recv.l << " " << recv.i << " " << recv.c << std::endl;
    }
    
    return 0;
}

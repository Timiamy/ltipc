#include <iostream>
#include <Subscriber.hpp>
#include <typedef.hpp>



int main(int argc, char const *argv[])
{
    (void) argc;
    (void) argv;

    ltipc::Subscriber<Test1,2, 0> sub1;

    while (true)
    {
        Test1 recv = sub1.Recevie();
        std::cout << "Subscriber1 receive:" << std::endl;
        std::cout << recv.l << " " << recv.i << " " << recv.c << std::endl;
    }
    
    return 0;
}

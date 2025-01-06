#include <iostream>
#include <Publisher.hpp>
#include <typedef.hpp>
#include <thread>


int main(int argc, char const *argv[])
{
    (void) argc;
    (void) argv;

    ltsm::Publisher<Test1,2> pub;

    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "Start to send..." << std::endl;
    Test1 test1{1024,42,'a'};
    Test1 test2{2048,84,'z'};

    pub.Send(&test1);
    pub.Send(&test2);

    std::cout << "Message Sent!" << std::endl;

    getchar();
    
    return 0;
}

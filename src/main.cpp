#include <iostream>
#include <Publisher.hpp>
#include <Subscriber.hpp>
#include <typedef.hpp>
#include <thread>
#include <type_traits>

static constexpr auto kMaxThread = 1;

//struct timespec start, end;

std::chrono::_V2::steady_clock::time_point start;
std::chrono::_V2::steady_clock::time_point end;


template<size_t SubIndex>
void createSubscriberTherad()
{
    std::thread([](){
        ltipc::Subscriber<Test1, kMaxThread, SubIndex + 1> sub;

            Test1 recv = sub.Recevie();
            end = std::chrono::steady_clock::now();
            auto latency = (end- start).count();
            std::cout << "Cost: " << latency << std::endl;
            std::cout << recv.l << " " << recv.i << " " << recv.c << std::endl;

            Test1 recv2 = sub.Recevie();
            end = std::chrono::steady_clock::now();

            latency = (end- start).count();
            std::cout << "Cost2: " << latency << std::endl;
            std::cout << recv2.l << " " << recv2.i << " " << recv2.c << std::endl;
    }).detach();
}

template<size_t... Indexes>
void createSubscriberTherads(std::index_sequence<Indexes...> seq)
{
    (void)seq;
    (createSubscriberTherad<Indexes>(),...);
}

int main(int argc, char const *argv[])
{
    (void) argc;
    (void) argv;

    ltipc::Publisher<Test1,kMaxThread> pub;

    std::cout << "Start to send..." << std::endl;

    createSubscriberTherads(std::make_index_sequence<kMaxThread>{});

    Test1 test1{1024,42,'a'};
    Test1 test2{2048,84,'z'};

    std::this_thread::sleep_for(std::chrono::seconds(1));

    start = std::chrono::steady_clock::now();
    pub.Send(test1);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    start = std::chrono::steady_clock::now();

    pub.Send(test2);


    

    getchar();
    
    return 0;
}

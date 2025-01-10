#include <iostream>
#include <Publisher.hpp>
#include <Subscriber.hpp>
#include <typedef.hpp>
#include <thread>
#include <type_traits>

static constexpr auto kMaxThread = 1;

struct timespec start, end;

template<size_t SubIndex>
void createSubscriberTherad()
{
    std::thread([](){
        ltipc::Subscriber<Test1,kMaxThread, SubIndex> sub;

            Test1 recv = sub.Recevie();
            clock_gettime(CLOCK_MONOTONIC, &end);
            long latency = (end.tv_nsec - start.tv_nsec);
            std::cout << "Cost: " << latency << std::endl;
            std::cout << recv.l << " " << recv.i << " " << recv.c << std::endl;

            Test1 recv2 = sub.Recevie();
            clock_gettime(CLOCK_MONOTONIC, &end);

            latency = (end.tv_nsec - start.tv_nsec);
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

    clock_gettime(CLOCK_MONOTONIC, &start);
    pub.Send(test1);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    clock_gettime(CLOCK_MONOTONIC, &start);
    pub.Send(test2);


    

    getchar();
    
    return 0;
}

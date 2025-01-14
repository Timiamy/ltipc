#include <Publisher.hpp>
#include <Subscriber.hpp>
#include <format>
#include <iostream>
#include <ltlog.hpp>
#include <thread>
#include <type_traits>
#include <typedef.hpp>

static constexpr auto kMaxThread = 1;

struct timespec start, end;

template <size_t SubIndex>
void createSubscriberTherad() {
    std::thread([]() {
        ltipc::Subscriber<Test1, kMaxThread, SubIndex + 1> sub;
        while (true) {
            Test1 recv = sub.Recevie();
            clock_gettime(CLOCK_MONOTONIC, &end);
            LogInfo(std::format("Recv: {:0>3} Cost: {}", recv.l,
                                end.tv_nsec - start.tv_nsec));
        }
    }).detach();
}

template <size_t... Indexes>
void createSubscriberTherads(std::index_sequence<Indexes...> seq) {
    (void)seq;
    (createSubscriberTherad<Indexes>(), ...);
}

int main(int argc, char const *argv[]) {
    (void)argc;
    (void)argv;

    ltipc::Publisher<Test1, kMaxThread> pub;

    std::cout << "Start to send..." << std::endl;

    createSubscriberTherads(std::make_index_sequence<kMaxThread>{});

    for (size_t i = 0; i < 10000; i++) {
        Test1 test1{i, 42, 'a'};
        clock_gettime(CLOCK_MONOTONIC, &start);
        pub.Send(test1);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    getchar();

    return 0;
}

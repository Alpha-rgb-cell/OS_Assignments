#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <pthread.h>
#include <chrono>

int user_main(int argc, char **argv); // Forward declaration of user-defined main

// Mutex for Synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to execute a function pointer in a thread
void* parallel_execution(void* arg) {
    auto* func = reinterpret_cast<std::function<void()>*>(arg);
    (*func)(); // Execute the function pointer
    return nullptr;
}

// Function to demonstrate passing lambda as a parameter
void demonstration(std::function<void()> && lambda) {
    lambda(); // Execute the lambda function
}

// Function to parallelize a 1D loop using pthreads
void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads) {
    // Measure execution time
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    pthread_t threads[numThreads];
    std::function<void()> funcs[numThreads];

    int range = high - low;
    int step = range / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int start = low + i * step;
        int end = (i == numThreads - 1) ? high : start + step;

        // Assign a lambda function to each thread to execute the loop
        funcs[i] = [start, end, &lambda]() {
            for (int j = start; j < end; ++j) {
                lambda(j); // Execute the lambda function for each iteration of the loop
            }
        };

        // Create threads and execute the assigned function
        if (pthread_create(&threads[i], nullptr, parallel_execution, &funcs[i]) != 0) {
            std::cerr << "Error: Thread creation failed!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < numThreads; ++i) {
        if (pthread_join(threads[i], nullptr) != 0) {
            std::cerr << "Error: Thread join failed!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Measure and print execution time
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Execution Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
              << " milliseconds" << std::endl;
}

// Function to parallelize a 2D loop using pthreads
void parallel_for(int low1, int high1, int low2, int high2, std::function<void(int, int)> &&lambda, int numThreads) {
    // Measure execution time
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    pthread_t threads[numThreads];
    std::function<void()> funcs[numThreads];

    int range1 = high1 - low1;
    int step1 = range1 / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int start1 = low1 + i * step1;
        int end1 = (i == numThreads - 1) ? high1 : start1 + step1;

        // Assign a lambda function to each thread to execute the 2D loop
        funcs[i] = [start1, end1, low2, high2, &lambda]() {
            for (int j = start1; j < end1; ++j) {
                for (int k = low2; k < high2; ++k) {
                    lambda(j, k); // Execute the lambda function for each iteration of the 2D loop
                }
            }
        };

        // Create threads and execute the assigned function
        if (pthread_create(&threads[i], nullptr, parallel_execution, &funcs[i]) != 0) {
            std::cerr << "Error: Thread creation failed!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < numThreads; ++i) {
        if (pthread_join(threads[i], nullptr) != 0) {
            std::cerr << "Error: Thread join failed!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    
    // Measure and print execution time
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Execution Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
              << " milliseconds" << std::endl;
}

// Main function to demonstrate lambda functions and execute user-defined main
int main(int argc, char **argv) {
    // Sample lambda function 1
    int x = 5, y = 1;
    auto lambda1 = [x, &y](void) {
        y = 5;
        std::cout << "====== Welcome to Assignment-" << y << " of the CSE231(A) ======\n";
    };
    demonstration(lambda1);

    // Execute user-defined main
    int rc = user_main(argc, argv);

    // Sample lambda function 2
    auto lambda2 = []() {
        std::cout << "====== Hope you enjoyed CSE231(A) ======\n";
    };
    demonstration(lambda2);

    return rc;
}

#define main user_main // Redefine main as user_main

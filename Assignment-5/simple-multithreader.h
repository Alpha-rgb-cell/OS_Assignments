#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>
#include <pthread.h>
#include <chrono>


int user_main(int argc, char **argv);

//Mutex for Synchronization
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


//for parallel execution
void* parallel_execution(void* arg) {
    auto* func = reinterpret_cast<std::function<void()>*>(arg);
    (*func)();
    return nullptr;
}

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> && lambda) {
  lambda();
}



void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads) {
    // Your implementation for parallelizing for loop goes here
    // Create numThreads Pthreads and distribute the work
    // Execute the lambda in parallel
    // Remember to handle synchronization if needed
    // Calculate execution time and print if required

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    pthread_t threads[numThreads];
    std::function<void()> funcs[numThreads];

    int range = high - low;
    int step = range / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int start = low + i * step;
        int end = (i == numThreads - 1) ? high : start + step;

        funcs[i] = [start, end, &lambda]() {
            for (int j = start; j < end; ++j) {
                lambda(j);
            }
        };

        pthread_create(&threads[i], nullptr, parallel_execution, &funcs[i]);
    }

    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Execution Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
              << " milliseconds" << std::endl;
}


void parallel_for(int low1, int high1, int low2, int high2, std::function<void(int, int)> &&lambda, int numThreads) {
    // Your implementation for parallelizing 2D for loops goes here
    // Create numThreads Pthreads and distribute the work for 2D loops
    // Execute the lambda in parallel
    // Handle synchronization if necessary
    // Calculate execution time and print if required
    
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    pthread_t threads[numThreads];
    std::function<void()> funcs[numThreads];

    int range1 = high1 - low1;
    int step1 = range1 / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int start1 = low1 + i * step1;
        int end1 = (i == numThreads - 1) ? high1 : start1 + step1;

        funcs[i] = [start1, end1, low2, high2, &lambda]() {
            for (int j = start1; j < end1; ++j) {
                for (int k = low2; k < high2; ++k) {
                    lambda(j, k);
                }
            }
        };

        pthread_create(&threads[i], nullptr, parallel_execution, &funcs[i]);
    }

    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Execution Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
              << " milliseconds" << std::endl;
}

int main(int argc, char **argv) {
  /* 
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be 
   * explicity captured if they are used inside lambda.
   */
  int x=5,y=1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/[/*by value*/ x, /*by reference*/ &y](void) {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    std::cout<<"====== Welcome to Assignment-"<<y<<" of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);
 
  auto /*name*/ lambda2 = [/*nothing captured*/]() {
    std::cout<<"====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main



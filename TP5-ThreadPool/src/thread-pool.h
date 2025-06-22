/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector
#include "Semaphore.h" // for Semaphore
#include <queue>       // for queue
#include <mutex>       // for mutex
#include <condition_variable> // for condition_variable
#include <atomic>      // for atomic

using namespace std;


/**
 * @brief Represents a worker in the thread pool.
 * 
 * The `worker_t` struct contains information about a worker 
 * thread in the thread pool. Should be includes the thread object, 
 * availability status, the task to be executed, and a semaphore 
 * (or condition variable) to signal when work is ready for the 
 * worker to process.
 */
typedef struct worker {
    thread ts;                     // El thread del worker
    function<void(void)> thunk;    // La tarea a ejecutar
    bool available;                // Indica si el worker está disponible
    Semaphore sem{0};             // Semáforo para sincronización, inicializado en 0
    int id;                        // Identificador único del worker
    bool active;                   // Indica si el worker está activo

    worker() : available(true), active(true), id(0) {}  // Constructor por defecto
} worker_t;

class ThreadPool {
  public:

  /**
  * Constructs a ThreadPool configured to spawn up to the specified
  * number of threads.
  */
    ThreadPool(size_t numThreads);

  /**
  * Schedules the provided thunk (which is something that can
  * be invoked as a zero-argument function without a return value)
  * to be executed by one of the ThreadPool's threads as soon as
  * all previously scheduled thunks have been handled.
  */
    void schedule(const function<void(void)>& thunk);

  /**
  * Blocks and waits until all previously scheduled thunks
  * have been executed in full.
  */
    void wait();

  /**
  * Waits for all previously scheduled thunks to execute, and then
  * properly brings down the ThreadPool and any resources tapped
  * over the course of its lifetime.
  */
    ~ThreadPool();
    
  private:

    void worker(int id);
    void dispatcher();
    thread dt;                              // dispatcher thread handle
    vector<worker_t> wts;                   // worker thread handles
    atomic<bool> done;                      // flag to indicate the pool is being destroyed
    mutex queueLock;                        // mutex to protect the queue of tasks
    
    // Variables privadas necesarias
    queue<function<void(void)>> taskQueue;  // cola de tareas pendientes
    Semaphore queueSem;                     // semáforo para la cola de tareas
    Semaphore workerAvailableSem{0};        // semáforo para señalizar workers disponibles
    size_t numThreads;                      // número de threads en el pool
    mutex waitLock;                         // mutex para wait()
    condition_variable waitCV;              // variable de condición para wait()
    condition_variable taskCV;              // variable de condición para esperar tareas
    atomic<size_t> pendingTasks;                    // contador de tareas pendientes
    atomic<size_t> availableWorkers;                // contador de workers disponibles
    queue<int> availableWorkerQueue;        // cola de workers disponibles para asignación O(1)
    mutex workerQueueLock;                  // mutex para proteger la cola de workers

    /* It is incomplete, there should be more private variables to manage the structures... 
    * *
    */
  
    /* ThreadPools are the type of thing that shouldn't be cloneable, since it's
    * not clear what it means to clone a ThreadPool (should copies of all outstanding
    * functions to be executed be copied?).
    *
    * In order to prevent cloning, we remove the copy constructor and the
    * assignment operator.  By doing so, the compiler will ensure we never clone
    * a ThreadPool. */
    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};
#endif

/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
using namespace std;

// constructor
ThreadPool::ThreadPool(size_t numThreads) 
    : wts(numThreads), 
      done(false), 
      queueSem(0),
      numThreads(numThreads),
      pendingTasks(0),
      availableWorkers(numThreads) {  // Inicialmente todos los workers están disponibles
    
    // Inicializar workers
    for (size_t i = 0; i < numThreads; i++) {
        wts[i].id = i;
        wts[i].available = true;
        wts[i].ts = thread(&ThreadPool::worker, this, i);
        workerAvailableSem.signal();  // Señalizar que hay un worker disponible
    }
    
    // Iniciar thread dispatcher
    dt = thread(&ThreadPool::dispatcher, this);
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    lock_guard<mutex> lock(queueLock);
    taskQueue.push(thunk);
    pendingTasks++;
    queueSem.signal();
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(waitLock);
    waitCV.wait(lock, [this]() { return pendingTasks == 0; });
}

void ThreadPool::worker(int id) {
    while (true) {
        wts[id].sem.wait();  // Esperar por una tarea
        
        if (!wts[id].active) {
            break;  // Worker debe terminar
        }
        
        // Ejecutar la tarea
        wts[id].thunk();
        
        {
            lock_guard<mutex> lock(queueLock);
            wts[id].available = true;
            availableWorkers++;
            workerAvailableSem.signal();  // Señalizar que hay un worker disponible
        }
        
        {
            lock_guard<mutex> lock(waitLock);
            pendingTasks--;
            if (pendingTasks == 0) {
                waitCV.notify_all();
            }
        }
    }
}

void ThreadPool::dispatcher() {
    while (!done) {
        queueSem.wait();  // Esperar por tareas en la cola
        
        if (done) break;
        
        // Obtener una tarea de la cola
        function<void(void)> task;
        {
            lock_guard<mutex> lock(queueLock);
            if (!taskQueue.empty()) {
                task = taskQueue.front();
                taskQueue.pop();
            }
        }
        
        // Esperar a que haya un worker disponible
        workerAvailableSem.wait();
        
        // Buscar un worker disponible
        bool assigned = false;
        while (!assigned && !done) {
            for (size_t i = 0; i < numThreads && !assigned; i++) {
                lock_guard<mutex> lock(queueLock);
                if (wts[i].available) {
                    wts[i].available = false;
                    availableWorkers--;
                    wts[i].thunk = task;
                    wts[i].sem.signal();
                    assigned = true;
                }
            }
        }
    }
}

// destructor
ThreadPool::~ThreadPool() {
    // Primero esperamos que terminen todas las tareas pendientes
    wait();
    
    // Luego marcamos que el pool debe terminar
    done = true;
    queueSem.signal();  // Despertar al dispatcher
    workerAvailableSem.signal(); // Por si el dispatcher está esperando un worker
    
    // Terminar todos los workers
    for (size_t i = 0; i < numThreads; i++) {
        wts[i].active = false;
        wts[i].sem.signal();
    }
    
    // Esperar que terminen todos los threads
    dt.join();
    for (size_t i = 0; i < numThreads; i++) {
        wts[i].ts.join();
    }
}

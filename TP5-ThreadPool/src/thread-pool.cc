/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include <chrono>
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
        availableWorkerQueue.push(i);  // Agregar worker a la cola de disponibles
        workerAvailableSem.signal();  // Señalizar que hay un worker disponible
    }
    
    // Iniciar thread dispatcher
    dt = thread(&ThreadPool::dispatcher, this);
}

// producer
void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (done) {
        throw runtime_error("ThreadPool has been destroyed");
    }
    
    if (!thunk) {
        throw runtime_error("Cannot schedule nullptr function");
    }
    
    {
        lock_guard<mutex> lock(queueLock);
        taskQueue.push(thunk);
    }
    pendingTasks++;  // Incremento atómico
    queueSem.signal();
}

void ThreadPool::wait() {
    unique_lock<mutex> lock(waitLock);
    waitCV.wait(lock, [this]() { 
        return pendingTasks == 0; 
    });
}

void ThreadPool::worker(int id) {
    while (true) {
        wts[id].sem.wait();  // Esperar por una tarea
        
        {
            lock_guard<mutex> lock(queueLock);
            if (!wts[id].active) {
                break;  // Worker debe terminar
            }
        }
        
        // Ejecutar la tarea
        function<void(void)> task_to_execute;
        {
            lock_guard<mutex> lock(queueLock);
            task_to_execute = wts[id].thunk;  // Copiar la tarea localmente
        }
        task_to_execute();  // Ejecutar la tarea sin lock
        
        {
            lock_guard<mutex> lock(queueLock);
            wts[id].available = true;
        }
        
        availableWorkers++;  // Incremento atómico
        
        {
            lock_guard<mutex> lock(workerQueueLock);
            availableWorkerQueue.push(id);  // Agregar worker a la cola de disponibles
            workerAvailableSem.signal();  // Señalizar que hay un worker disponible
        }
        
        // Decrementar pendingTasks y notificar si es necesario
        size_t remaining = --pendingTasks;
        if (remaining == 0) {
            lock_guard<mutex> lock(waitLock);
            waitCV.notify_all();
        }
    }
}

// consumer
void ThreadPool::dispatcher() {
    while (!done) {
        queueSem.wait();  // Esperar por tareas en la cola
        
        if (done) break;  // Verificar done después de despertar
        
        // Obtener una tarea de la cola
        function<void(void)> task;
        {
            unique_lock<mutex> lock(queueLock);
            if (taskQueue.empty()) {
                continue;  // No hay tareas, continuar al siguiente ciclo
            }
            task = taskQueue.front();
            taskQueue.pop();
        }
        
        // Esperar a que haya un worker disponible
        workerAvailableSem.wait();
        
        if (done) break;  // Verificar done nuevamente
        
        // Obtener un worker disponible de la cola
        int worker_id;
        {
            lock_guard<mutex> lock(workerQueueLock);
            worker_id = availableWorkerQueue.front();
            availableWorkerQueue.pop();
        }
        
        // Asignar la tarea al worker
        {
            lock_guard<mutex> lock(queueLock);
            wts[worker_id].available = false;
            wts[worker_id].thunk = task;  // Asignar thunk bajo lock
        }
        
        availableWorkers--;  // Decremento atómico
        
        wts[worker_id].sem.signal();
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
        {
            lock_guard<mutex> lock(queueLock);
            wts[i].active = false;
        }
        wts[i].sem.signal();
    }
    
    // Esperar que terminen todos los threads
    dt.join();
    for (size_t i = 0; i < numThreads; i++) {
        wts[i].ts.join();
    }
}

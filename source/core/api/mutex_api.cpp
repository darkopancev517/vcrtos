#include <vcrtos/mutex.h>

#include "core/mutex.hpp"

using namespace vc;

void mutex_init(void *instances, mutex_t *mutex)
{
#if VCRTOS_CONFIG_MULTIPLE_INSTANCE_ENABLE
    mutex->instance = instances;
#else
    (void) instances;
#endif
    mutex->queue.next = NULL;
}

void mutex_lock(mutex_t *mutex)
{
    Mutex &mtx = *static_cast<Mutex *>(mutex);
    mtx.lock();
}

void mutex_try_lock(mutex_t *mutex)
{
    Mutex &mtx = *static_cast<Mutex *>(mutex);
    mtx.try_lock();
}

void mutex_unlock(mutex_t *mutex)
{
    Mutex &mtx = *static_cast<Mutex *>(mutex);
    mtx.unlock();
}

void mutex_unlock_and_sleeping(mutex_t *mutex)
{
    Mutex &mtx = *static_cast<Mutex *>(mutex);
    mtx.unlock_and_sleeping_current_thread();
}

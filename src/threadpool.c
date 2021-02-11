#include "threadpool.h"
#include "barrier.h"
#include "taskqueue.h"

#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

int threadpool_init(struct threadpool *tp, size_t num_threads)
{
  int err;

  err = taskqueue_init(&tp->queue);
  if (err) { return err; }

  if ((tp->threads = malloc(num_threads * sizeof(*tp->threads))) == NULL) {
    return -1;
  }

  tp->num_threads = num_threads;

  for (size_t i = 0; i < num_threads; ++i) {
    err = pthread_create(&tp->threads[i], NULL, taskqueue_basic_worker_func,
                         &tp->queue);
    if (err) { return err; }
  }

  return 0;
}

void threadpool_wait(struct threadpool *tp)
{
  taskqueue_wait_for_complete(&tp->queue);
}

int threadpool_push_task(struct threadpool *tp, struct task t)
{
  return taskqueue_push(&tp->queue, t);
}

void threadpool_notify(struct threadpool *tp) { taskqueue_notify(&tp->queue); }

void threadpool_barrier_task_func(void *arg)
{
  struct barrier *bar = (struct barrier *)arg;
  int ret = barrier_wait(bar);

  if (ret == BARRIER_FINAL_THREAD) {
    // If we were the final thread to exit the barrier, clean up after ourselves
    barrier_destroy(bar);
  }
}

int threadpool_push_barrier(struct threadpool *tp)
{
  int err;

  struct barrier *bar = malloc(sizeof(*bar));
  if (bar == NULL) { return -1; }

  err = barrier_init(bar, tp->num_threads);
  if (err) { return err; }

  err = taskqueue_push_n(
      &tp->queue,
      (struct task){.func = threadpool_barrier_task_func, .arg = bar},
      tp->num_threads);
  if (err) { return err; }

  return 0;
}


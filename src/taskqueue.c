#include "taskqueue.h"
#include "pool.h"

#include <pthread.h>
#include <stdlib.h>

void taskqueue_entry_init(struct taskqueue_entry *te, void (*func)(void *),
                          void *func_args)
{
  te->next = te->prev = NULL;
  te->task.func = func;
  te->task.func_args = func_args;
}

void task_execute(struct task *t) { t->func(t->func_args); }

int taskqueue_init(struct taskqueue *q)
{
  int err;

  q->head = q->tail = NULL;
  q->count = 0;

  err = pthread_mutex_init(&q->lock, NULL);
  if (err) { return err; }

  err = pthread_cond_init(&q->notify, NULL);
  if (err) { return err; }

  err = pool_init(&q->entry_pool, TASKQUEUE_DEFAULT_POOLSIZE,
                  sizeof(struct taskqueue_entry));
  if (err) { return err; }

  return 0;
}

int taskqueue_push(struct taskqueue *q, struct task t)
{
  int err = 0;
  pthread_mutex_lock(&q->lock);

  struct taskqueue_entry *te = pool_acquire(&q->entry_pool);
  if (te == NULL) {
    err = -1;
    goto done;
  }

  te->next = q->head;
  te->prev = NULL;
  te->task = t;

  if (q->head == NULL) { q->tail = te; }
  else {
    q->head->prev = te;
  }
  q->head = te;

  q->count += 1;

done:
  pthread_mutex_unlock(&q->lock);
  return err;
}

int taskqueue_pop_locked(struct taskqueue *q, struct task *t)
{

  if (t == NULL) { return -1; }

  if (q->count == 0) { return -1; }

  struct taskqueue_entry *te = q->tail;

  if (q->count != 1) { q->tail = q->tail->prev; }
  else {
    q->head = q->tail = NULL;
  }

  q->count--;

  *t = te->task;

  return pool_release(&q->entry_pool, te);
}

int taskqueue_pop(struct taskqueue *q, struct task *t)
{
  int err;
  pthread_mutex_lock(&q->lock);

  err = taskqueue_pop_locked(q, t);

  pthread_mutex_unlock(&q->lock);

  return err;
}

size_t taskqueue_count(struct taskqueue *q)
{
  size_t count;

  pthread_mutex_lock(&q->lock);
  count = q->count;
  pthread_mutex_unlock(&q->lock);

  return count;
}

int taskqueue_wait_for_work(struct taskqueue *q, struct task *t)
{
  int ret;
  pthread_mutex_lock(&q->lock);

  while (q->count == 0) {
    pthread_cond_wait(&q->notify, &q->lock);
  }
  if ((ret = taskqueue_pop_locked(q, t)) != 0) {
    // NOTE: This is an error condition.
  }
  else {
    q->num_running += 1;
  }
  pthread_mutex_unlock(&q->lock);
  return ret;
}

void taskqueue_wait_for_complete(struct taskqueue *q)
{
  pthread_mutex_lock(&q->lock);

  while (!(q->count == 0 && q->num_running == 0)) {
    pthread_cond_wait(&q->notify, &q->lock);
  }

  pthread_mutex_unlock(&q->lock);
}

int taskqueue_notify(struct taskqueue *q)
{
  return pthread_cond_broadcast(&q->notify);
}

void taskqueue_task_complete(struct taskqueue *q)
{
  pthread_mutex_lock(&q->lock);
  q->num_running -= 1;
  pthread_mutex_unlock(&q->lock);
  taskqueue_notify(q);
}

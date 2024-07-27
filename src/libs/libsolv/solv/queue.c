/*
 * Copyright (c) 2007, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

/*
 * queue.c
 *
 */

#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "util.h"

#define EXTRA_SPACE 8
#define EXTRA_SPACE_HEAD 8

void
queue_init(Queue *q)
{
  q->alloc = q->elements = 0;
  q->count = q->left = 0;
}

void
queue_init_clone(Queue *t, Queue *s)
{
  if (!s->elements)
    {
      t->alloc = t->elements = 0;
      t->count = t->left = 0;
      return;
    }
  t->alloc = t->elements = solv_malloc2(s->count + EXTRA_SPACE, sizeof(Id));
  if (s->count)
    memcpy(t->alloc, s->elements, s->count * sizeof(Id));
  t->count = s->count;
  t->left = EXTRA_SPACE;
}

void
queue_init_buffer(Queue *q, Id *buf, int size)
{
  q->alloc = 0;
  q->elements = buf;
  q->count = 0;
  q->left = size;
}

void
queue_free(Queue *q)
{
  if (q->alloc)
    solv_free(q->alloc);
  q->alloc = q->elements = 0;
  q->count = q->left = 0;
}

void
queue_alloc_one(Queue *q)
{
  if (!q->alloc)
    {
      q->alloc = solv_malloc2(q->count + EXTRA_SPACE, sizeof(Id));
      if (q->count)
	memcpy(q->alloc, q->elements, q->count * sizeof(Id));
      q->elements = q->alloc;
      q->left = EXTRA_SPACE;
    }
  else if (q->alloc != q->elements)
    {
      int l = q->elements - q->alloc;
      if (q->count)
        memmove(q->alloc, q->elements, q->count * sizeof(Id));
      q->elements -= l;
      q->left += l;
    }
  else
    {
      q->elements = q->alloc = solv_realloc2(q->alloc, q->count + EXTRA_SPACE, sizeof(Id));
      q->left = EXTRA_SPACE;
    }
}

/* make room for an element in front of queue */
void
queue_alloc_one_head(Queue *q)
{
  int l;
  if (!q->alloc || !q->left)
    queue_alloc_one(q);
  l = q->left > EXTRA_SPACE_HEAD ? EXTRA_SPACE_HEAD : q->left;
  if (q->count)
    memmove(q->elements + l, q->elements, q->count * sizeof(Id));
  q->elements += l;
  q->left -= l;
}

void
queue_insert(Queue *q, int pos, Id id)
{
  queue_push(q, id);	/* make room */
  if (pos < q->count - 1)
    {
      memmove(q->elements + pos + 1, q->elements + pos, (q->count - 1 - pos) * sizeof(Id));
      q->elements[pos] = id;
    }
}

void
queue_delete(Queue *q, int pos)
{
  if (pos >= q->count)
    return;
  if (pos < q->count - 1)
    memmove(q->elements + pos, q->elements + pos + 1, (q->count - 1 - pos) * sizeof(Id));
  q->left++;
  q->count--;
}

void
queue_insert2(Queue *q, int pos, Id id1, Id id2)
{
  queue_push(q, id1);	/* make room */
  queue_push(q, id2);	/* make room */
  if (pos < q->count - 2)
    {
      memmove(q->elements + pos + 2, q->elements + pos, (q->count - 2 - pos) * sizeof(Id));
      q->elements[pos] = id1;
      q->elements[pos + 1] = id2;
    }
}

void
queue_delete2(Queue *q, int pos)
{
  if (pos >= q->count)
    return;
  if (pos == q->count - 1)
    {
      q->left++;
      q->count--;
      return;
    }
  if (pos < q->count - 2)
    memmove(q->elements + pos, q->elements + pos + 2, (q->count - 2 - pos) * sizeof(Id));
  q->left += 2;
  q->count -= 2;
}

void
queue_insertn(Queue *q, int pos, int n, Id *elements)
{
  if (n <= 0)
    return;
  if (pos > q->count)
    pos = q->count;
  if (q->left < n)
    {
      int off;
      if (!q->alloc)
	queue_alloc_one(q);
      off = q->elements - q->alloc;
      q->alloc = solv_realloc2(q->alloc, off + q->count + n + EXTRA_SPACE, sizeof(Id));
      q->elements = q->alloc + off;
      q->left = n + EXTRA_SPACE;
    }
  if (pos < q->count)
    memmove(q->elements + pos + n, q->elements + pos, (q->count - pos) * sizeof(Id));
  if (elements)
    memcpy(q->elements + pos, elements, n * sizeof(Id));
  else
    memset(q->elements + pos, 0, n * sizeof(Id));
  q->left -= n;
  q->count += n;
}

void
queue_deleten(Queue *q, int pos, int n)
{
  if (n <= 0 || pos >= q->count)
    return;
  if (pos + n >= q->count)
    n = q->count - pos;
  else
    memmove(q->elements + pos, q->elements + pos + n, (q->count - n - pos) * sizeof(Id));
  q->left += n;
  q->count -= n;
}

/* allocate room for n more elements */
void
queue_prealloc(Queue *q, int n)
{
  int off;
  if (n <= 0 || q->left >= n)
    return;
  if (!q->alloc)
    queue_alloc_one(q);
  off = q->elements - q->alloc;
  q->alloc = solv_realloc2(q->alloc, off + q->count + n + EXTRA_SPACE, sizeof(Id));
  q->elements = q->alloc + off;
  q->left = n + EXTRA_SPACE;
}


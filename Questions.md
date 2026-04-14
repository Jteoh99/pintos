# Project 1: Threads

## Alarm Clock

#### DATA STRUCTURES

>A1: Copy here the declaration of each new or changed struct or struct member, global or static variable, typedef, or enumeration.  Identify the purpose of each in 25 words or less.

- struct sleep_element { struct list_elem elem; int64_t wake_tick; struct thread *thread; } — holds sleeping thread and wake time.
- static struct list sleepers; — sorted sleeping thread list.
- static int64_t next_tick_to_awake; — earliest wake time optimization.
- struct thread got int64_t wake_tick and list_elem elem for sleep list.


#### ALGORITHMS

>A2: Briefly describe what happens in a call to timer_sleep(),
>including the effects of the timer interrupt handler.

- timer_sleep(ticks) computes wake tick, inserts current thread into sleepers list ordered by wake_tick, blocks thread.
- Timer interrupt increments tick count and calls wake-up helper.
- Wake-up helper checks sleepers from front; unblocks threads whose wake_ticks <= current tick.

>A3: What steps are taken to minimize the amount of time spent in
>the timer interrupt handler?
    
- Keep sleepers sorted by wake time and track earliest wake tick.
- In interrupt, compare only front element wake_tick vs current tick.
- Wake only as many ready threads as needed, then return quickly.

#### SYNCHRONIZATION

>A4: How are race conditions avoided when multiple threads call
>timer_sleep() simultaneously?

- timer_sleep disables interrupts during list insert and block sequence.
- Global sleepers list protected by interrupts-off atomic region.

>A5: How are race conditions avoided when a timer interrupt occurs
>during a call to timer_sleep()?

- Interrupts are disabled around timer_sleep modifications.
- timer interrupt handler also runs with interrupts disabled and updates shared list atomically.

#### RATIONALE

>A6: Why did you choose this design?  In what ways is it superior to
>another design you considered?

- Chosen design: sorted sleep list + minimal interrupt work. Superior to full scan because it scales and keeps interrupt latency low.


## Priority Scheduling

#### DATA STRUCTURES

>B1: Copy here the declaration of each new or changed struct or struct member, global or static variable, typedef, or enumeration.  Identify the purpose of each in 25 words or less.

- struct thread { int priority; int original_priority; struct lock *waiting_lock; ... }
    - priority: effective current priority.
    - original_priority: base priority before donation.
    - waiting_lock: lock the thread is blocked on.
- static struct list ready_list; priority-ordered ready queue.
- struct lock { struct thread *holder; struct semaphore semaphore; }
- struct semaphore { unsigned value; struct list waiters; }

>B2: Explain the data structure used to track priority donation.
>Use ASCII art to diagram a nested donation.  (Alternately, submit a
>.png file.)

- Tracking donation: each thread records waiting_lock and original_priority; thread_donate_priority traverses chain.

- T2 (pri 50)
    - waiting_lock -> L1
- L1 holder -> T1 (orig 30, donated 50)
    - waiting_lock -> L2
- L2 holder -> T3 (orig 40, donated 50)

#### ALGORITHMS

>B3: How do you ensure that the highest priority thread waiting for
>a lock, semaphore, or condition variable wakes up first?

- All waiters stored in ordered list by priority.
- For semaphore up: select max-priority waiter (via list_max with priority comparator).
- For locks: lock acquire uses semaphore semantics.
- For condvar signal: wake front of ordered waiters list.

>B4: Describe the sequence of events when a call to lock_acquire()
>causes a priority donation.  How is nested donation handled?

- lock_acquire(lock):
    - If lock held, set current waiting_lock = lock.
    - Call donation helper on holder.
    - sema_down blocks.
- Donation helper updates holder priority to max of own priority and waiters.
- Nested: if holder itself waits on another lock, propagate donation recursively up chain.

>B5: Describe the sequence of events when lock_release() is called
>on a lock that a higher-priority thread is waiting for.

- lock_release(lock):
    1. Set holder = NULL.
    2. sema_up: wakes highest-priority waiter.
    3. Recompute current thread priority from original + any remaining donations (thread_refresh_priority).
    4. Potential yield if higher-priority now ready.

#### SYNCHRONIZATION

>B6: Describe a potential race in thread_set_priority() and explain
>how your implementation avoids it.  Can you use a lock to avoid
>this race?

- Race: two threads call thread_set_priority concurrently while one holds the lock and donations need recalculation.
- Avoided by running with interrupts disabled in thread and scheduler primitives, and by always using atomic list operations.
- A lock isn't appropriate for scheduler internal states, disabling interrupts is correct.

#### RATIONALE

>B7: Why did you choose this design?  In what ways is it superior to
>another design you considered?

- Chose “donation + recompute from waiters” approach because it’s simple, safe, and supports nested donation.
- Superior to “single donated field only” because it correctly handles un-donation on release and multiple waiters.
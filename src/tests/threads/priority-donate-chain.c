/* The main thread set its priority to PRI_MIN and creates 7 threads 
   (thread 1..7) with priorities PRI_MIN + 3, 6, 9, 12, ...
   The main thread initializes 8 locks: lock 0..7 and acquires lock 0.

   When thread[i] starts, it first acquires lock[i] (unless i == 7.)
   Subsequently, thread[i] attempts to acquire lock[i-1], which is held by
   thread[i-1], except for lock[0], which is held by the main thread.
   Because the lock is held, thread[i] donates its priority to thread[i-1],
   which donates to thread[i-2], and so on until the main thread
   receives the donation.

   After threads[1..7] have been created and are blocked on locks[0..7],
   the main thread releases lock[0], unblocking thread[1], and being
   preempted by it.
   Thread[1] then completes acquiring lock[0], then releases lock[0],
   then releases lock[1], unblocking thread[2], etc.
   Thread[7] finally acquires & releases lock[7] and exits, allowing 
   thread[6], then thread[5] etc. to run and exit until finally the 
   main thread exits.

   In addition, interloper threads are created at priority levels
   p = PRI_MIN + 2, 5, 8, 11, ... which should not be run until the 
   corresponding thread with priority p + 1 has finished.
  
   Written by Godmar Back <gback@cs.vt.edu> */ 
/* Đầu tiên thread chính được set mức ưu tiên là PRI_MIN . Sau đó tạo luồng 
(1->7) với các mức độ ưu tiên là PRI_MIN+3,6,9,12,... 
Luồng chính khởi tạo 8 khóa ( 0-> 7) và thread chính nhận khóa 0.
Khi thread[i] bắt đầu , đầu tiên nó nhận lock[i]( với i!=7) 
Sau đó thread[i] sẽ cố gắng lấy lock[i-1] được giữ bởi lock[i-1],
ngoại trừ lock[0] được giữ bởi thread chính .
Vì lock được giữ , thread[i] dành priority của nó cho thread[i-1] , thread[i-1]
cho thread[i-1] và cứ tiếp tục như thế cho đến chuỗi chính đến khi thread chính 
nhận được.
Sau đó khi thread[1...7] được tạo và bị blocked bởi lock[0...7] , thread chính
giải phóng lock[0] , bỏ blocked thread[1] và lấy lock[0] phủ sang thread[1]
Sau đó , thread[1] hoàn thành do đã thu thập lock của thread chính , sau đó thread[1]
làm xong việc trả lock[1] cho thread2 , rồi cứ thế tương tự
Thread[7] cuối cùng cũng có lock[7] và thực hiện nhiệm vụ rồi thoát ra , cho phép thread[6],...
chạy và thoát cho đến cuối cùng ctrinh.
Ngoài ra , các thread được tạo bởi các mức ưu tiên p=PRI_MIN+2,5,8,11 .. sẽ không chạy cho đến khi
thread tương ứng với priority +1 kết thúc.
*/
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/synch.h"
#include "threads/thread.h"

#define NESTING_DEPTH 8

struct lock_pair
  {
    struct lock *second;
    struct lock *first;
  };

static thread_func donor_thread_func;
static thread_func interloper_thread_func;

void
test_priority_donate_chain (void) 
{
  int i;  
  struct lock locks[NESTING_DEPTH - 1];		// tạo 1 mảng chứa các lock
  struct lock_pair lock_pairs[NESTING_DEPTH];	// Tạo 1 mảng chứa các lock_pairs

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  thread_set_priority (PRI_MIN);	priority của thread chính = PRI_MIN

  for (i = 0; i < NESTING_DEPTH - 1; i++)
    lock_init (&locks[i]);		// Khởi tạo các lock

  lock_acquire (&locks[0]);
  msg ("%s got lock.", thread_name ());

  for (i = 1; i < NESTING_DEPTH; i++)
    {
      char name[16];
      int thread_priority;

      snprintf (name, sizeof name, "thread %d", i);
      thread_priority = PRI_MIN + i * 3;
      lock_pairs[i].first = i < NESTING_DEPTH - 1 ? locks + i: NULL;
      lock_pairs[i].second = locks + i - 1;

      thread_create (name, thread_priority, donor_thread_func, lock_pairs + i);
      msg ("%s should have priority %d.  Actual priority: %d.",
          thread_name (), thread_priority, thread_get_priority ());

      snprintf (name, sizeof name, "interloper %d", i);
      thread_create (name, thread_priority - 1, interloper_thread_func, NULL);
    }

  lock_release (&locks[0]);
  msg ("%s finishing with priority %d.", thread_name (),
                                         thread_get_priority ());
}

static void
donor_thread_func (void *locks_) 
{
  struct lock_pair *locks = locks_;

  if (locks->first)
    lock_acquire (locks->first);

  lock_acquire (locks->second);
  msg ("%s got lock", thread_name ());

  lock_release (locks->second);
  msg ("%s should have priority %d. Actual priority: %d", 
        thread_name (), (NESTING_DEPTH - 1) * 3,
        thread_get_priority ());

  if (locks->first)
    lock_release (locks->first);

  msg ("%s finishing with priority %d.", thread_name (),
                                         thread_get_priority ());
}

static void
interloper_thread_func (void *arg_ UNUSED)
{
  msg ("%s finished.", thread_name ());
}

// vim: sw=2

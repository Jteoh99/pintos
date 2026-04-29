#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <stdint.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "lib/kernel/stdio.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
static bool fetch_user_uint32 (const struct intr_frame *, unsigned, uint32_t *);
static bool get_user_byte (const uint8_t *uaddr, uint8_t *dst);
static bool read_user_uint32 (const uint32_t *uaddr, uint32_t *dst);
static bool validate_user_string (const char *uaddr);
static bool validate_user_range (const void *uaddr, unsigned size);
static int syscall_write (int fd, const void *buffer, unsigned size);
static void exit_invalid_user_access (void);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  uint32_t syscall_number;
  if (!fetch_user_uint32 (f, 0, &syscall_number))
    exit_invalid_user_access ();

  switch (syscall_number)
    {
    case SYS_HALT:
      shutdown_power_off ();
      break;

    case SYS_EXIT:
      {
        uint32_t status;
        if (!fetch_user_uint32 (f, 1, &status))
          exit_invalid_user_access ();
        thread_current ()->exit_status = (int) status;
        process_exit ();
        thread_exit ();
        break;
      }

    case SYS_EXEC:
      {
        uint32_t cmd_line;
        if (!fetch_user_uint32 (f, 1, &cmd_line))
          exit_invalid_user_access ();

        if (!validate_user_string ((const char *) cmd_line))
          exit_invalid_user_access ();

        f->eax = (uint32_t) process_execute ((const char *) cmd_line);
        break;
      }

    case SYS_WAIT:
      {
        uint32_t child_tid;
        if (!fetch_user_uint32 (f, 1, &child_tid))
          exit_invalid_user_access ();

        f->eax = (uint32_t) process_wait ((tid_t) child_tid);
        break;
      }

    case SYS_WRITE:
      {
        uint32_t fd;
        uint32_t buffer;
        uint32_t size;

        if (!fetch_user_uint32 (f, 1, &fd) ||
            !fetch_user_uint32 (f, 2, &buffer) ||
            !fetch_user_uint32 (f, 3, &size))
          exit_invalid_user_access ();

        if (!validate_user_range ((const void *) buffer, (unsigned) size))
          exit_invalid_user_access ();

        f->eax = (uint32_t) syscall_write ((int) fd,
                                          (const void *) buffer,
                                          (unsigned) size);
        break;
      }

    default:
      exit_invalid_user_access ();
    }
}

static void
exit_invalid_user_access (void)
{
  thread_current ()->exit_status = -1;
  process_exit ();
  thread_exit ();
}

static bool
fetch_user_uint32 (const struct intr_frame *f, unsigned index, uint32_t *dst)
{
  const uint32_t *uaddr = ((const uint32_t *) f->esp) + index;
  return read_user_uint32 (uaddr, dst);
}

static bool
get_user_byte (const uint8_t *uaddr, uint8_t *dst)
{
  if (uaddr == NULL || !is_user_vaddr (uaddr))
    return false;

  void *kaddr = pagedir_get_page (thread_current ()->pagedir, uaddr);
  if (kaddr == NULL)
    return false;

  *dst = * (const uint8_t *) kaddr;
  return true;
}

static bool
read_user_uint32 (const uint32_t *uaddr, uint32_t *dst)
{
  uint8_t *out = (uint8_t *) dst;
  const uint8_t *addr = (const uint8_t *) uaddr;

  for (size_t i = 0; i < sizeof *dst; i++)
    {
      if (!get_user_byte (addr + i, out + i))
        return false;
    }
  return true;
}

static bool
validate_user_string (const char *uaddr)
{
  uint8_t byte;

  while (true)
    {
      if (!get_user_byte ((const uint8_t *) uaddr, &byte))
        return false;
      if (byte == '\0')
        return true;
      uaddr++;
    }
}

static bool
validate_user_range (const void *uaddr, unsigned size)
{
  const uint8_t *addr = (const uint8_t *) uaddr;
  uint8_t byte;

  for (unsigned i = 0; i < size; i++)
    if (!get_user_byte (addr + i, &byte))
      return false;

  return true;
}

static int
syscall_write (int fd, const void *buffer, unsigned size)
{
  if (fd != 1)
    return -1;

  putbuf (buffer, size);
  return (int) size;
}

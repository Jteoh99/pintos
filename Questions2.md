# Project 2: User Programs

## Argument Passing

#### DATA STRUCTURES

>A1: Copy here the declaration of each new or changed struct or struct member, global or static variable, typedef, or enumeration. Identify the purpose of each in 25 words or less.

- `char *cmd_line` in `process_launch_info` — stores the full command line copy passed from `process_execute()` to `start_process()` so user args survive until setup.
- `char *args[128]` and `void *arg_addresses[128]` in `setup_stack()` — temporary arrays that hold parsed argument tokens and their stack addresses for correct reverse-stack construction.
- `int argc` in `setup_stack()` — counts parsed arguments so `argv` and `argc` are pushed correctly and the stack frame matches the C ABI.

#### ALGORITHMS

>A2: Briefly describe how you implemented argument parsing. How do you arrange for the elements of argv[] to be in the right order?
>How do you avoid overflowing the stack page?

- Parse `cmd_line` with `strtok_r()` to split the command string into tokens separated by spaces.
- Store each token pointer in `args[]` as I parse forward, then push argument strings onto user stack in reverse order so `argv[0]` ends up first in memory.
- After pushing strings, word-align the stack, push a null sentinel, then push pointers to each argument in reverse order, then `argv`, `argc`, and a fake return address.
- To avoid overflowing the stack page, build the stack downward from `PHYS_BASE` and check each allocation implicitly by keeping all data within the one user stack page. The kernel page is preallocated and the layout size is bounded by maximum token counts and lengths.

>In Pintos, the kernel separates commands into a executable name and arguments. In Unix-like systems, the shell does this separation. Identify at least two advantages of the Unix approach.

- Shell parsing model lets kernel remain simpler and trust that user-space programs provide already-tokenized arguments, reducing kernel complexity.
- Enables richer command syntax, quoting, pipes, and redirection in user space without forcing the kernel to understand shell grammar.
- Separates responsibilities cleanly. The shell handles command-line parsing and quoting, while the kernel handles execution semantics and security.
    

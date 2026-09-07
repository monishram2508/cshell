# Custom Shell

An interactive Unix-like shell written in C, built directly on POSIX system calls
(`fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `open`, `sigaction`). No third-party
libraries are used.

## Build and Run

```
make        # builds the executable ./shell
./shell     # start the shell
make clean  # remove the executable and object files
```

The directory the shell is launched from becomes the shell's home directory for that
session. Exit with `exit` or Ctrl+D on an empty line.

## Features

**Prompt** — `<username@hostname:cwd>`, with the home directory shown as `~` and
sub-directories as `~/sub`. Username comes from `getpwuid(getuid())`, hostname from
`gethostname()`; neither is hard-coded.

**Built-ins** (run in the shell process, no `exec`): `echo`, `pwd`, `cd`, `history`,
and `exit`.

- `echo` joins its arguments with single spaces; runs of spaces and tabs collapse.
- `pwd` prints the absolute path from `getcwd()`.
- `cd` supports no argument, `~`, `~/path`, `..`, `-` (prints the destination path),
  and any path. More than one argument is an error.
- `history` stores the last 20 commands and displays the last 10, persisted across
  sessions in a file in the shell's home directory. A command identical to the
  immediately preceding one is not stored.

**External commands** — executed by `fork` + `execvp` in a child process. The shell
waits for foreground commands and returns immediately for background commands ending
in `&`.

**I/O redirection** — `<` and `>`, individually or together, applied in the child via
`open` + `dup2` before `exec`.

**Pipelines** — `|` with an arbitrary number of stages (up to 16). All stages are
forked before the shell waits for any of them, and every pipe descriptor is closed in
both the children and the parent so readers reliably see EOF.

**Signals**

- Ctrl+C terminates the foreground command only. The shell installs a `SIGINT`
  handler so it survives; the handler is reset to the default by `exec` in children,
  so they terminate normally. Background processes are placed in their own process
  group with `setpgid`, so the terminal never signals them.
- Ctrl+D on an empty line exits cleanly.
- Background processes are reaped in a `SIGCHLD` handler (`waitpid` with `WNOHANG` in
  a loop), so no zombies remain.

## Source Layout

| File | Responsibility |
| --- | --- |
| `main.c` | Read-eval loop, dispatch between built-ins and external commands |
| `parser.c` | Tokenizer and command-line parser |
| `display.c` | Prompt construction and `~` substitution |
| `builtins.c` | Built-in dispatch, `echo`, `pwd`, `exit` |
| `cd.c` | `cd`, including previous-directory tracking |
| `history.c` | History buffer, persistence, and display |
| `execute.c` | Process creation, redirection, pipelines |
| `signals.c` | Signal handlers and foreground-process tracking |
| `shell.h` | Shared `Command` / `CommandLine` types and size limits |

## Implementation Notes

The parser tokenizes character by character rather than with `strtok`, treating `|`,
`&`, `<`, and `>` as self-delimiting tokens. Surrounding whitespace is therefore
optional: `ls>out.txt` and `ls > out.txt` parse identically. Quoted sections are
recorded as quoted so that a quoted operator (`echo "a|b"`) is not mistaken for a real
one. The parsed result is a `CommandLine` holding one `Command` per pipeline stage,
which is the single structure the executor consumes.

Pipe wiring is applied before explicit redirection, so `<` and `>` override the pipe
for the stage they appear on, matching bash.

Built-ins normally run in the shell process. When a built-in is piped or redirected it
runs in the forked child instead, so that its output can be redirected; this matches
bash and is why `cd dir > file` does not change the shell's directory.

`SIGINT` is installed without `SA_RESTART` so that an interrupted `getline` returns
`EINTR` and the main loop can print a fresh prompt. `SIGCHLD` uses `SA_RESTART` so a
finishing background job does not disturb input. Since the `SIGCHLD` handler may reap a
foreground child before `waitpid` sees it, `ECHILD` from the foreground wait is treated
as normal completion.

## Assumptions

1. The shell's home directory is the directory it was launched from, as stated in the
   specification. `$HOME` is not used.
2. The history file is `.shell_history`, stored in the shell's home directory.
3. For history comparison, runs of whitespace between words are normalised to a single
   space, so `echo hi` and `echo   hi` are treated as the same command. Trailing
   whitespace is preserved and does distinguish two commands. Leading whitespace is
   ignored.
4. The `history` command is itself recorded before it runs, so it appears as the last
   line of its own output. Commands that fail or are not found are also recorded;
   blank lines are not.
5. Quotes (`"` and `'`) are stripped and their contents kept intact, including spaces
   and operator characters. Escape sequences, environment variables, `;`, `&&`, `||`,
   `>>`, and globbing are not handled, as they are outside the specification.
6. `&` is recognised only as the final token of a line.
7. `exit` is provided as an additional built-in for convenience; the specification does
   not require it.
8. Ctrl+Z and Ctrl+`\` are ignored by the shell and its children. Without job control,
   a stopped foreground child would block the shell's `waitpid` indefinitely, so
   suspension is disabled rather than left in a state the shell cannot recover from.
9. Nothing is printed when a background process finishes. Reporting it would require
   formatted output inside a signal handler, which is not async-signal-safe.
10. `pwd` and the prompt report the physical working directory from `getcwd()`, so
    symbolic links in the path appear resolved.
11. Fixed limits: 63 arguments per command, 16 pipeline stages, 128 tokens per line,
    and 255 characters per token. Exceeding any of these produces an error message and
    the line is rejected; input line length itself is unbounded.

## Error Handling

System call failures are reported with `perror`, prefixed by the relevant name: the
command for a failed `exec`, the filename for a failed redirection, and `cd` for a
failed directory change. Parse errors are prefixed with `shell:`. All errors are
written to standard error, and no error path terminates the shell.

# Custom Shell

An interactive Unix-like shell written in C, built directly on POSIX system calls
(`fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `open`, `sigaction`). No third-party
libraries are used.

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
3. Before a command is recorded in history, the terminating newline is removed and runs
   of whitespace *between* words are normalised to a single space, so `echo hi` and
   `echo  hi` are treated as the same command. Whitespace at the start and end of the
   line is left exactly as typed and is significant: `echo hi ` and `echo hi   ` are
   different commands, as are ` echo hi` and `  echo hi`. Normalising the interior
   follows the same idea as the `echo` built-in, where repeated separators between
   arguments carry no meaning; the surrounding whitespace is preserved because nothing in
   the specification licenses discarding it.
4. Leading whitespace is preserved rather than used as a signal, and a command typed with
   a leading space is still recorded. This matches bash with its default settings. Some
   shells are configured to discard space-prefixed commands (bash with
   `HISTCONTROL=ignorespace`, zsh with `HIST_IGNORE_SPACE`), but that is an opt-in
   setting rather than default behaviour, so it is not imitated here.
5. The `history` command is itself recorded before it runs, so it appears as the last
   line of its own output. Commands that fail or are not found are also recorded. Blank
   and whitespace-only lines are not recorded, as they carry no command to repeat.
6. Quotes (`"` and `'`) are stripped and their contents kept intact, including spaces
   and operator characters. Escape sequences, environment variables, `;`, `&&`, `||`,
   `>>`, and globbing are not handled, as they are outside the specification.
7. `&` is recognised only as the final token of a line.
8. `exit` is provided as an additional built-in for convenience; the specification does
   not require it.
9. Ctrl+Z and Ctrl+`\` are ignored by the shell and its children. Without job control,
   a stopped foreground child would block the shell's `waitpid` indefinitely, so
   suspension is disabled rather than left in a state the shell cannot recover from.
10. Nothing is printed when a background process finishes. Reporting it would require
    formatted output inside a signal handler, which is not async-signal-safe.
11. `pwd` and the prompt report the physical working directory from `getcwd()`, so
    symbolic links in the path appear resolved.
12. Input is read with `getline` and no line-editing library such as GNU Readline is
    used, since third-party libraries are not permitted. The shell therefore has none of
    the interactive conveniences of a terminal's own shell: the arrow keys do not move
    the cursor or recall earlier commands, and there is no tab completion. An arrow key
    sends a raw escape sequence, which is read as ordinary text, so pressing the up arrow
    and Enter reports `^[[A: No such file or directory`. A tab is treated as whitespace,
    so `ech` followed by Tab is simply the word `ech`. Past commands can be viewed with
    the `history` built-in but must be retyped to run again.
13. The limits in the table below are fixed at compile time. Exceeding any of them
    produces an error message and the line is rejected without being run.

## Compile-Time Limits

| Limit | Value | Notes |
| --- | --- | --- |
| Arguments per command | 63 | Includes the command name; one array slot is reserved for the `NULL` terminator that `execvp` requires |
| Pipeline stages | 16 | A pipeline of *n* stages uses *n* − 1 pipes |
| Tokens per input line | 128 | Operators (`\|`, `&`, `<`, `>`) count as individual tokens |
| Characters per token | 255 | Measured after quote removal |
| Commands retained in history | 20 | The oldest is discarded once the limit is reached |
| Commands shown by `history` | 10 | The most recent ones |
| Hostname buffer | 256 bytes | Truncated if the host name is longer |
| Path buffers | `PATH_MAX` | Falls back to 4096 where the system does not define it |
| Input line length | Unbounded | `getline` grows its buffer as needed |

## Error Handling

System call failures are reported with `perror`, prefixed by the relevant name: the
command for a failed `exec`, the filename for a failed redirection, and `cd` for a
failed directory change. Parse errors are prefixed with `shell:`. All errors are
written to standard error, and no error path terminates the shell.

## Compilation and Execution

Compile from the project directory:

```
make
```

This compiles each source file with `gcc -Wall -Wextra -g` and links them into an
executable named `shell`. The build produces no warnings.

Run the shell:

```
./shell
```

The directory the shell is started from becomes its home directory for that session
and is displayed as `~` in the prompt. Commands are typed at the prompt in the usual
way, for example:

```
<user@host:~> ls -l | grep txt > out.txt
<user@host:~> sleep 5 &
<user@host:~> cd ..
```

Exit with the `exit` built-in, or by pressing Ctrl+D on an empty line. Ctrl+C
interrupts the running foreground command without terminating the shell.

Remove the executable and the object files:

```
make clean
```

### Environment

The shell uses only C standard library and POSIX functions, so it builds on Linux and
macOS without modification. It was developed and tested on macOS, where `gcc` invokes
Apple Clang, and it compiles cleanly with the same sources under `gcc` on Linux; no
platform-specific code or conditional compilation is used.

Two portability details are worth noting. `PATH_MAX` is taken from `<limits.h>` with a
4096-byte fallback defined in `shell.h`, since the constant is optional in POSIX.
`HOST_NAME_MAX` is not used at all, as it is unavailable on macOS; a fixed 256-byte
buffer is used for the hostname instead.

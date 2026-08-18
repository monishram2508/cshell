# custom-shell

![C](https://img.shields.io/badge/language-C-00599C?logo=c&logoColor=white)
![Make](https://img.shields.io/badge/build-Make-427819?logo=gnu&logoColor=white)
![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)

A Unix-like interactive shell built from scratch in C, implemented directly on top of POSIX system calls rather than any shell-building library.

It parses and runs commands the way a real shell does: forking child processes, `exec`-ing external programs, wiring up file descriptors for I/O redirection and multi-stage pipelines, and handling signals so the shell itself stays alive while foreground jobs can still be interrupted. A small set of commands (`cd`, `pwd`, `echo`, `history`) are implemented as true built-ins — run in the shell's own process rather than spawned as subprocesses — with persistent, file-backed command history across sessions.

## Features

- Dynamic shell prompt showing user, host, and current directory (with `~` substitution for the home directory)
- Built-in commands: `echo`, `pwd`, `cd` (including `cd -` for previous directory), `history`
- Persistent command history stored across sessions
- Foreground and background (`&`) process execution
- Input/output redirection (`<`, `>`)
- Multi-stage pipelines (`|`)
- Signal handling: `Ctrl+C` interrupts only the running foreground job (shell survives), `Ctrl+D` exits cleanly, background processes are reaped to avoid zombies

## Usage

_Build and run instructions coming soon._

## License

MIT — see [LICENSE](LICENSE).

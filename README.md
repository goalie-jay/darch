# DArch (Direct Archiver)

A dead-simple, non-compressing archiving tool.

## Prerequisites

- A C compiler

## Syntax

### Archiving

`darch -a <object 1> [more objects ...] -o <output.archive>`

### Extracting

`darch -x <archive name> -o <output directory>`

## Supported Platforms

- [x] macOS
- [x] Linux
- [x] Solaris
- [x] FreeBSD
- [x] OpenBSD
- [x] Literally any UNIX or just POSIX-compliant system
- [x] Windows (Folder attributes break and I have zero intention of fixing them, but other than that it works more or less as expected)
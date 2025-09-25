# DArch

A dead-simple, non-compressing archiving tool.

## Prerequisites

- dotnet >=8.0

## Syntax

### Archiving

`darch -a <object 1> [more objects ...] -o <output.archive>`

### Extracting

`darch -x <archive name> -o <output directory>`

## Supported platforms

- [x] macOS
- [x] Linux
- [ ] Windows (eventually)
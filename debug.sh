#!/bin/bash -ex

# forces an interrupt to happen on windows
gdb ./demo -ex "b ../src/backtrace.c:force_interrupt" -ex run

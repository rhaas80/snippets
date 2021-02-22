tasker
======

A very simple minded TCP based job scheduler (no MPI, base TCL only)

## Usage

```
aprun -n $RANKS -d 1 -cc none  tclsh tasker.tcl $TASKS $ROOT_HOST $PORT
```

where `ROOT_HOST` is the hostname of rank 0 (which will act as the amanger) and
`TASKS` is a text file with one line per command to execute.

Please see `demo.pbs` for a full example.

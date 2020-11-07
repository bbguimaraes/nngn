impero
======

`impero` is a graphical command executor, intended to be used via a keyboard
shortcut.  Available commands are displayed and optionally filtered with the
contents of a text field.  When one is selected using `Tab/Shift-Tab` + `Enter`,
it is executed and the program exits.

Available commands are read from files, separated by new-line characters.  The
`$XDG_CONFIG_DIR/impero/commands.txt` (or equivalent for the platform) is read,
unless the `--defaults no` argument is specified.  An additional file can be
specified with `--from-file path/to/file`.

```sh
$ # Reads commands from `$XDG_CONFIG_DIR/impero/commands.txt`.
$ impero
$ # Reads commands from a file
$ impero --from-file path/to/file
```

building
--------

Both `autotools` and `cmake` builds are supported.  `impero` uses Qt for the
graphical use interface.  The only dependencies are Qt's Core and Wigets
modules (see [`configure.ac`](./configure.ac) and
[`CMakeLists.txt`](./CMakeLists)).

```sh
$ # autotols build
$ ./configure && make
$ # cmake build
$ cmake -B . && make
```

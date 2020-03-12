# Grapher

Karl Yerkes / 2020-03-11

This uses the Tiny C Compiler (TCC) library (libtcc). On macOS, install with `brew install libtcc`. On Windows, try `choco install tinycc`. Linux can `sudo apt install libtcc-dev`.

This is a sort of graphing calculator for C.


## TODO

This example still needs some work.

- Comment and clean the example code; it's still gross
- a callback that adjusts the range of evaluation or domain
  + `double x0() {return -10;}`

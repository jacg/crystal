# -*-Makefile-*-

# Needed to make `"$@"` usable in recipes
set positional-arguments := true

default:
  just run --beam-on 10

test *REST: install
  meson setup build/test test
  meson compile -C build/test
  meson install -C build/test
  sh install/test/run-each-test-in-separate-process.sh "$@"

catch2-demo *REST:
  echo "$@"
  meson setup build/test test
  meson compile -C build/test
  meson install -C build/test
  install/test/bin/catch2-demo-test "$@"

build:
  meson setup build/crystal src
  meson compile -C build/crystal

install: build
  meson install -C build/crystal

run *ARGS: install
  #!/usr/bin/env sh
  sh execute-with-nixgl-if-needed.sh ./install/crystal/bin/crystal "$@"
  exit $?

clean:
  rm build install -rf

alias r := run
alias s := setup
alias c := compile

build := join(justfile_directory(), 'build')

default:
  @just --summary

run: compile
  @{{build}}/clox

setup reconfigure='f':
  @meson {{ if reconfigure == 'f' { 'setup' } else { '--reconfigure' } }} {{build}}

compile:
  @meson compile -C {{build}}

format:
  # TODO

test:
  # TODO

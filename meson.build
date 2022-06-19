project('clox', 'c', version: '0.1.0')

inc = include_directories('include')
src = []

foreach s: ['main', 'chunk', 'memory']
  src += 'src' / (s + '.c' )
endforeach

exe = executable(
  'clox', src, include_directories: inc)

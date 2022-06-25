project(
  'clox', 'c', version: '0.1.0',
  default_options: ['warning_level=3'])

c_args = []
buildtype = get_option('buildtype')

if buildtype.startswith('debug')
  c_args += '-DDEBUG'
  c_args += '-DDEBUG_TRACE_EXECUTION'
endif

inc = include_directories('include')
src = []

c_files = [
  'main', 'chunk', 'compiler', 'memory', 
  'debug', 'value', 'vm', 'scanner']

foreach s: c_files
  src += 'src' / (s + '.c' )
endforeach

exe = executable(
  'clox', src, include_directories: inc, c_args: c_args)

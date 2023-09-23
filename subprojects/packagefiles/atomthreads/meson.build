project('atomthreads', 'c')

atomthreads_sources = files(
  'kernel/atomkernel.c',
  'kernel/atommutex.c',
  'kernel/atomqueue.c',
  'kernel/atomsem.c',
  'kernel/atomtimer.c',
  'ports/avrxmega/atomport-private.c',
  'ports/avrxmega/atomport.c',
  'ports/avrxmega/atomport-asm.S'
)

compiler_args=[
  '-DATOM_STACK_CHECKING',
]

add_project_arguments(compiler_args , language : 'c')

atomthreads_inc = include_directories('/usr/avr/include','./kernel', './ports/avrxmega', './tests')
atomthreads_lib = library('atomthreads', atomthreads_sources, include_directories: atomthreads_inc)
atomthreads_dep = declare_dependency(link_with : atomthreads_lib, include_directories : atomthreads_inc)
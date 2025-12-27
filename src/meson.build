include = include_directories('../include')
curldep = dependency('libcurl', required: true)
yamldep = dependency('yaml-cpp', required: true)

executable('bupm', ['main.cc', 'about.cc', 'help.cc', 'fetch.cc', 'install.cc'], dependencies: [curldep, yamldep], include_directories: include, install: true)

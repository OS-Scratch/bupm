include = include_directories('../include')
curldep = dependency('libcurl', required: true)
yamldep = dependency('yaml-cpp', required: true)
tardep = dependency('libarchive', required: true)

executable('bupm', ['main.cc', 'about.cc', 'help.cc', 'fetch.cc', 'install.cc', 'repo.cc'], dependencies: [tardep, curldep, yamldep], include_directories: include, install: true)

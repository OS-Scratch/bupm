include = include_directories('../include')
curldep = dependency('libcurl', required: true)
tomldep = dependency('tomlplusplus', required: true)
tardep = dependency('libarchive', required: true)
sodiumdep = dependency('libsodium', required: true)

executable('bupm', ['main.cc', 'about.cc', 'help.cc', 'fetch.cc', 'install.cc', 'repo.cc'], dependencies: [tardep, curldep, tomldep, sodiumdep], include_directories: include, install: true)

project('libtouch', 'c',
		    version : '0.1',
		    default_options : ['c_std=c18'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)



install_headers('libtouch.h')
libtouch = library('touch', 'libtouch.c', dependencies : m_dep, install : true)

pkgconfig = import('pkgconfig')
pkgconfig.generate(libtouch)

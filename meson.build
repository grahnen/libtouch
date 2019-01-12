project('libtouch', 'c',
		    default_options : ['c_std=c18'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)


library('libtouch', 'libtouch.c', dependencies : m_dep)
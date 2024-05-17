# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def options(opt):
    pass

def configure(conf):
    conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')
    conf.env['lcrypto'] = conf.check(mandatory = True, lib = 'crypto', uselib_store = 'OPENSSL')

def build(bld):
    module = bld.create_ns3_module('upenn-cis553', ['core', 'network', 'netanim'])
    module.source = [
        'ls-routing-protocol/ls-routing-protocol.cc',
        'ls-routing-protocol/ls-message.cc',
        'ls-routing-protocol/ls-routing-helper.cc',
        'dv-routing-protocol/dv-routing-protocol.cc',
        'dv-routing-protocol/dv-message.cc',
        'dv-routing-protocol/dv-routing-helper.cc',
        'test-app/test-app.cc',
        'test-app/test-app-message.cc',
        'test-app/test-app-helper.cc',
        'common/ping-request.cc',
        'common/penn-log.cc',
        'common/penn-routing-protocol.cc',
        'common/penn-application.cc',
        'common/test-result.cc',
        'penn-search/penn-search.cc',
        'penn-search/penn-chord.cc',
        'penn-search/penn-chord-message.cc',
        'penn-search/penn-search-message.cc',
        'penn-search/penn-search-helper.cc',
        'penn-search/grader-logs.cc'
        ]
    module.use.append("OPENSSL")
    headers = bld(features='ns3header')
    headers.module = 'upenn-cis553'
    headers.source = [
        'common/penn-log.h',
        'common/ping-request.h',
        'common/penn-routing-protocol.h',
        'common/penn-application.h',
        'common/test-result.h',
        'ls-routing-protocol/ls-message.h',
        'ls-routing-protocol/ls-routing-protocol.h',
        'ls-routing-protocol/ls-routing-helper.h',
        'dv-routing-protocol/dv-message.h',
        'dv-routing-protocol/dv-routing-protocol.h',
        'dv-routing-protocol/dv-routing-helper.h',
        'test-app/test-app.h',
        'test-app/test-app-message.h',
        'test-app/test-app-helper.h',
        'penn-search/penn-search.h',
        'penn-search/penn-chord.h',
        'penn-search/penn-chord-message.h',
        'penn-search/penn-search-message.h',
        'penn-search/penn-search-helper.h',
        'penn-search/penn-key-helper.h',
        'penn-search/grader-logs.h'
        ]

    # bld.ns3_python_bindings()



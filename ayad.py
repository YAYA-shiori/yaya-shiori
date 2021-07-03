# -*- coding: utf-8 -*-
#
#  ayad.py - a (Real) AYA loader for ninix
#  Copyright (C) 2004 by linjian
#  Copyright (C) 2004-2011 by Shyouzou Sugitani <shy@users.sourceforge.jp>
#  Copyright (C) 2011 by henryhu
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License (version 2) as
#  published by the Free Software Foundation.  It is distributed in the
#  hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
#  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#  PURPOSE.  See the GNU General Public License for more details.
#

import os
import sys
import traceback
from ctypes import *
import _ctypes
try:
    aya = cdll.LoadLibrary("libaya5.so")
except:
    print "aya load fail!"
    traceback.print_exc()
    aya = None


class Shiori:
    def __init__(self, dll_name):
        self.dll_name = dll_name
        self.pathdic = []
        self.reqdic = []

    def use_saori(self, saori):
        self.saori = saori

    def find(self, topdir, dll_name): ## FIXME
        result = 0
        if aya:
            if os.path.isfile(os.path.join(topdir, 'aya.txt')):
                result = 205
            elif dll_name is not None and \
                 os.path.isfile(os.path.join(topdir,
                                             ''.join((dll_name[:-3], 'txt')))):
                result = 105
        return result

    def show_description(self):
        sys.stdout.write(
            'Shiori: Real Direct AYA loader for ninix\n'
            '        Copyright (C) 2004 by linjian\n'
            '        Copyright (C) 2004-2011 by Shyouzou Sugitani\n'
            '        Copyright (C) 2011 by henryhu\n')

    def get_charset(self, topdir, dll_name): ## FIXME
        charset = 'Shift_JIS'
        f = open(os.path.join(topdir, ''.join((dll_name[:-3], 'txt'))))
        for line in f:
            if 'charset' in line:
                charset = line.split(',')[1]
                charset = charset.strip()
                break
        if charset == 'default':
            charset = 'Shift_JIS'
        return charset

    def load(self, topdir):
        self.dir = topdir
        self.charset = self.get_charset(self.dir, self.dll_name)
        if aya:
            if self.dir.endswith(os.sep):
                topdir = self.dir
            else:
                topdir = ''.join((self.dir, os.sep))
            path = create_string_buffer(topdir)
            self.pathdic += [path] # so python would not free it
#            print self.pathdic
            # since aya would free it (...)
            # we must not allow python to free it
            ret = aya.load(path, len(topdir))
#            print "load result: %d" % ret
            return ret
        else:
            return 0

    def unload(self):
        global aya
        if aya:
            aya.unload()
            # force Python to unload the library
            # else the next load would not function properly
            try:
                _ctypes.FreeLibrary(aya._handle)
            except:
                pass
            try:
                _ctypes.dlclose(aya._handle)
            except:
                pass
            aya = cdll.LoadLibrary("libaya5.so")

    __req_charset = 'GET SHIORI/3.0\r\n' \
                    'ID: charset\r\n' \
                    'Sender: ninix-aya\r\n' \
                    'SecurityLevel: local\r\n\r\n'

    def request(self, req_string):
        if aya:
            if req_string == self.__req_charset:
                return 'SHIORI/3.0 200 OK\r\n' \
                       'Sender: AYA\r\n' \
                       'Value: %s\r\n\r\n' % self.charset
            reqf = aya.request
            reqf.restype = c_char_p
            request = create_string_buffer(req_string)
            # since aya may free it (...)
            # we must not allow python to free it
            self.reqdic += [request] # so python would not free it
            rlen = c_long(len(request))
#            print "request:"
#            print req_string
            ret = reqf(request, byref(rlen))
#            print "result len: %d" % rlen.value
#            print ret
            return ret
        else:
            return '' # FIXME


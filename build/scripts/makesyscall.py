#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import sys,os,re
import time,logging

rootdirs = ['./board',         \
            './bootloader',    \
            './devices',       \
            './example',       \
            './framework',     \
            './include',       \
            './kernel',        \
            './platform',      \
            './security',      \
            './test',          \
            './tools',         \
            './utility']

filterType = ['gif','png','bmp','jpg','jpeg','rar','zip',
            'ico','apk','ipa','doc','docx','xls','jar',
            'xlsx','ppt','pptx','pdf','gz','pyc','class']

filterOutType = ['h','c','cpp','s','S','ld']

num = 0
syscall_num = 0
symbol_list = []

# DEBUG < INFO < WARNING < ERROR < CRITICAL
logging.basicConfig(level=logging.WARNING)


def search_symbols(path=None, cont=None):
    if not path or not cont:
        print('path or searchString is empty')
        return
    _loopFolder(path, cont)

    return

def _loopFolder(path, cont):
    arr = path.split('/')
    if not arr[-1].startswith('.'):            #Do not check hidden folders
        if os.path.isdir(path):
            folderList = os.listdir(path)
#            print folderList
            for x in folderList:
                _loopFolder(path + "/" + x, cont)
        elif os.path.isfile(path):
            _verifyContent(path, cont)

    return

def _verifyContent(path, cont):
    if path.split('.')[-1].lower() in filterType:
        return
    global num
    global symbol_list
    try:
        fh = open(path, 'r+')
        fhContent = fh.read()
        fh.close()
        symbols = re.findall(cont, fhContent, re.M | re.S)
        if symbols:
#                print symbols
	        symbol_list.extend(symbols)
	        num += 1
#                print ("%s" % (path))
    except:
        print "File '" + path + "'can't be read"

    return

def _writeSyscallHeader(cr_path, sh_path, sn_path):
    fcr = open(cr_path, 'r')               # read copyright
    copyright = fcr.read()
    fcr.close()

    fsh = open(sh_path, "w+")              # creat syscall_tbl.h
    fsh.seek(0, 0)
    fsh.write(copyright)
    fsh.write("\n")

    if os.path.exists(sn_path):
        fsn = open(sn_path, "a+")              # read from syscall_num
    else:
        fsn = open(sn_path, "w+")              # read from syscall_num
    fsnContent = fsn.read()
    symbols = re.findall(r"\d+\s(.*?)\s\".*?\"\s\".*?\"\n", fsnContent, re.M | re.S)
#    print symbols
#    print symbols
    global symbol_list
    for symbol in symbol_list:                          # write to syscall_num
        if symbol[1] not in symbols:
            str_num = repr(len(symbols)) + " " + symbol[1] + " " + "\"" + symbol[0].replace("\n", "") + "\"" + " " + symbol[2].replace("\n", "")
            logging.info(str_num)                       # print the data for write to syscall_num
            fsn.write(str_num + "\n")
            symbols.append(symbol[1])

	fsn.flush()
    fsn.seek(0, 0)
    fsnContent = fsn.read()
    newsymbols = re.findall(r"(\d+)\s(.*?)\s\"(.*?)\"\s\"(.*?)\"\n", fsnContent, re.M | re.S)
#    print newsymbols
    global syscall_num
    syscall_num = len(newsymbols)
    for symbol in newsymbols:                     # according to syscall_num to implementation syscall_tbl.h
#        print symbol
        fsh.write("#if (" + symbol[2] + ")\n")
        strdef = "#define SYS_" + symbol[1].upper() + " " + symbol[0] + "\n"
        strsysc = "SYSCALL(SYS_" + symbol[1].upper() + ", " + symbol[1] + ")"
        fsh.write(strdef + strsysc + "\n")
        fsh.write("#endif" + "\n\n")

    fsn.close()
    fsh.close()

    return


def _writeSyscallUapi(cr_path, sc_path, sn_path, ui_path):
    fcr = open(cr_path, 'r')               # read copyright
    copyright = fcr.read()
    fcr.close()

    fui = open(ui_path, 'r')               # read usyscall include
    usys_incl = fui.read()
    fui.close()

    fsc = open(sc_path, "w+")              # creat syscall_uapi.c
    fsc.seek(0, 0)
    fsc.write(copyright)
    fsc.write(usys_incl)
    fsc.write("\n")

    fsn = open(sn_path, 'r')               # read usyscall include
    fsnContent = fsn.read()
    fsn.close()

    newsymbols = re.findall(r"(\d+)\s(.*?)\s\"(.*?)\"\s\"(.*?)\"\n", fsnContent, re.M | re.S)
#    print newsymbols
    for symbol in newsymbols:                     # according to syscall_num to implementation syscall_tbl.h
#        print symbol
        fsc.write("#if (" + symbol[2] + ")\n" + symbol[3] + "\n" + "{\n" + "    ")
        elements = re.findall(r"(.*?)" + symbol[1] + r"\((.*?)\)$", symbol[3], re.M | re.S)
#        print elements
        for element in elements:
#            print element[1]
            args = element[1].split(',')
            i = 0
            for arg in args:
                while args[i].count("(") != args[i].count(")"):
                    args[i] = args[i] + "," + args[i+1]
                    args.pop(i+1)
                i += 1
            arg_nu = len(args)

            if arg_nu == 1:
                if args[0].strip() == r"void" or args[0].strip() == r"":
                    arg_nu = 0
#            print args
#            print arg_nu

        if element[0].strip() != r"void":
            fsc.write("return ")
        fsc.write("SYS_CALL" + str(arg_nu) + "(SYS_" + symbol[1].upper() + ", " + element[0].strip())
        i = 0
        if arg_nu == 0:
            fsc.write(r");")
        else:
            for arg in args:
                if "(" in arg.strip():
                    u = arg.strip().split("(*")
                    u1 = u[0] + "(*)(" + u[1].split(")(")[1]
                    u2 = u[1].split(")(")[0]
                elif "*" in arg.strip():
                    u1 = arg[0:(arg.index("*") + arg.count("*"))]
                    u2 = arg[-(len(arg) - len(u1)):]
                else:
                    u = arg.strip().split(" ", arg.strip().count(" "))
                    u2 = u[arg.strip().count(" ")]
                    u1 = arg[0:(len(arg) - len(u2))]
    
    #            print u1
    #            print u2
    #            print len(args)
                i += 1
                if u1 != "":
                    fsc.write(", " + u1.strip())
                if u2 != "":
                    fsc.write(", " + u2.strip())
                    if i == len(args):
                        fsc.write(r");")

        fsc.write("\n}\n" + "#endif" + "\n\n")

    fsc.close()

    return

def _modifySyscallMax(sc_path):
    global syscall_num
    fcr = open(sc_path, 'r+')               # read syscall_tbl.c
    tblc = fcr.readlines()
    fcr.seek(0)
#    fcr.truncate()
#    print syscall_num
    for line in tblc:
        if(line.find(r"#define SYSCALL_MAX") == 0):
            line = r"#define SYSCALL_MAX %s" % (syscall_num + 1) + "\n"   
        fcr.write(line)
    fcr.close()

    return

def _removeSyscallData(sn_path):
    if os.path.exists(sn_path):
#        print sn_path
        os.remove(sn_path)                     # remove syscall_num

    return

def main():
    search_string = r"EXPORT_SYMBOL_K\((.*?)\,\s*?[\\|\s]\s*?(\S*?)\,\s*?[\\|\s]\s*?(\".*?\")\)$"
    copyright_path = r"./build/copyright"
    syscall_tblc_path = r"./kernel/syscall/syscall_tbl.c"
    syscall_tbl_path = r"./kernel/syscall/syscall_tbl.h"
    syscall_uapi_path = r"./framework/usyscall/syscall_uapi.c"
    syscall_data_path = r"./build/scripts/syscall_data"
    usyscall_incl_path = r"./framework/usyscall/syscall_uapi_include.h"
    global symbol_list
    global num

    starttime = time.time()

#--------------------------------------------------
    # remove syscall data, when want to store the syscall data for release version, annotations it
    _removeSyscallData(syscall_data_path)
#--------------------------------------------------

    # Search for each directory, find the symbol
    for rootdir in rootdirs:
        search_symbols(rootdir, search_string)

    # Remove duplicate element & Element sorting
    symbol_list=sorted(set(symbol_list),key=symbol_list.index)
    symbol_list.sort()

    logging.info("======================================")
    logging.info(" new symbol:")
    # Creat and write to syscall_tbl.h
    _writeSyscallHeader(copyright_path, syscall_tbl_path, syscall_data_path)
    logging.info("======================================")

    # Creat and write to syscall_uapi.c
    _writeSyscallUapi(copyright_path, syscall_uapi_path, syscall_data_path, usyscall_incl_path)

    #modify SYSCALL_MAX
    _modifySyscallMax(syscall_tblc_path)

    endtime = time.time()

    print "======================================"
    print (" create syscall file")
    print (" total: %s symbol find" % len(symbol_list))
    print (" total: %s file find" % num)
    print (" total time: %s s" % (endtime - starttime))
    print "======================================"

if __name__ == "__main__":
    main()


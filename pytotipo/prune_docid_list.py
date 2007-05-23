#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
gen_valid_docid_list

The docid list may contain entries for URLs thar were not effectively
downloaded. This script tries to find the subset of such URLs that where
downloaded.
"""

import sys
import os

def print_usage():
    print """prune_docid_list.py [store_dir] [docid_lst] [output]
    
    \tstore_dir\tWhere the downloaded files are
    \tdocid_lst\tThe list of docids
    \toutput\tWhere the pruned list will be saved.
    \t\t\tUse "-" to  redirect to stdout.
    """
    sys.exit()

def docidDownloaded(store_dir, docid):
    id_hex = "%08X" % docid
    id_path = "/".join([id_hex[0:2], id_hex[2:4], id_hex[4:6], id_hex[6:8]])
    filename = store_dir + "/" + id_path + "/" + "data.gz"
    try:
        os.stat(filename)
        return True
    except OSError:
        return False
    


def prune_docids(store_dir, input, output):
    n = 0
    for line in input:
        n += 1
        docid_str, url = line.split()
        docid = int(docid_str)
        # test
        if docidDownloaded(store_dir,docid):
            output.write(line)
        # stats
        if n % 1000 == 0:
            sys.stderr.write("%i entries read\n"% n)

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print_usage()
        sys.exit(1)
    else:
        store_dir, docids_filename, output_filename = sys.argv[1:]

        input = open(docids_filename, 'r')
        # output
        if output_filename == "-":
            output = sys.stdout
        else:
            output = open(output_filename, 'w')

        prune_docids(store_dir, input, output)
            
# vim:sts=4:ts=4:et:sw=4:ai:fo+=tcroq:fileencoding=utf-8:

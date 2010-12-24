#!/usr/bin/python

from optparse import OptionParser
import fnmatch
import tarfile
import StringIO
import re
import os
import sys
import rfc822

class TarFilterList(object):
    def __init__(self, filename):
        self.patterns = {}
        for filt in open(filename).readlines():
            f = filt.strip().split(None, 1)
            if len(f) == 1:
                [pat] = f
                cmd = None
            else:
                [pat, cmd] = f

            pat = pat.split(os.sep)
            self.patterns['t'] = "A"
            self.add_pattern(pat, self.patterns, cmd)

    def add_pattern(self, pat, patterns, cmd):
        if re.search(r'[\[\?\*]', pat[0]):
            if not '*' in patterns:
                patterns['*'] = []
            patterns['*'].append([os.sep.join(pat), cmd])
        else:
            if not pat[0] in patterns:
                patterns[pat[0]] = {}
            if len(pat) > 2:
                self.add_pattern(pat[1:], patterns[pat[0]], cmd)
            else:
                if not '*' in patterns[pat[0]]:
                    patterns[pat[0]]['*'] = []
                patterns[pat[0]]['*'].append([os.sep.join(pat[1:]), cmd])

    def match(self, name):
        name = name.split(os.sep)[1:]
        if len(name) == 0:
            return False
        return self._match(name, self.patterns)

    def _match(self, name, patterns):
        if len(name) > 1 and name[0] in patterns:
            cmd = self._match(name[1:], patterns[name[0]])
            if cmd != False:
                return cmd
        if '*' in patterns:
            for [pat, cmd] in patterns['*']:
                if fnmatch.fnmatch(name[0], pat) or fnmatch.fnmatch(os.sep.join(name), pat):
                    return cmd
        return False

def filter_tar(orig, new, filt):
    filt = TarFilterList(filt)
    tar = tarfile.open(orig, "r:*")
    new = tarfile.open(new, "w:bz2")

    while True:
        info = tar.next()
        if not info:
            break
        do_filt = filt.match(info.name)
        if do_filt == None:
            print >> sys.stderr, "Removing %s" % (info.name)
            continue

        if info.isfile():
            file = tar.extractfile(info)
            if do_filt:
                print >> sys.stderr, "Filtering %s" % (info.name)
                orig = file
                file = StringIO.StringIO()
                the_filt = lambda l: l
                if do_filt[0].isalpha():
                    f = do_filt.split(do_filt[1])
                    if f[0] == 's':
                        the_filt = lambda l: re.sub(f[1], f[2], l)
                else:
                    f = do_filt.split(do_filt[0])
                    if f[2] == 'd':
                        the_filt = lambda l: "" if re.search(f[1], l) else l
                file.writelines(map(the_filt, orig.readlines()))
                file.seek(0);
                info.size = len(file.buf)
            new.addfile(info, file)
        else:
            new.addfile(info)

    tar.close()
    new.close()

def get_package_name():
    return rfc822.Message(open("debian/control"))["Source"]

def main():
    parser = OptionParser()
    parser.add_option("-u", "--upstream-version", dest="upstream_version",
        help="define upstream version number to use when creating the file",
        metavar="VERSION")
    (options, args) = parser.parse_args()

    if not options.upstream_version:
        parser.error("Need an upstream version")
        return

    if len(args) < 1:
        parser.error("Too few arguments")
        return
    if len(args) > 1:
        parser.error("Too many arguments")
        return

    if os.path.islink(args[0]):
        orig = os.path.realpath(args[0])
        new_file = args[0]
    else:
        orig = args[0]
        new_file = get_package_name() + "_" + options.upstream_version + ".orig.tar.bz2"
        new_file = os.path.realpath(os.path.join(os.path.dirname(orig), new_file))
    print orig, new_file
    filter_tar(orig, new_file + ".new", "debian/source.filter")
    os.rename(new_file + ".new", new_file)

if __name__ == '__main__':
    main()

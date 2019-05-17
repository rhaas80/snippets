import sys
sys.path = sys.path[2:]
import os, io, builtins
import imp
import importlib
import marshal
import zipfile

class MyImporter:
  def __init__(self, zipfile, archivename):
    self._zipfile = zipfile
    self._namelist = self._zipfile.namelist()
    self._archivename = archivename
    self._fallback = importlib.machinery.FileFinder(os.path.dirname(self._archivename))
  def find_module(self, fullname, path=None):
    zippath = fullname.replace('.', '/')
    for prefix in ("", "site-packages"):
      for postfix in ("", "__init__"):
        p = os.path.join(prefix, zippath, postfix).rstrip('/') + ".py"
        if p in self._namelist:
          return self
    return None
  # copied more or less verbatim from https://www.python.org/dev/peps/pep-0302/
  def load_module(self, fullname):
    zippath = fullname.replace('.', '/')
    found = False
    for prefix in ("", "site-packages/"):
      for (ispkg, postfix) in enumerate(("","__init__")):
        p = os.path.join(prefix, zippath, postfix).rstrip('/') + ".py"
        if p in self._namelist:
          zippath = p
          found = True
          break
      if found:
        break
    sourcepath = os.path.join(self._archivename, zippath)
    (dirname, basename) = os.path.split(zippath)
    cachefile = os.path.join(dirname, "__pycache__", basename).replace(".py", ".cpython-36.pyc")
    isbytecode = cachefile in self._namelist
    if isbytecode:
      # copied from https://github.com/python/cpython/blob/3.6/Lib/importlib/_bootstrap_external.py
      # the skipe size is python VERSION dependend (8 for python2, 12 for python3, 16 for python3.7)
      # TODO: this is hardly portable but I don't know how to read stuff in otherwise
      bytecode =  self._zipfile.open(cachefile).read()[12:]
      code = marshal.loads(bytecode)
    else:
      source = self._zipfile.open(zippath).read()
      code = compile(source, sourcepath, 'exec')
    mod = sys.modules.setdefault(fullname, imp.new_module(fullname))
    # do not use sourcepath so that data files are found
    mod.__file__ = os.path.join(os.path.dirname(self._archivename), zippath)
    mod.__loader__ = self
    if ispkg:
      mod.__path__ = []
      mod.__package__ = fullname
    else:
      mod.__package__ = fullname.rpartition('.')[0]
    exec(code, mod.__dict__)
    return mod

__archivename = "/mnt/c/scratch/staff/rhaas/tmp/Python-3.6.6-install/lib/python3.6/modules.zip"
__abs_archive_dirname = os.path.abspath(os.path.dirname(__archivename))
__zf = zipfile.ZipFile(__archivename)
sys.meta_path.insert(2, MyImporter(__zf, __archivename))
#sys.meta_path.insert(2, MyImporter("/mnt/c/scratch/staff/rhaas/tmp/Python-3.6.6-install/lib/python3.6/site-packages/modules.zip"))

__FILES = __zf.namelist()
__builtin_open = builtins.open
__os_path_exists = os.path.exists
__os_path_getsize = os.path.getsize
def __is_in_FILES(filename):
  l = len(__abs_archive_dirname)
  return filename[:l] == __abs_archive_dirname and filename[l+1:] in __FILES
def __comb_open(filename, *args, **kwargs):
  if __is_in_FILES(filename):
    l = len(__abs_archive_dirname)
    return io.BytesIO(__zf.open(filename[l+1:]).read())
  else:
    return __builtin_open(filename, *args, **kwargs)
def __comb_exists(filename, *args, **kwargs):
  if __is_in_FILES(filename):
    return True
  else:
    return __os_path_exists(filename, *args, **kwargs)
def __comb_getsize(filename, *args, **kwargs):
  if filename in __FILES:
    return __zf.getinfo(filename).file_size
  else:
    return __os_path_getsize(filename, *args, **kwargs)
builtins.open = __comb_open
os.path.exists = __comb_exists
os.path.getsize = __comb_getsize

# python's webrowser.py TESTs for very many browsers in $PATH
os.environ["PATH"] = ""

open("strace.log")


__requires__ = 'globus-cli==1.6.3'
import re
import sys
from pkg_resources import load_entry_point

#import cProfile, pstats, atexit

if __name__ == '__main__':
    sys.argv[0] = re.sub(r'(-script\.pyw?|\.exe)?$', '', sys.argv[0])
#    pr = cProfile.Profile()
#    def dumpstats():
#      pr.disable()
#      stats = pstats.Stats(pr)
#      #stats.sort_stats('cumulative').print_stats()
#      stats.dump_stats('globus.profile')
#    atexit.register(dumpstats)
#    pr.enable()
    sys.exit(load_entry_point('globus-cli==1.6.3', 'console_scripts', 'globus')())

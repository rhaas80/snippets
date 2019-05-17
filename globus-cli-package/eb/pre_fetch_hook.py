# this uses curl to download source files since urllib2 fails with tsl due to
# an old ssh being used

import os

def pre_fetch_hook(self, *args, **kwargs):
  """Use curl cmmand to download files"""
  name = self.cfg['name']
  for s in self.cfg['sources']:
    for u in self.cfg['source_urls']:
      print "attempting %s/%s" % (u,s)
      cmd = 'curl "%s/%s" -o %s/.local/easybuild/sources/g/%s/%s' % \
        (u,s, os.environ['HOME'], name, s)
      if os.system(cmd) == 0:
        break

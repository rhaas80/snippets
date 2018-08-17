#!/usr/bin/python

nranks = 12
ranks_per_group = 3

ngroups = nranks / ranks_per_group

for i in range(nranks):
  print i, (i%ngroups) * ranks_per_group + i / ngroups

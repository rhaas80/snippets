#!/bin/bash

while true ; do
  `nc -d localhost 9922 | sed 's/\r$//'`
done

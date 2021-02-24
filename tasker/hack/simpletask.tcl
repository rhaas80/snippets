#!/usr/bin/env tclsh

proc Server {channel clientaddr clientport} {
  puts $channel [gets stdin]
  close $channel
}

socket -server Server 9922
vwait forever

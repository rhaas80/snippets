#!/usr/bin/env tclsh

proc Server {channel clientaddr clientport} {
    global tasks taskid workers

    # read worker id from socket
    set workerid [gets $channel]

    # if I still have tasks, assign one, otherwise tell worker to quit
    if {$taskid < [llength $tasks]} then {
      puts $channel "$taskid [lindex $tasks $taskid]"
      incr taskid
      # once I see a worker I remmeber it to keep track of when I have told all
      # workers to quit
      set workers($workerid) 1
    } else {
      puts $channel ""
      # forget about worker, it will quit soon
      if [info exists workers($workerid)] then {
        unset workers($workerid)
      }
    }

    close $channel

    # all done?
    if {[array size workers] == 0} then {
      exit 0
    }
}

# first argument is list of tasks
set tasksfile [lindex $::argv 0]
# second argument is server node (rank 0) name
set server [lindex $::argv 1]
# third argument is port to use
set port [lindex $::argv 2]

# MPI rank, rank 0 will be the manager
set myid $::env(ALPS_APP_PE)

# how long to try to connect to server, must be long enough to startup to
# finish
set TIMEOUT 10

if {$myid == 0} then {
  # manager
  set fh [open $tasksfile "r"]
  # split returns an empty string after the final '\n' in the file
  set tasks [lrange [split [read $fh] "\n"] 0 end-1]
  set taskid 0
  close $fh

  socket -server Server $port
  puts "Server accepting connections on $server $port"
  vwait forever
} else {
  # client (keep this in global space so that after .. vwait works)
  while {1} {
    # connect to server
    for {set i 0} {$i < $TIMEOUT} {incr i} {
      if {[catch {socket $server $port} sock]} then {
        puts "Waiting for $server $port to accept connections"
        after 1000 set end 1
        vwait end
      } else {
        break
      }
    }
    if {$i == $TIMEOUT} {
      puts stderr "Timeout connecting to $server $port"
      exit 1
    }

    # ask for work by sending my worker id
    puts $sock $myid
    flush $sock

    # get work item
    set line [gets $sock]
    close $sock
    set cmdid [lindex $line 0]
    set cmd [lrange $line 1 end]

    # are we done?
    if {$cmd eq ""} then {
      exit 0
    }

    # run command and store result
    set logfile "job$cmdid.log"
    eval exec -- $cmd ">&" $logfile
  }  
}

# Otto

Base Otto package - server, command line utilities...

## Setting up Otto

1. Create a directory to store the Otto configuration file, database, and logs.  To support 1000 jobs the Otto database is pretty small so you can put it somewhere in your home directory if you want.

        -rwxr-xr-x 1 otto otto 2.2M Jan  1 00:01 otto.db  


2. Set up environment variables pointing to files or directories Otto needs:

        Example:  
        export OTTOCFG=$HOME/.otto/otto.conf
        export OTTOLOG=$HOME/.otto/log
        export OTTODB=$HOME/.otto/otto.db

   See etc/setup.sh.


4. Pick a port for the ottosysd daemon to listen on.  Low port numbers are typically preassigned to well-known services so pick a higher number.  You can use the netstat command to show ports in use by other programs:

        Example:  
        $ netstat -an -f inet  
        Active Internet connections (including servers)  
        Proto Recv-Q Send-Q  Local Address          Foreign Address        (state)  
        tcp        0      0  10.40.209.26.21        10.143.210.152.54404    FIN_WAIT_2  
        tcp        0      0  127.0.0.1.12345        *.*                     LISTEN  
        tcp        0      0  *.53520                *.*                     LISTEN  
        tcp        0      0  127.0.0.1.53861        127.0.0.1.50155         ESTABLISHED  

    In the above example the fifth number (or the number after the asterisk) in the Local Address column is a port currently in use (21, 12345, 53520, 53861) suggesting you should choose a port not shown.


5. Save the port number in your $OTTOCFG file in a single line of the format "ottosysd_port \[port number\]".  

        Example:  
        $ cat $OTTOCFG  
        ottosysd_port 12345  

    If you pick a port already in use ottosysd will complain.  A line will be written to the log file such as:

        14:04:58 MAJR: bind failed.


6. Save values for additional optional parameters in your $OTTOCFG file.

        Example:  
        #
        # username of privileged user allowed to start and stop the deamon from the command line
        #
        user           otto

        #
        # up to max signed short (32767)
        #
        ottodb_maxjobs 1500

        #
        # daemon listening port
        #
        ottosysd_port  12345

        #
        # archive logs on restart
        #
        archivelog     true

        #
        # verbose response on job operations
        #
        verbose        true

        #
        # show the time a process has been running "so far" in the Runtime column on the summary report
        #
        show_sofar     true

        #
        # Produce additional debug output in the log
        #
        debug          true

        #
        # identify users issuing commands using getpw calls instead of using $LOGNAME/$USER
        #
        use_getpw      false

        #
        # httpd is best used behind a proxy like caddy or nginx
        #
        enable_httpd   true
        httpd_port     12346
        base_url       /otto

        #
        # variables to insert into the environment of child jobs
        #
        envvar         OTTOBIN=/home/otto/Otto/bin
        

7. Compile and install the programs (make install in src).  Make sure ottosysd, ottocmd, ottoexp, ottoimp, ottorep, ottotr, ottostart, ottostop, and ottoping are in your $PATH and are executable.

        Example:  
        $ ls -logh
        total 1.8M
        -rwxr-xr-x. 1 248K Jun  5 00:50 ottocmd
        -rwxr-xr-x. 1 270K Jun  5 00:50 ottoexp
        -rwxr-xr-x. 1 327K Jun  5 00:50 ottoimp
        -rwxrwx--x. 1  324 May  4 10:28 ottoping
        -rwxr-xr-x. 1 233K Jun  5 00:50 ottorep
        -rwxrwx--x. 1 2.3K Jun  5 00:44 ottostart
        -rwxrwx--x. 1  581 May  4 10:28 ottostop
        -rwxr-xr-x. 1 362K Jun  5 00:50 ottosysd
        -rwxr-xr-x. 1 325K Jun  5 00:50 ottotr

## Using Otto

1. Start and stop Otto using ottostart and ottostop.  ottostart will run your ottosysd instance in the background using nohup.

2. Create JIL files and use Otto as if it were autosys.  Remember, Otto doesn't support some autosys functionality.

    What it supports:  
    
        eventor program equivalent     Daemon runs in shell session or in background (nohupped)  
        jil command equivalent         Reads a subset of autosys JIL plus Otto-specific extensions.
                                       Common keywords between Otto and autosys JIL:
                                       auto_hold, box_name, command, condition,
                                       date_conditions, days_of_week, delete_box, delete_job, description,
                                       insert_job, job_type, start_mins, start_times, update_job

                                       Otto specific keywords:
                                       auto_noexec, environment, finish, loop, new_name, start
        autorep command equivalent     Reports in (mostly) autosys formats  
        sendevent command equivalent   Common events between Otto and autosys:
                                       CHANGE_STATUS, FORCE_STARTJOB, JOB_OFF_HOLD, JOB_OFF_NOEXEC,
                                       JOB_ON_HOLD, JOB_ON_NOEXEC, KILLJOB, SEND_SIGNAL, STARTJOB,
                                       STOP_DEMON

                                       Otto specific events:
                                       BREAK_LOOP, JOB_OFF_AUTOHOLD, JOB_OFF_AUTONOEXEC, JOB_ON_AUTOHOLD,
                                       JOB_ON_AUTONOEXEC, MOVE_JOB_BOTTOM, MOVE_JOB_DOWN, MOVE_JOB_TOP,
                                       MOVE_JOB_UP, RESET_JOB, SET_LOOP, DEBUG_OFF, DEBUG_ON, PAUSE_DAEMON,
                                       PING, REFRESH, RESUME_DAEMON, VERIFY_DB
    
                                       CHANGE_STATUS supports FAILURE, INACTIVE, RUNNING, SUCCESS, TERMINATED
        some conditions                Supports DONE, FAILURE, NOTRUNNING, SUCCESS, and TEMINATED job statuses  
                                       and logical and (&) and or (|) with condition grouping  


    What it doesn't support:  

        permissions                    Otto is meant to be a local "personal" job scheduler  
        file watchers                  These can be implemented as job scripts and run like normal commands  
        calendars                      These can be implemented as a job script or cron job  
        multiple machines              Otto is meant to be a local "personal" job scheduler not an  
                                       enterprise wide solution  
        some conditions                Does not support lookback, exit status, global variables  


## Notable differences from autosys

1. ottorep -s presents the run time in the last column instead of the number of runs and exit code
2. ottorep -s presents 'autohold', 'onnoexec', or 'exprfail' in the runtime column to indicate a job
   is in AUTO_HOLD or JOB_ON_NOEXEC state or has a condition parsing error respectively

        Example:  
        $ ottorep -s -j %  
        
        Job Name                                     Last Start           Last End             ST Runtime  
        ____________________________________________ ____________________ ____________________ __ ________  
        CNV_OTTO_BOX                                 -----                -----                IN --------  
         CNV_OTTO_JOB1                               -----                -----                IN onnoexec  
         CNV_OTTO_JOB2                               -----                -----                IN autohold  
         CNV_OTTO_JOB3                               -----                -----                IN --------  
         CNV_OTTO_JOB4                               -----                -----                IN exprfail  

3. ottorep -d presents details in an otto specific format

4. ottoexp -F JIL (instead of ottorep -q) doesn't show unsupported machine, owner, permission, or alarm_if_fail attributes

        Example:  
        /* ----------------- CNV_OTTO_BOX ----------------- */  
        
        insert_job:    CNV_OTTO_BOX  
        job_type:      b  
        description:   "OTTO test box"  
    
         /* ----------------- CNV_OTTO_JOB1 ----------------- */  
         
         insert_job:    CNV_OTTO_JOB1  
         job_type:      c  
         box_name:      CNV_OTTO_BOX  
         command:       set > ~/CNV_OTTO_JOB1.log; sleep 20  
         description:   "OTTO test job 1"  
         auto_hold:     1  

5. Otto's delete_job operation does not work on boxes.  autosys will allow you to delete a box
   using the delete_job command but this action retains the box's contents, promoting them to
   the level of the box that contained them.

6. Otto provides multiple non-autosys supported keywords and events listed above.

7. ottorep -p presents the PID of the job for use by monitoring programs.

8. Unlike autosys, Otto does NOT automatically run a job or box when its conditions are satisfied UNLESS:   
    a. you STARTJOB the dependent job/box first, OR   
    b. the preceding box/job is in the SAME parent box as the dependent job/box.  



## What?  No JOB_ON_ICE?

I never liked it.  I got into the bad habit of using it early in my experience with autosys
but in writing this utility I decided JOB_ON_NOEXEC is a much better (safer) method of not
running jobs.

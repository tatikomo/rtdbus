#include <glog/logging.h>

#include "mdp_common.h"
#include "mdp_broker.hpp"
#include "proxy.hpp"

using namespace mdp;

/*  ---------------------------------------------------------------------[<]-
    Function: process_server

    Synopsis: Converts the process from an interactive foreground process
    into a background process.  Depending on the system either the existing
    process becomes a background process, or a new process is started as
    a background process and the existing process terminates.

    Requires the original command-line arguments as passed to the main()
    function.  If a new process is started, any arguments that match any
    of the values in the (NULL terminated) 'sswitch' array will be omitted
    from the arguments to the new process.  You should specify the
    command-line switch(es) your main program uses to run as a background
    service.   If it returns, returns 0 if okay, and returns -1 if there
    was an error.

    The precise effect of this function depends on the system.  On UNIX,
    does this:
    <LIST>
    Switches the process to run as a background job, under init;
    closes all open files;
    moves to a safe, known working directory, if required;
    sets the umask for new files;
    opens stdin, stdout, and stderr to the null device;
    enforces exclusive execution through a lock file, if required;
    logs the process id in the lock file;
    ignores the hangup unwanted signals.
    </LIST>

    On other systems does nothing except return 0.
    ---------------------------------------------------------------------[>]-*/

int
process_server (
    /*const char *workdir,*/                /*  Where server runs, or NULL/""    */
    /*const char *lockfile,*/               /*  For exclusive execution          */
    int   argc,                         /*  Original command-line arguments  */
    char *argv [],                      /*  Original command-line arguments  */
    const char *sswitch [])             /*  Filter these options from argv   */
{
#if (defined (__UNIX__))
    int
        fork_result,
        file_handle;
    char
        pid_buffer [10];
    struct flock
        lock_file;                      /*  flock() argument block           */
#endif
    int
        argi = 0,                       /*  Input arguments iterator         */
        argo = 0;                       /*  Output arguments iterator        */
    char
        **newargv = NULL;               /*  Array of new arguments           */

    newargv = new char* [argc * sizeof (char *)];
    if (newargv == NULL)
        return (-1);

    /*  Copy the arguments across, skipping any in sswitch                   */
    for (argi = argo = 0; argi < argc; argi++)
    {
        bool
            copy_argument = true;
        int
            i = 0;

        for (i = 0; sswitch != NULL && sswitch [i] != NULL; i++)
        {
            if (strcmp (argv [argi], sswitch [i]) == 0)
                copy_argument = false;
        }
        if (copy_argument)
            newargv [argo++] = argv [argi];
    }

    /*  Terminate the new argument array                                     */
    newargv [argo] = NULL;

#if (defined (__UNIX__))
    /*  We recreate our process as a child of init.  The process continues   */
    /*  to exit in the background.  UNIX calls wait() for all children of    */
    /*  the init process, so the server will exit cleanly.                   */

    fork_result = fork ();
    if (fork_result < 0)                /*  < 0 is an error                  */
        return (-1);                    /*  Could not fork                   */
    else
    if (fork_result > 0)                /*  > 0 is the parent process        */
        exit (EXIT_SUCCESS);            /*  End parent process               */

    /*  We close all open file descriptors that may have been inherited      */
    /*  from the parent process.  This is to reduce the resources we use.    */

    for (file_handle = FILEHANDLE_MAX - 1; file_handle >= 0; file_handle--)
        close (file_handle);            /*  Ignore errors                    */

    /*  We move to a safe and known directory, which is supplied as an       */
    /*  argument to this function (or not, if workdir is NULL or empty).     */

    if (workdir && strused (workdir)) {
        ASSERT (chdir (workdir));
    }
    /*  We set the umask so that new files are given mode 750 octal          */

    umask (027);                        /*  Complement of 0750               */

    /*  We set standard input and output to the null device so that any      */
    /*  functions that assume that these files are open can still work.      */

    file_handle = open ("/dev/null", O_RDWR);    /*  stdin = handle 0        */
    ASSERT (dup (file_handle));                  /*  stdout = handle 1       */
    ASSERT (dup (file_handle));                  /*  stderr = handle 2       */

    /*  We enforce a lock on the lockfile, if specified, so that only one    */
    /*  copy of the server can run at once.  We return -1 if the lock fails. */
    /*  This locking code might be better isolated into a separate package,  */
    /*  since it is not very portable between unices.                        */

    if (lockfile && strused (lockfile))
    {
        file_handle = open (lockfile, O_RDWR | O_CREAT, 0640);
        if (file_handle < 0)
            return (-1);                /*  We could not open lock file      */
        else
        {
            lock_file.l_type = F_WRLCK;
            if (fcntl (file_handle, F_SETLK, &lock_file))
                return (-1);            /*  We could not obtain a lock       */
        }
        /*  We record the server's process id in the lock file               */
        snprintf (pid_buffer, sizeof (pid_buffer), "%6d\n", getpid ());
        ASSERT (write (file_handle, pid_buffer, strlen (pid_buffer)));
    }

    /*  We ignore any hangup signal from the controlling TTY                 */
    signal (SIGHUP, SIG_IGN);
    return (0);                         /*  Initialisation completed ok      */
#else
    return (0);                         /*  Nothing to do on this system     */
#endif
}

//  Finally here is the main task. We create a new broker instance and
//  then processes messages on the broker socket.
int main (int argc, char *argv [])
{
    bool verbose = (argc > 1 && (0 == strncmp(argv [1], "-v", 3)));
    std::string sender;
    std::string empty;
    std::string header;
    Broker *broker = NULL;

    ::google::InstallFailureSignalHandler();
    ::google::InitGoogleLogging(argv[0]);

    try
    {
      s_version_assert (3, 2);
      s_catch_signals ();
      broker = new Broker(verbose);
      /*
       * NB: "tcp://lo:5555" использует локальный интерфейс, 
       * что удобно для мониторинга wireshark-ом.
       */
      broker->bind ("tcp://*:5555");

      broker->start_brokering();
    }
    catch (zmq::error_t err)
    {
        LOG(ERROR) << err.what();;
    }

    if (s_interrupted)
    {
        LOG(INFO) << "interrupt received, shutting down...";
    }

    delete broker;

//    ::google::protobuf::ShutdownProtobufLibrary();
    ::google::ShutdownGoogleLogging();
    return 0;
}


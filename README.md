C programming including:

* System calls
* Low level concurrency
* Multiple processes
* Fork-Join
* Thread scheduling
* Syncronization of system (POSIX) threads.
  * Mutex locks
  * Semaphores

# Assignment 1

Task 1: Shell Interface

    Design a C program for a shell interface.
    The shell prompts user for commands (e.g., osh> cat prog.c).
    Execute commands in separate child processes.
    If a command ends with &, it should run in the background.
    Use fork() to create child processes and execvp() to execute commands.

Part I— Creating a Child Process

    Parse user's command into separate tokens.
    Example: "ps -ael" -> args[0]="ps", args[1]="-ael", args[2]=NULL.
    Use execvp(args[0], args) for execution.

Part II—Creating a History Feature

    Allow users to access up to 10 of the most recent commands.
    history command to list command history.
    !! to execute the most recent command.
    !N to execute the Nth command in history.

Task 2: String Search in a File using 4 Child Processes

    Accept a filename and a search string as command-line arguments.
    Divide the file into 4 parts and assign each to a child process.
    Display the starting row and column if the string is found.

Task 3: Factorial Calculation

    Use fork() to create a child process.
    Child process prints factorial series.
    Sum up the series and display the result.

# Assignment 2

Task 1: TA Office Hour Simulation

    Use POSIX threads, mutex locks, and semaphores.
    TA's office has one desk and limited hallway chairs.
    TA naps when no students, wakes up when a student arrives.
    Students wait in chairs if TA is busy, or come back later if chairs are full.
    Simulate TA-student interactions using random sleep durations.

Task 2: Chemical Reaction Simulation

    Simulate synchronization between two H atoms and one O atom to make water.
    Assume that the semaphore implementation enforces FIFO wakeup policy.
    hReady() for H atom readiness.
    oReady() for O atom readiness.
    Ensure no starvation or busy waiting.
    When conditions met, call makeWater().

Task 3: Redmond Riverboat Problem

    Simulate rowboat crossing with Linux hackers and Microsoft serfs. Each person is a thread.
    Boat holds exactly four people.
    Avoid unsafe combinations: 1 hacker with 3 serfs, or 1 serf with 3 hackers.
    On boarding, threads call board().
    One thread should call rowboat() to indicate rowing.

Task 4: SJF and FCFS CPU Scheduling Algorithms

    Given processes and their burst times.
    Compute average waiting and turnaround times using SJF and FCFS algorithms
    Definitions:
        Turnaround Time = Completion - Submission.
        Waiting Time = Turnaround Time - Burst Time.

This implements MapReduce debugger and performance tracer.
The application is an example from the paper "Nondeterminism in MapReduce Considered Harmful? An Empirical Study on Non-commutative Aggregators in MapReduce Programs". It reads in a log file of Ad clicks and revenues and sum up the records by ViewId, which is the first column in each record. The input file is a text log file. Each line is a record and have five fields seperated with a tab. The five fields are ViewId, State, AdId, Clicks and Revenue. 

## Non-determinism and the reducer debugger: 
This program assumes that records with the same ViewId will have the same State and AdId. Unfortunately, as the paper points out, this assumption does not always hold. And since the reducer is non-commutative, it yields different results when the order of the inputs changes, which usually occurs due to indeterministic execution of multi-thread or multi-process programs. With the reducer debugger, we can log the input of each reducer and then replay the same sequence, leading to deterministic behavior, which will help programmer debug the program.

## Performance tracer:
The program tracer can log the time in which each event occurs in the MapReduce framework. Currently we log master thread events, worker events, and map/reduce/merge events.

## How to use debugger and tracer:
Logging is enabled through setting the LOG environment variable:
	```$LOG=1 ./adrecord test2.txt```
	With this command you should get a reducer.trace file in the same directory, which records the reducer inputs. Since there is nondeterminism in the program, it will sometimes give you different outputs if you run it multiple times.

Replaying is enabled through setting the REPLAY environment variable:
	```$REPLAY=1 ./adrecord test2.txt```
	This command will execute the same MapReduce program and load the reduce input from reducer.trace. As a result, it should always give you the same output as the round in which you do logging.

Performance tracing is enabled through setting the PTRACE environment variable:
	```PTRACE=1 ./adrecord test2.txt```
	This command will execute the program and record performance traces in performance.trace.

Introduction
============

This directory contains the source code used to evaluate existing cache attacks as well as XLATE + PROBE and XLATE + ABORT. XLATE attacks are a new family of indirect cache attacks that leverage the MMU to indirectly manipulate the cache. XLATE + PROBE and XLATE + ABORT are inspired by PRIME + PROBE and PRIME + ABORT respectively. XLATE attacks require eviction sets to monitor the target page of the victim. The algorithm to build these is similar to the algorithm outlined in Spy in the Sandbox and can be found in `source/eviction.c`. More information about the project can be found at: https://www.vusec.net/projects/xlate/

The `histogram` proram is used to evaluate the timing difference between cache hits and misses, or more specifically to differentiate between the target page being used or not being used. It is useful to use this program to tune the timings used in the other programs. The `tools/` directory also contains some useful Python scripts to visualize these, such as `tools/plot_histogram.py`.

OpenSSL
=======

For the programs targetting the AES T-table implementation of OpenSSL, you will need OpenSSL with this implementation enabled. Download openssl-1.0.1e and build it as follows:

```
./config shared no-hw no-asm
make
```

Build the tools as follows:

```
make
```

To perform an attack against the AES T-tables in OpenSSL using FLUSH + RELOAD, run the following:

```
LD_LIBRARY_PATH="./openssl-1.0.1e:$LD_LIBRARY_PATH" ./obj/aes-fr openssl-1.0.1e/libcrypto.so
```

Other attacks that are available are:

 * FLUSH + FLUSH (aes-ff),
 * PRIME + PROBE (aes-pp),
 * XLATE + PROBE (aes-xp).
 * PRIME + ABORT (aes-pa).
 * XLATE + ABORT (aes-xa).

For PRIME + PROBE, PRIME + ABORT, XLATE + PROBE and XLATE + ABORT, make sure the transparent hugepages option is set to madvise or never.

There are various tools to visualize the results of the OpenSSL AES T-table attack in `tools/`. The most interesting one to start out with would probably be `tools/plot_single_aes.py`.

Covert channels
===============

To evualate the bandwidth and error rate of the various cache attacks, we implemented a covert channel framework. This framework implements a basic synchronization protocol and uses cache attacks to transfer zeroes and ones between two programs over the cache. The source code of the program can be found in `source/xfer.c`.

To support FLUSH + RELOAD and FLUSH + FLUSH, the `xfer` program uses shared memory. This shared memory is also used to be able to build the appropriate eviction sets, as otherwise the cache jamming agreement protocol (CJAG) would be required. It is recommended to use the hugetlbfs and set up a 2M page for this. The path to this 2M page can be passed as the first argument. The second argument is the cache attack to use for receiving data. The sender will always use FLUSH + RELOAD, since it is the most reliable one for sending the acknowledgements. The third argument indicates whether the program runs as the receiver (recv) or the sender (send).

See https://wiki.debian.org/Hugepages for more information on how to set up hugetlbfs. The 2M page can be allocated by using the following command:

```
truncate -s 2097152 /hugepages/foo
```

Once hugepages are set up, first run the receiver:

```
./obj/xfer /hugepages/foo flush-reload recv
```

Next run the sender:

```
./obj/xfer /hugepages/foo flush-reload send
```

And press enter to resume the receiver. If everything went well, the receiver should be receiving bursts of messages over the cache from the sender. For PRIME + PROBE, PRIME + ABORT, XLATE + PROBE, XLATE + ABORT, the eviction sets have to be built first, give this some time.

In addition to `xfer`, there is also a `count` program used to count the bandwidth and the error rate.

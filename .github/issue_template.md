### What version of hwloc are you using?

* Run `which lstopo`to find out where your tools come from
* Run `lstopo --version` to find their hwloc version
* If hwloc comes from a RPM package (RHEL, CentOS, Fedora, etc.), run `rpm -qa '*hwloc*'`
* If hwloc comes from a DEB package (Debian, Ubuntu, Mint, etc.), run `dpkg -l '*hwloc*'`
* If hwloc was built from GIT, report the GIT hash (or at least the branch and date you checked out)
* If the hwloc error occured inside a non-hwloc process (e.g. MPI, SLURM, etc.), report the version of that software. Also try things like `ldd` on the program to find out which hwloc it is using.

### Which operating system and hardware are you running on?

* On Unix, run `uname -a` so that we know which operating system, distribution, and kernel version you are using.
* Post the output of `lstopo -` if it works
* Describe the machine and the processors it contains, as well as memory and/or peripherals if they matter to your issue

### Details of the problem

* What happened?
* How did you start your process?
* How did it fail? Crash? Unexpected result?
* If crashed, can you get a `gdb` backtrace, etc?

### Additional information

If your issue consists in a wrong topology detection, we'll also need this for debugging remotely:
* On Linux, run `hwloc-gather-topology myhost` and post the `myhost.*` files that it will generate (so that we may debug remotely).
* On MacOS, `sysctl hw` and `sysctl machdep.cpu`
* On Solaris, `kstat cpu_info` and `lgrp_info -a`
* On BSD, `sysctl hw`
* On BSD x86 platforms, if using hwloc >= 2.0, also run `hwloc-gather-cpuid` and post an archive of the `cpuid` that was generated.
* On Windows, `coreinfo -cgnlsm`

**Don't post these files on Github or on public mailing list if your machine is a top-secret prototype!**

Note that upgrading your operating system (e.g. Linux kernel) and platform firmwares (e.g. BIOS) might help solving issues about wrong topology detection.

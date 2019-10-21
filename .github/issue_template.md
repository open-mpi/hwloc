### What version of hwloc are you using?

* Run `which lstopo` to ensure that you are running the desired hwloc installation
* Run `lstopo --version` to find their hwloc version
  * If hwloc comes from a RPM package (RHEL, CentOS, Fedora, etc.), run `rpm -qa '*hwloc*'`
  * If hwloc comes from a DEB package (Debian, Ubuntu, Mint, etc.), run `dpkg -l '*hwloc*'`
  * If hwloc was built from a Git clone, report the Git commit hash from `git show`
* If the hwloc error occured inside a non-hwloc process (e.g., MPI, SLURM, etc.), report the version of that software. Also try things like `ldd` on the program to find out which hwloc installation that software is using.

### Which operating system and hardware are you running on?

* On Unix-like systems, run `uname -a` so that we know which operating system, distribution, and kernel version you are using.
* Post the output of `lstopo -` if it works
* Describe the machine and the processors it contains, as well as any memory and/or peripherals that matter to your issue

### Details of the problem

* What happened?
* How did you start your process?
* How did it fail? Crash? Unexpected result?
* If it crashed, please include a `gdb` backtrace if possible.
  * Passing `CFLAGS='-g -O0'` on the configure line may make the backtrace more useful.
  * Passing `--enable-debug` to the configure line should also enable lots of debug messages before the crash.

### Additional information

If your issue consists in a wrong topology detection, we also need the following for debugging remotely:

* On Linux, run `hwloc-gather-topology myhost` and post the `myhost.*` files that it will generate. Note that this tool may be slow on large nodes or when I/O is enabled.
* On Solaris, `kstat -C cpu_info`, `lgrpinfo -a` and `psrinfo`
* On MacOS and BSD, `sysctl hw` and `sysctl machdep`
* On BSD x86 platforms, if using hwloc >= 2.0, also run `hwloc-gather-cpuid` and post an archive of the `cpuid` that was generated.
* On Windows, `coreinfo -cgnlsm`

:warning: **Do not post this information on Github or on public mailing list if your machine is a top-secret prototype!** :warning:

You may need to archive the output from the above commands into a `.zip` or `.tar.gz` (not `.bz2`!) file before Github will allow you to drag-n-drop the file into the issue to attach it.

Note that upgrading your operating system (e.g., Linux kernel) and platform firmwares (e.g., BIOS) might help solving issues about wrong topology detection.
Some known issues are listed at https://github.com/open-mpi/hwloc/wiki/Linux-kernel-bugs

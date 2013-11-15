#!/bin/sh

#####
#
# Configuration options
#
#####

# e-mail address to send results to
results_addr=hwloc-devel@open-mpi.org

# git repository URL
code_uri=https://github.com/open-mpi/hwloc.git
raw_uri=https://raw.github.com/open-mpi/hwloc

# where to put built tarballs
outputroot=/l/osl/www/www.open-mpi.org/software/hwloc/nightly

# where to find the build script
script_uri=contrib/nightly/make_snapshot_tarball

# The tarballs to make
if [ $# -eq 0 ] ; then
    # Branches v1.6 and earlier were not updated to build nightly
    # snapshots from git, so only check v1.7 and later
    branches="master v1.7"
else
    branches=$@
fi

# Build root - scratch space
build_root=/home/mpiteam/hwloc/nightly-tarball-build-root

export PATH=$HOME/local/bin:$PATH
export LD_LIBRARY_PATH=$HOME/local/lib:$LD_LIBRARY_PATH

#####
#
# Actually do stuff
#
#####

# load the modules configuration
. /etc/profile.d/modules.sh
module use ~/modules

# get our nightly build script
mkdir -p $build_root
cd $build_root

# Loop making them
for branch in $branches; do
    # Form a URL-specific script name
    script=$branch-`basename $script_uri`

    wget --quiet --no-check-certificate --tries=10 $raw_uri/$branch/$script_uri -O $script
    if test ! $? -eq 0 ; then
        echo "wget of hwloc nightly tarball create script failed."
        if test -f $script ; then
            echo "Using older version of $script for this run."
        else
            echo "No build script available.  Aborting."
            exit 1
        fi
    fi
    chmod +x $script

    module load "autotools/hwloc-$branch"
    module load "tex-live/hwloc-$branch"

    ./$script \
        $build_root/$branch \
        $results_addr \
        $outputroot/$branch \
        $code_uri \
        $branch \
        >/dev/null 2>&1

    module unload autotools tex-live
done

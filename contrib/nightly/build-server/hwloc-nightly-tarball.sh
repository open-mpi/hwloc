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
    branches="master v1.7 v1.8 v1.9 v1.10 v1.11"
else
    branches=$@
fi

# Build root - scratch space
build_root=/home/mpiteam/hwloc/nightly-tarball-build-root

# Coverity stuff
coverity_token=`cat $HOME/coverity/hwloc-token.txt`

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

pending_coverity=$build_root/tarballs-to-run-through-coverity.txt
rm -f $pending_coverity
touch $pending_coverity

# Loop making them
for branch in $branches; do
    # Get the last tarball version that was made
    prev_snapshot=`cat $outputroot/$branch/latest_snapshot.txt`

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

    # Did the script generate a new tarball?  If so, save it so that we can
    # spawn the coverity checker on it afterwards.  Only for this for the
    # master (for now).
    latest_snapshot=`cat $outputroot/$branch/latest_snapshot.txt`
    if test "$prev_snapshot" != "$latest_snapshot" && \
        test "$branch" = "master"; then
        echo "$outputroot/$branch/hwloc-$latest_snapshot.tar.bz2" >> $pending_coverity
    fi

    module unload autotools tex-live
done

# If we had any new snapshots to send to coverity, process them now

for tarball in `cat $pending_coverity`; do
    $HOME/scripts/hwloc-nightly-coverity.pl \
        --filename=$tarball \
        --coverity-token=$coverity_token \
        --verbose \
        --logfile-dir=$HOME/coverity \
        --make-args="-j8"
done
rm -f $pending_coverity

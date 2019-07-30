# License and Signed-off-by token

In order to ensure that we can keep distributing hwloc under our
[open source license](/COPYING), we need to ensure that all
contributions are compatible with that license.

To that end, we require that all Git commits contributed to hwloc
have a "Signed-off-by" token indicating that the commit author agrees
with [Open MPI's Contributor's
Declaration](https://github.com/open-mpi/ompi/wiki/Administrative-rules#contributors-declaration).

If you have not already done so, please ensure that:

1. Every commit contains exactly the "Signed-off-by" token.  You can
add this token via `git commit -s`.
1. The email address after "Signed-off-by" must match the Git commit
email address.

# Copyright

You may also update the copyright headers whenever you modify
a file. `contrib/update-my-copyright.pl` may help you doing so.
It requires you to set the `HWLOC_COPYRIGHT_FORMAT_NAME` environment
to something like `Inria.  All rights reserved.`
and `HWLOC_COPYRIGHT_SEARCH_NAME` to something like `Inria`.

You may even call `contrib/update-my-copyright.pl --check-only` from
the git pre-commit hook so that it prevents committing without
updated copyright headers (unless `--no-verify` is given).

Major contributors are also listed in the [Authors](/AUTHORS) file.

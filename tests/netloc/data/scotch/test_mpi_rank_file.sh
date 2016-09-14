export NETLOC_CURRENTSLOTS=$(realpath scotch/resources.txt)
export NETLOC_TOPODIR=$(realpath ./plafrim2)
export NETLOC_PARTITION=miriel
export NETLOC_SUBNET=fe80:0000:0000:0000

netloc_mpi_rank_file scotch/app.mat test.rank

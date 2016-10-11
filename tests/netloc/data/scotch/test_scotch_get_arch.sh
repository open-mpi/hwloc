export NETLOC_TOPODIR=$(realpath ./plafrim2)
export NETLOC_PARTITION=miriel
export NETLOC_SUBNET=fe80:0000:0000:0000
export NETLOC_CURRENTSLOTS=$(realpath scotch/resources.txt)

cd $(dirname $0)
./scotch_get_arch

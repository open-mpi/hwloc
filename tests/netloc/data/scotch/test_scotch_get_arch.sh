export NETLOC_TOPOFILE=$(realpath ./plafrim2/netloc/IB-fe80:0000:0000:0000-nodes.txt)
export NETLOC_PARTITION=miriel
export NETLOC_CURRENTSLOTS=$(realpath scotch/resources.txt)

cd $(dirname $0)
./scotch_get_arch

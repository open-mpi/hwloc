#
# Copyright © 2018-2019 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

_lstopo() {
    local INPUT_FORMAT=(xml synthetic fsroot cpuid)
    local OUTPUT_FORMAT=(console ascii fig pdf ps png svg xml synthetic)
    local TYPES=("Machine" "Misc" "Group" "NUMANode" "Package" "L1" "L2" "L3" "L4" "L5" "L1i" "L2i" "L3i" "Core" "Bridge" "PCIDev" "OSDev" "PU")
    local FILTERKINDS=("none" "all" "structure" "important")
    local OPTIONS=(-l --logical
		   -p --physical
		   --output-format --of
		   -f --force
		   --only
		   -v --verbose
		   -s --silent
		   --distances
		   -c --cpuset
		   -C --cpuset-only
		   --taskset
		   --filter --ignore
		   --no-caches
		   --no-useless-caches
		   --no-icaches
		   --merge
		   --no-collapse
		   --factorize --factorize=
		   --no-factorize --no-factorize=
		   --restrict
		   --restrict-flags
		   --no-io
		   --no-bridges
		   --whole-io
		   --input -i
		   --input-format --if
		   --thissystem
		   --pid
		   --disallowed --whole-system
		   --allow
		   --children-order
		   --fontsize
		   --gridsize
		   --linespacing
		   --horiz --horiz=
		   --vert --vert=
		   --rect --rect=
		   --text --text=
		   --no-text --no-text=
		   --index --index=
		   --no-index --no-index=
		   --attrs --attrs=
		   --no-attrs --no-attrs=
		   --no-legend
		   --append-legend
		   --binding-color
		   --disallowed-color
		   --top-color
		   --export-xml-flags
		   --export-synthetic-flags
		   --ps --top
		   --version
		   -h --help
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}
    local prev=${COMP_WORDS[COMP_CWORD-1]}
    local pprev=${COMP_WORDS[COMP_CWORD-2]}
    local ppprev=${COMP_WORDS[COMP_CWORD-3]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    elif [[ "$pprev" == "--filter" && "$cur" == ":" ]] ; then
	COMPREPLY=( `compgen -W "${FILTERKINDS[*]}"` )
    elif [[ "$ppprev" == "--filter" && "$prev" == ":" ]] ; then
	COMPREPLY=( `compgen -W "${FILTERKINDS[*]}" -- "$cur"` )
    elif [[ "$cur" == "=" && " --horiz --vert --rect --text --no-text --index --no-index --attrs --no-attrs --no-factorize " =~ " $prev " ]] ; then
	COMPREPLY=( `compgen -W "${TYPES[*]}"` )
	# we could also support "<type1>,<type2>,..." for --index/attrs/text but "," is not a completion word separator
    elif [[ "$prev" == "=" && " --horiz --vert --rect --text --no-text --index --no-index --attrs --no-attrs --no-factorize --factorize " =~ " $pprev " ]] ; then
	COMPREPLY=( `compgen -W "${TYPES[*]}" -- "$cur"` )
    elif [[ "$cur" == "=" && "--factorize" = "$prev" ]] ; then
	COMPREPLY=( `compgen -W "${TYPES[*]}"` "<N>" "<N,F,L>" )
    else
	case "$prev" in
	    --of | --output-format)
		COMPREPLY=( `compgen -W "${OUTPUT_FORMAT[*]}" -- "$cur"` )
		;;
	    --only | --ignore)
		COMPREPLY=( `compgen -W "${TYPES[*]}" -- "$cur"` )
		;;
	    --filter)
		COMPREPLY=( `compgen -W "${TYPES[*]/%/:} cache: icache: io:" -- "$cur"` ) && compopt -o nospace
		;;
	    --restrict)
		COMPREPLY=( `compgen -W "binding <cpuset>" -- "$cur"` )
		;;
	    -i | --input)
		_filedir xml
		;;
	    --if | --input-format)
		COMPREPLY=( `compgen -W "${INPUT_FORMAT[*]}" -- "$cur"` )
		;;
	    --pid)
		COMPREPLY=( "<pid>" "" )
		;;
	    --allow)
		COMPREPLY=( `compgen -W "all local <mask> nodeset=<mask>" -- "$cur"` )
		;;
	    --restrict-flags | --export-xml-flags | --export-synthetic-flags | --fontsize | --gridsize | --linespacing)
		COMPREPLY=( "<integer>" "" )
		;;
	    --append-legend)
		COMPREPLY=( "<line of text>" "" )
		;;
	    --binding-color | --disallowed-color)
		COMPREPLY=( `compgen -W "none" -- "$cur"` )
		;;
	    --top-color)
		COMPREPLY=( `compgen -W "none <#xxyyzz>" -- "$cur"` )
		;;
	    --children-order)
		COMPREPLY=( `compgen -W "plain memoryabove" -- "$cur"` )
		;;
	esac
    fi
}
complete -F _lstopo lstopo
complete -F _lstopo hwloc-ls


_hwloc-info(){
    local TYPES=("Machine" "Misc" "Group" "NUMANode" "Package" "L1" "L2" "L3" "L4" "L5" "L1i" "L2i" "L3i" "Core" "Bridge" "PCIDev" "OSDev" "PU")
    local FILTERKINDS=("none" "all" "structure" "important")
    local OPTIONS=(--objects
		   --topology
		   --support
		   -v --verbose
		   -s --silent
		   --ancestors
		   --ancestor
		   --children
		   --descendants
		   -n
		   --restruct
		   --filter
		   --no-icaches
		   --no-io
		   --no-bridges
		   --whole-io
		   --input -i
		   --input-format --if
		   --thissystem
		   --pid
		   --disallowed --whole-system
		   -l --logical
		   -p --physical
		   --version
		   -h --help
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}
    local prev=${COMP_WORDS[COMP_CWORD-1]}
    local pprev=${COMP_WORDS[COMP_CWORD-2]}
    local ppprev=${COMP_WORDS[COMP_CWORD-3]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    elif [[ "$pprev" == "--filter" && "$cur" == ":" ]] ; then
	COMPREPLY=( `compgen -W "${FILTERKINDS[*]}"` )
    elif [[ "$ppprev" == "--filter" && "$prev" == ":" ]] ; then
	COMPREPLY=( `compgen -W "${FILTERKINDS[*]}" -- "$cur"` )
    else
	case "$prev" in
	    --restrict)
		COMPREPLY=( `compgen -W "binding <cpuset>" -- "$cur"` )
		;;
	    -i | --input)
		_filedir xml
		;;
	    --if | --input-format)
		COMPREPLY=( `compgen -W "${INPUT_FORMAT[*]}" -- "$cur"` )
		;;
	    --pid)
		COMPREPLY=( "<pid>" "" )
		;;
	    --filter)
		COMPREPLY=( `compgen -W "${TYPES[*]/%/:} cache: icache: io:" -- "$cur"` ) && compopt -o nospace
		;;
	    --ancestor | --descendants)
		COMPREPLY=( `compgen -W "${TYPES[*]}" -- "$cur"` )
		;;
	esac
    fi
}
complete -F _hwloc-info hwloc-info


_hwloc-bind(){
    local OPTIONS=(--cpubind
		   --membind
		   --mempolicy
		   --logical -l
		   --physical -p
		   --single
		   --strict
		   --get
		   -e --get-last-cpu-location
		   --nodeset
		   --pid
		   --tid
		   --taskset
		   --restrict
		   --disallowed --whole-system
		   --hbm
		   --no-hbm
		   -f --force
		   -q --quiet
		   -v --verbose
		   --version
		   -h --help
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}
    local prev=${COMP_WORDS[COMP_CWORD-1]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$prev" in
	    --mempolicy)
		COMPREPLY=( `compgen -W "default firsttouch bind interleave nexttouch" -- "$cur"` )
		;;
	    --pid)
	        COMPREPLY=( "<pid>" "" )
		;;
	    --tid)
		COMPREPLY=( "<tid>" "" )
		;;
	    --restrict)
		COMPREPLY=( "<bitmask>" "" )
		;;
	esac
    fi
}
complete -F _hwloc-bind hwloc-bind


_hwloc-calc(){
    local TYPES=("Machine" "Misc" "Group" "NUMANode" "Package" "L1" "L2" "L3" "L4" "L5" "L1i" "L2i" "L3i" "Core" "Bridge" "PCIDev" "OSDev" "PU")
    local OPTIONS=(-N --number-of
		   -I --intersect
		   -H --hierarchical
		   --largest
		   -l --logical
		   -p --physical
		   --li --logical-input
		   --lo --logical-output
		   --pi --physical-input
		   --po --physical-output
		   -n --nodeset
		   --ni --nodeset-input
		   --no --nodeset-output
		   --sep
		   --taskset
		   --single
		   --restrict
		   --disallowed --whole-system
		   --input -i
		   --input-format --if
		   -q --quiet
		   -v --verbose
		   --version
		   -h --help
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}
    local prev=${COMP_WORDS[COMP_CWORD-1]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$prev" in
	    -N | --number-of | -I | --intersect)
		COMPREPLY=( `compgen -W "${TYPES[*]}" -- "$cur"` )
		;;
	    -H | --hierarchical)
		COMPREPLY=( "<type1>.<type2>..." "" )
		;;
	    --sep)
		COMPREPLY=( "<separator>" "" )
		;;
	    -i | --input)
		_filedir xml
		;;
	    --if | --input-format)
		COMPREPLY=( `compgen -W "${INPUT_FORMAT[*]}" -- "$cur"` )
		;;
	    --restrict)
		COMPREPLY=( "<bitmask>" "" )
		;;
	esac
    fi
}
complete -F _hwloc-calc hwloc-calc


_hwloc-annotate(){
    local OPTIONS=(--ci --ri --cu --cd -h --help)
    local cur=${COMP_WORDS[COMP_CWORD]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"`)
    fi
    _filedir xml
}
complete -F _hwloc-annotate hwloc-annotate


_hwloc-diff(){
    local OPTIONS=(--refname
		   --version
		   -h --help
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}
    local prev=${COMP_WORDS[COMP_CWORD-1]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$prev" in
	    --refname)
		COMPREPLY=( "<reference topology identifier>" "")
		;;
	esac
    fi
    _filedir xml
}
complete -F _hwloc-diff hwloc-diff


_hwloc-patch(){
    local OPTIONS=(--R --reverse
		   --version
		   -h --help
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    fi
    _filedir xml
}
complete -F _hwloc-patch hwloc-patch


_hwloc-compress-dir(){
    local OPTIONS=(-R --reverse
		   -v --verbose
		   -h --help
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    fi
    _filedir -d
}
complete -F _hwloc-compress-dir hwloc-compress-dir


_hwloc-distrib(){
    local TYPES=("Machine" "Misc" "Group" "NUMANode" "Package" "L1" "L2" "L3" "L4" "L5" "L1i" "L2i" "L3i" "Core" "Bridge" "PCIDev" "OSDev" "PU")
    local OPTIONS=(--ignore
		   --from
		   --to
		   --at
		   --reverse
		   --restrict
		   --disallowed --whole-system
		   --input -i
		   --input-format --if
		   --single
		   --taskset
		   -v --verbose
		   --version
		   -h --help
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}
    local prev=${COMP_WORDS[COMP_CWORD-1]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$prev" in
	    --ignore | --from | --to | --at)
		COMPREPLY=( `compgen -W "${TYPES[*]}" -- "$cur"` )
		;;
	    -i | --input)
		_filedir xml
		;;
	    --if | --input-format)
		COMPREPLY=( `compgen -W "${INPUT_FORMAT[*]}" -- "$cur"` )
		;;
	    --restrict)
		COMPREPLY=( "<bitmask>" "" )
		;;
	esac
    fi
}
complete -F _hwloc-distrib hwloc-distrib


_hwloc-ps(){
    local OPTIONS=(-a
		   --pid
		   --name
		   -l --logical
		   -p --physical
		   -c --cpuset
		   -t --threads
		   -e --get-last-cpu-location
		   --pid-cmd
		   --disallowed --whole-system
		   --json-server
		   --json-port
		   -v --verbose
		   -h --help
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}
    local prev=${COMP_WORDS[COMP_CWORD-1]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$prev" in
	--name)
	    COMPREPLY=( "<task name>" "" )
	    ;;
	--pid)
	    COMPREPLY=( "<pid>" "" )
	    ;;
	--pid-cmd)
	    _filedir
	    ;;
	--json-port)
	    COMPREPLY=( "<port>" "" )
	    ;;
	esac
    fi
}
complete -F _hwloc-ps hwloc-ps


_hwloc-gather-cpuid(){
    local OPTIONS=(-c
		   -s --silent
		   -h --help
		   )
    local cur=${COMP_WORDS[COMP_CWORD]}
    local prev=${COMP_WORDS[COMP_CWORD-1]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$prev" in
	    -c)
	        COMPREPLY=( "<index of cpu to operate on>" "" )
		;;
	esac
    fi
}
complete -F _hwloc-gather-cpuid hwloc-gather-cpuid


_hwloc-gather-topology(){
    local OPTIONS=(--io
		   --dmi
		   --keep
		   -h --help
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}

    if [[ $COMP_CWORD == 1 || $cur == -* ]] ; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    fi
}
complete -F _hwloc-gather-topology hwloc-gather-topology

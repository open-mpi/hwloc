_lstopo() {
    local INPUT_FORMAT=(xml synthetic fsroot cpuid)
    local OUTPUT_FORMAT=(console ascii fig pdf ps png svg xml synthetic)
    local TYPES=("Machine" "Misc" "Group" "NUMANode" "Package" "L1" "L2" "L3" "L4" "L5" "L1i" "L2i" "L3i" "Core" "Bridge" "PCIDev" "OSDev" "PU")
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
		    --horiz --horiz=\<type\>
		    --vert --vert=\<type\>
		    --rect --rect=\<type\>
		    --text --text=\<type\>,...
		    --no-text --no-text=\<type\>,...
		    --index --index=\<type\>,...
		    --no-index --no-index=\<type\>,...
		    --attrs --attrs=\<type\>,...
		    --no-attrs --no-attrs=\<type\>,...
		    --no-legend
		    --append-legend
		    --binding-color
		    --disallowed-color
		    --top-color
		    --export-xml-flags
		    --export-synthetic-flags
		    --ps --top
		    --version
		   )
    local cur=${COMP_WORDS[COMP_CWORD]}

    if [ $COMP_CWORD == 1 ]; then
	COMPREPLY=($( compgen -W "${OPTIONS[*]}" -- "$cur" ))
    else
	case "$3" in
	    --of | --output-format)
		COMPREPLY=( `compgen -W "${OUTPUT_FORMAT[*]}" -- "$cur"` )
		;;
	    --only | --ignore)
		COMPREPLY=( `compgen -W "${TYPES[*]}" -- "$cur"` )
		;;
	    --filter)
		COMPREPLY=( "<type>:<none,all,structure>" "" )
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
    local OPTIONS=(
	--objects
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

    if [ $COMP_CWORD == 1 ]; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$3" in
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
		COMPREPLY=( "<type>:<none,all,structure>" "" )
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

    if [ $COMP_CWORD == 1 ]; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$3" in
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
		   -pi --physical-input
		   -po --physical-output
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

    if [ $COMP_CWORD == 1 ]; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$3" in
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

    if [ $COMP_CWORD == 1 ]; then
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

    if [ $COMP_CWORD == 1 ]; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$3" in
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

    if [ $COMP_CWORD == 1 ]; then
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

    if [ $COMP_CWORD == 1 ]; then
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

    if [ $COMP_CWORD == 1 ]; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$3" in
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
		  )
    local cur=${COMP_WORDS[COMP_CWORD]}

    if [ $COMP_CWORD == 1 ]; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$3" in
	--name)
	    COMPREPLY=( "<task name>" "" )
	    ;;
	--pid)
	    COMPREPLY=( "<pid>" "" )
	    ;;
	--pid-cmd)
	    _filedir
	    ;;
	esac
    fi
}
complete -F _hwloc-ps hwloc-ps


_hwloc-gather-cpuid(){
    local OPTIONS=(-c
		   -h --help
		   )
    local cur=${COMP_WORDS[COMP_CWORD]}

    if [ $COMP_CWORD == 1 ]; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    else
	case "$3" in
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

    if [ $COMP_CWORD == 1 ]; then
	COMPREPLY=( `compgen -W "${OPTIONS[*]}" -- "$cur"` )
    fi
}
complete -F _hwloc-gather-topology hwloc-gather-topology

#include <stdio.h> // for scotch
#include <scotch.h>
#include <netloc.h>
#include <netlocscotch.h>

int main()
{
    int ret;
    SCOTCH_Arch arch;
    SCOTCH_Arch subarch;

    ret = netlocscotch_build_global_arch(&arch);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }
    FILE *arch_file = fopen("arch.scotch", "w");
    SCOTCH_archSave(&arch, arch_file);
    fclose(arch_file);

    SCOTCH_archExit(&arch);

    ret = netlocscotch_build_current_arch(&arch, &subarch);
    if( NETLOC_SUCCESS != ret ) {
        return ret;
    }
    FILE *subarch_file = fopen("subarch.scotch", "w");
    SCOTCH_archSave(&subarch, subarch_file);
    fclose(subarch_file);

    SCOTCH_archExit(&arch);
    SCOTCH_archExit(&subarch);


    return 0;
}


#include <stdio.h>
#include <scotch.h>
#include <netlocscotch.h>

int main()
{
    int ret;
    SCOTCH_Arch subarch[1];
    ret = netlocscotch_build_current_arch(subarch);
    return ret;
}

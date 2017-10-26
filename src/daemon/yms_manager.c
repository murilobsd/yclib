#include "yms.h"
#include "rns.h"

int main()
{
    rns_proc_service();
    rns_proc_daemon("./bin/yms");
    
    return 0;
}

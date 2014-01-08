#include <iostream>
#include "ossie/ossieSupport.h"

#include "psk_soft.h"


int main(int argc, char* argv[])
{
    psk_soft_i* psk_soft_servant;
    Resource_impl::start_component(psk_soft_servant, argc, argv);
    return 0;
}

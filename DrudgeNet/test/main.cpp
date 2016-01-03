//
//  main.cpp
//  DrudgeNetTest
//
//  Created by The Drudgerist on 20/12/15.
//  Copyright © 2015 The Drudgerist. All rights reserved.
//

#include "SocketTests.hpp"
#include "ConnectionTests.hpp"
#include "ReliabilityTests.hpp"
#include "MeshTests.hpp"
#include "StreamTests.hpp"
#include "TransportLANTests.hpp"
#include <iostream>

void WaitForInput() {
    do {
        std::cout << '\n' <<"Press a key to continue...";
    } while (std::cin.get() != '\n');
}

int main(int argc, const char * argv[])
{
    RunSocketTests();

    WaitForInput();
    
    RunConnectionTests();
    
    WaitForInput();
    
    RunReliabilityTests();

    WaitForInput(),
    
    RunMeshTests();

    WaitForInput();
    
    RunStreamTests();
    
    WaitForInput();
    
    RunTransportTests();
    
    return 0;
}

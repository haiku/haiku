#ifndef PRINT_TRANSPORT_ADD_ON_H
#define PRINT_TRANSPORT_ADD_ON_H

#include <DataIO.h>
#include <Directory.h>
#include <Message.h>

extern "C" BDataIO* instanciate_transport(BDirectory* printe, BMessage* msg);

#endif PRINT_TRANSPORT_ADD_ON_H

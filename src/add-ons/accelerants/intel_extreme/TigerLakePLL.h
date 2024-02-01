/*
 * Copyright 2024, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef TIGERLAKEPLL_H
#define TIGERLAKEPLL_H

#include "intel_extreme.h"

#include <SupportDefs.h>


bool ComputeHdmiDpll(int freq, int* Pdiv, int* Qdiv, int* Kdiv, float* bestdco);
bool ComputeDisplayPortDpll(int freq, int* Pdiv, int* Qdiv, int* Kdiv, float* bestdco);

status_t ProgramPLL(int which, int Pdiv, int Qdiv, int Kdiv, float dco);


#endif /* !TIGERLAKEPLL_H */

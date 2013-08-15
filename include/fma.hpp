/**
 *	\file
 */
 
 
#pragma once
 

#include <cmath>


#ifndef FP_FAST_FMA
#ifdef fma
#undef fma
#endif
#define fma(a,b,c) (((a)*(b))+(c))
#endif

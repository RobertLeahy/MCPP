#include <noise.hpp>
#include <random.hpp>
#include <random>


//	Simplex functions and pre-populated data
//	tables from:
//
//	https://code.google.com/p/battlestar-tux/source/browse/procedural/simplexnoise.h
//	https://code.google.com/p/battlestar-tux/source/browse/procedural/simplexnoise.cpp
//
//	And ported to use doubles and signed
//	words (rather than floats and ints).
//
//	Data tables changed to byte types
//	for compactness.


namespace MCPP {


	static const SByte grad3 [12][3]={
		{1,1,0},
		{-1,1,0},
		{1,-1,0},
		{-1,-1,0},
		{1,0,1},
		{-1,0,1},
		{1,0,-1},
		{-1,0,-1},
		{0,1,1},
		{0,-1,1},
		{0,1,-1},
		{0,-1,-1}
	};
	
	
	static const SByte grad4 [32][4]={
		{0,1,1,1},
		{0,1,1,-1},
		{0,1,-1,1},
		{0,1,-1,-1},
		{0,-1,1,1},
		{0,-1,1,-1},
		{0,-1,-1,1},
		{0,-1,-1,-1},
		{1,0,1,1},
		{1,0,1,-1},
		{1,0,-1,1},
		{1,0,-1,-1},
		{-1,0,1,1},
		{-1,0,1,-1},
		{-1,0,-1,1},
		{-1,0,-1,-1},
		{1,1,0,1},
		{1,1,0,-1},
		{1,-1,0,1},
		{1,-1,0,-1},
		{-1,1,0,1},
		{-1,1,0,-1},
		{-1,-1,0,1},
		{-1,-1,0,-1},
		{1,1,1,0},
		{1,1,-1,0},
		{1,-1,1,0},
		{1,-1,-1,0},
		{-1,1,1,0},
		{-1,1,-1,0},
		{-1,-1,1,0},
		{-1,-1,-1,0}
	};
	
	
	static const Byte simplex [64][4]={
		{0,1,2,3},
		{0,1,3,2},
		{0,0,0,0},
		{0,2,3,1},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{1,2,3,0},
		{0,2,1,3},
		{0,0,0,0},
		{0,3,1,2},
		{0,3,2,1},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{1,3,2,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{1,2,0,3},
		{0,0,0,0},
		{1,3,0,2},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{2,3,0,1},
		{2,3,1,0},
		{1,0,2,3},
		{1,0,3,2},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{2,0,3,1},
		{0,0,0,0},
		{2,1,3,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{2,0,1,3},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{3,0,1,2},
		{3,0,2,1},
		{0,0,0,0},
		{3,1,2,0},
		{2,1,0,3},
		{0,0,0,0},
		{0,0,0,0},
		{0,0,0,0},
		{3,1,0,2},
		{0,0,0,0},
		{3,2,0,1},
		{3,2,1,0}
	};
	
	
	template <Word, typename T>
	inline constexpr Double dot_impl (const T *) noexcept {
	
		return 0;
	
	}
	
	
	template <Word i, typename T, typename... Args>
	inline Double dot_impl (const T * g, Double curr, Args &&... args) noexcept {
	
		return fma(
			g[i],
			curr,
			dot_impl<i+1>(g,std::forward<Args>(args)...)
		);
	
	}
	
	
	template <typename T, typename... Args>
	inline Double dot (const T * g, Args &&... args) noexcept {
	
		return dot_impl<0>(
			g,
			std::forward<Args>(args)...
		);
	
	}
	
	
	static inline SWord fastfloor (Double x) noexcept {
	
		return static_cast<SWord>(
			(x>0)
				?	x
				:	(x-1)
		);
	
	}
	
	
	Simplex::Simplex (UInt64 seed) noexcept {
	
		//	Create and seed random number
		//	generator
		std::default_random_engine gen;
		gen.seed(seed);
	
		//	Initialize
		init(gen);
	
	}
	
	
	Simplex::Simplex () : Simplex(CryptoRandom<UInt64>()) {	}
	
	
	Double Simplex::operator () (Double x, Double y) const noexcept {
	
		// Noise contributions from the three corners
		Double n0, n1, n2;

		// Skew the input space to determine which simplex cell we're in
		Double F2 = 0.5 * (sqrt(3.0) - 1.0);
		// Hairy factor for 2D
		Double s = (x + y) * F2;
		SWord i = fastfloor( x + s );
		SWord j = fastfloor( y + s );

		Double G2 = (3.0 - sqrt(3.0)) / 6.0;
		Double t = (i + j) * G2;
		// Unskew the cell origin back to (x,y) space
		Double X0 = i-t;
		Double Y0 = j-t;
		// The x,y distances from the cell origin
		Double x0 = x-X0;
		Double y0 = y-Y0;

		// For the 2D case, the simplex shape is an equilateral triangle.
		// Determine which simplex we are in.
		SWord i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
		if(x0>y0) {i1=1; j1=0;} // lower triangle, XY order: (0,0)->(1,0)->(1,1)
		else {i1=0; j1=1;} // upper triangle, YX order: (0,0)->(0,1)->(1,1)

		// A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
		// a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
		// c = (3-sqrt(3))/6
		Double x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
		Double y1 = y0 - j1 + G2;
		Double x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords
		Double y2 = y0 - 1.0 + 2.0 * G2;

		// Work out the hashed gradient indices of the three simplex corners
		SWord ii = i & 255;
		SWord jj = j & 255;
		SWord gi0 = permutation[ii+permutation[jj]] % 12;
		SWord gi1 = permutation[ii+i1+permutation[jj+j1]] % 12;
		SWord gi2 = permutation[ii+1+permutation[jj+1]] % 12;

		// Calculate the contribution from the three corners
		Double t0 = 0.5 - x0*x0-y0*y0;
		if(t0<0) n0 = 0.0;
		else {
			t0 *= t0;
			n0 = t0 * t0 * dot(grad3[gi0], x0, y0); // (x,y) of grad3 used for 2D gradient
		}

		Double t1 = 0.5 - x1*x1-y1*y1;
		if(t1<0) n1 = 0.0;
		else {
			t1 *= t1;
			n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
		}

		Double t2 = 0.5 - x2*x2-y2*y2;
		if(t2<0) n2 = 0.0;
		else {
			t2 *= t2;
			n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
		}

		// Add contributions from each corner to get the final noise value.
		// The result is scaled to return values in the SWorderval [-1,1].
		return 70.0 * (n0 + n1 + n2);
	
	}
	
	
	Double Simplex::operator () (Double x, Double y, Double z) const noexcept {
	
		Double n0, n1, n2, n3; // Noise contributions from the four corners

		// Skew the input space to determine which simplex cell we're in
		Double F3 = 1.0/3.0;
		Double s = (x+y+z)*F3; // Very nice and simple skew factor for 3D
		SWord i = fastfloor(x+s);
		SWord j = fastfloor(y+s);
		SWord k = fastfloor(z+s);

		Double G3 = 1.0/6.0; // Very nice and simple unskew factor, too
		Double t = (i+j+k)*G3;
		Double X0 = i-t; // Unskew the cell origin back to (x,y,z) space
		Double Y0 = j-t;
		Double Z0 = k-t;
		Double x0 = x-X0; // The x,y,z distances from the cell origin
		Double y0 = y-Y0;
		Double z0 = z-Z0;

		// For the 3D case, the simplex shape is a slightly irregular tetrahedron.
		// Determine which simplex we are in.
		SWord i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
		SWord i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords

		if(x0>=y0) {
			if(y0>=z0) { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; } // X Y Z order
			else if(x0>=z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; } // X Z Y order
			else { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; } // Z X Y order
		}
		else { // x0<y0
			if(y0<z0) { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; } // Z Y X order
			else if(x0<z0) { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; } // Y Z X order
			else { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; } // Y X Z order
		}

		// A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
		// a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
		// a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
		// c = 1/6.
		Double x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
		Double y1 = y0 - j1 + G3;
		Double z1 = z0 - k1 + G3;
		Double x2 = x0 - i2 + 2.0*G3; // Offsets for third corner in (x,y,z) coords
		Double y2 = y0 - j2 + 2.0*G3;
		Double z2 = z0 - k2 + 2.0*G3;
		Double x3 = x0 - 1.0 + 3.0*G3; // Offsets for last corner in (x,y,z) coords
		Double y3 = y0 - 1.0 + 3.0*G3;
		Double z3 = z0 - 1.0 + 3.0*G3;

		// Work out the hashed gradient indices of the four simplex corners
		SWord ii = i & 255;
		SWord jj = j & 255;
		SWord kk = k & 255;
		SWord gi0 = permutation[ii+permutation[jj+permutation[kk]]] % 12;
		SWord gi1 = permutation[ii+i1+permutation[jj+j1+permutation[kk+k1]]] % 12;
		SWord gi2 = permutation[ii+i2+permutation[jj+j2+permutation[kk+k2]]] % 12;
		SWord gi3 = permutation[ii+1+permutation[jj+1+permutation[kk+1]]] % 12;

		// Calculate the contribution from the four corners
		Double t0 = 0.6 - x0*x0 - y0*y0 - z0*z0;
		if(t0<0) n0 = 0.0;
		else {
			t0 *= t0;
			n0 = t0 * t0 * dot(grad3[gi0], x0, y0, z0);
		}

		Double t1 = 0.6 - x1*x1 - y1*y1 - z1*z1;
		if(t1<0) n1 = 0.0;
		else {
			t1 *= t1;
			n1 = t1 * t1 * dot(grad3[gi1], x1, y1, z1);
		}

		Double t2 = 0.6 - x2*x2 - y2*y2 - z2*z2;
		if(t2<0) n2 = 0.0;
		else {
			t2 *= t2;
			n2 = t2 * t2 * dot(grad3[gi2], x2, y2, z2);
		}

		Double t3 = 0.6 - x3*x3 - y3*y3 - z3*z3;
		if(t3<0) n3 = 0.0;
		else {
			t3 *= t3;
			n3 = t3 * t3 * dot(grad3[gi3], x3, y3, z3);
		}

		// Add contributions from each corner to get the final noise value.
		// The result is scaled to stay just inside [-1,1]
		return 32.0*(n0 + n1 + n2 + n3);
		
	}
	

	Double Simplex::operator () (Double w, Double x, Double y, Double z) const noexcept {
	
		// The skewing and unskewing factors are hairy again for the 4D case
		Double F4 = (sqrt(5.0)-1.0)/4.0;
		Double G4 = (5.0-sqrt(5.0))/20.0;
		Double n0, n1, n2, n3, n4; // Noise contributions from the five corners

		// Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
		Double s = (x + y + z + w) * F4; // Factor for 4D skewing
		SWord i = fastfloor(x + s);
		SWord j = fastfloor(y + s);
		SWord k = fastfloor(z + s);
		SWord l = fastfloor(w + s);
		Double t = (i + j + k + l) * G4; // Factor for 4D unskewing
		Double X0 = i - t; // Unskew the cell origin back to (x,y,z,w) space
		Double Y0 = j - t;
		Double Z0 = k - t;
		Double W0 = l - t;

		Double x0 = x - X0; // The x,y,z,w distances from the cell origin
		Double y0 = y - Y0;
		Double z0 = z - Z0;
		Double w0 = w - W0;

		// For the 4D case, the simplex is a 4D shape I won't even try to describe.
		// To find out which of the 24 possible simplices we're in, we need to
		// determine the magnitude ordering of x0, y0, z0 and w0.
		// The method below is a good way of finding the ordering of x,y,z,w and
		// then find the correct traversal order for the simplex we're in.
		// First, six pair-wise comparisons are performed between each possible pair
		// of the four coordinates, and the results are used to add up binary bits
		// for an SWordeger index.
		SWord c1 = (x0 > y0) ? 32 : 0;
		SWord c2 = (x0 > z0) ? 16 : 0;
		SWord c3 = (y0 > z0) ? 8 : 0;
		SWord c4 = (x0 > w0) ? 4 : 0;
		SWord c5 = (y0 > w0) ? 2 : 0;
		SWord c6 = (z0 > w0) ? 1 : 0;
		SWord c = c1 + c2 + c3 + c4 + c5 + c6;

		SWord i1, j1, k1, l1; // The SWordeger offsets for the second simplex corner
		SWord i2, j2, k2, l2; // The SWordeger offsets for the third simplex corner
		SWord i3, j3, k3, l3; // The SWordeger offsets for the fourth simplex corner

		// simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
		// Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
		// impossible. Only the 24 indices which have non-zero entries make any sense.
		// We use a thresholding to set the coordinates in turn from the largest magnitude.
		// The number 3 in the "simplex" array is at the position of the largest coordinate.
		i1 = simplex[c][0]>=3 ? 1 : 0;
		j1 = simplex[c][1]>=3 ? 1 : 0;
		k1 = simplex[c][2]>=3 ? 1 : 0;
		l1 = simplex[c][3]>=3 ? 1 : 0;
		// The number 2 in the "simplex" array is at the second largest coordinate.
		i2 = simplex[c][0]>=2 ? 1 : 0;
		j2 = simplex[c][1]>=2 ? 1 : 0;
		k2 = simplex[c][2]>=2 ? 1 : 0;
		l2 = simplex[c][3]>=2 ? 1 : 0;
		// The number 1 in the "simplex" array is at the second smallest coordinate.
		i3 = simplex[c][0]>=1 ? 1 : 0;
		j3 = simplex[c][1]>=1 ? 1 : 0;
		k3 = simplex[c][2]>=1 ? 1 : 0;
		l3 = simplex[c][3]>=1 ? 1 : 0;
		// The fifth corner has all coordinate offsets = 1, so no need to look that up.

		Double x1 = x0 - i1 + G4; // Offsets for second corner in (x,y,z,w) coords
		Double y1 = y0 - j1 + G4;
		Double z1 = z0 - k1 + G4;
		Double w1 = w0 - l1 + G4;
		Double x2 = x0 - i2 + 2.0*G4; // Offsets for third corner in (x,y,z,w) coords
		Double y2 = y0 - j2 + 2.0*G4;
		Double z2 = z0 - k2 + 2.0*G4;
		Double w2 = w0 - l2 + 2.0*G4;
		Double x3 = x0 - i3 + 3.0*G4; // Offsets for fourth corner in (x,y,z,w) coords
		Double y3 = y0 - j3 + 3.0*G4;
		Double z3 = z0 - k3 + 3.0*G4;
		Double w3 = w0 - l3 + 3.0*G4;
		Double x4 = x0 - 1.0 + 4.0*G4; // Offsets for last corner in (x,y,z,w) coords
		Double y4 = y0 - 1.0 + 4.0*G4;
		Double z4 = z0 - 1.0 + 4.0*G4;
		Double w4 = w0 - 1.0 + 4.0*G4;

		// Work out the hashed gradient indices of the five simplex corners
		SWord ii = i & 255;
		SWord jj = j & 255;
		SWord kk = k & 255;
		SWord ll = l & 255;
		SWord gi0 = permutation[ii+permutation[jj+permutation[kk+permutation[ll]]]] % 32;
		SWord gi1 = permutation[ii+i1+permutation[jj+j1+permutation[kk+k1+permutation[ll+l1]]]] % 32;
		SWord gi2 = permutation[ii+i2+permutation[jj+j2+permutation[kk+k2+permutation[ll+l2]]]] % 32;
		SWord gi3 = permutation[ii+i3+permutation[jj+j3+permutation[kk+k3+permutation[ll+l3]]]] % 32;
		SWord gi4 = permutation[ii+1+permutation[jj+1+permutation[kk+1+permutation[ll+1]]]] % 32;

		// Calculate the contribution from the five corners
		Double t0 = 0.6 - x0*x0 - y0*y0 - z0*z0 - w0*w0;
		if(t0<0) n0 = 0.0;
		else {
			t0 *= t0;
			n0 = t0 * t0 * dot(grad4[gi0], x0, y0, z0, w0);
		}

		Double t1 = 0.6 - x1*x1 - y1*y1 - z1*z1 - w1*w1;
		if(t1<0) n1 = 0.0;
		else {
			t1 *= t1;
			n1 = t1 * t1 * dot(grad4[gi1], x1, y1, z1, w1);
		}

		Double t2 = 0.6 - x2*x2 - y2*y2 - z2*z2 - w2*w2;
		if(t2<0) n2 = 0.0;
		else {
			t2 *= t2;
			n2 = t2 * t2 * dot(grad4[gi2], x2, y2, z2, w2);
		}

		Double t3 = 0.6 - x3*x3 - y3*y3 - z3*z3 - w3*w3;
		if(t3<0) n3 = 0.0;
		else {
			t3 *= t3;
			n3 = t3 * t3 * dot(grad4[gi3], x3, y3, z3, w3);
		}

		Double t4 = 0.6 - x4*x4 - y4*y4 - z4*z4 - w4*w4;
		if(t4<0) n4 = 0.0;
		else {
			t4 *= t4;
			n4 = t4 * t4 * dot(grad4[gi4], x4, y4, z4, w4);
		}

		// Sum up and scale the result to cover the range [-1,1]
		return 27.0 * (n0 + n1 + n2 + n3 + n4);
		
	}


}

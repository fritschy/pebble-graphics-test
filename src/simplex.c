#include <pebble.h>
#include "f32.h"

#if 1
/* Simplex Noise, Adapted from:
 *
 * A speed-improved simplex noise algorithm for 2D, 3D and 4D in Java.
 *
 * Based on example code by Stefan Gustavson (stegu@itn.liu.se).
 * Optimisations by Peter Eastman (peastman@drizzle.stanford.edu).
 * Better rank ordering method by Stefan Gustavson in 2012.
 *
 * This could be speeded up even further, but it's useful as it is.
 *
 * Version 2012-03-09
 *
 * This code was placed in the public domain by its original author,
 * Stefan Gustavson. You may use it as you see fit, but
 * attribution is appreciated.
 *
 */
static const f32 grad3[][3] = {
   {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
   {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
   {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
};

static const f32 grad4[][4] = {
   {0,1,1,1},{0,1,1,-1},{0,1,-1,1},{0,1,-1,-1},{0,-1,1,1},{0,-1,1,-1},{0,-1,-1,1},{0,-1,-1,-1},
   {1,0,1,1},{1,0,1,-1},{1,0,-1,1},{1,0,-1,-1},{-1,0,1,1},{-1,0,1,-1},{-1,0,-1,1},{-1,0,-1,-1},
   {1,1,0,1},{1,1,0,-1},{1,-1,0,1},{1,-1,0,-1},{-1,1,0,1},{-1,1,0,-1},{-1,-1,0,1},{-1,-1,0,-1},
   {1,1,1,0},{1,1,-1,0},{1,-1,1,0},{1,-1,-1,0},{-1,1,1,0},{-1,1,-1,0},{-1,-1,1,0},{-1,-1,-1,0}
};

#define SIMPLEX_NOISE_P(F)                                                     \
  151 F, 160 F, 137 F, 91 F, 90 F, 15 F, 131 F, 13 F, 201 F, 95 F, 96 F, 53 F, \
      194 F, 233 F, 7 F, 225 F, 140 F, 36 F, 103 F, 30 F, 69 F, 142 F, 8 F,    \
      99 F, 37 F, 240 F, 21 F, 10 F, 23 F, 190 F, 6 F, 148 F, 247 F, 120 F,    \
      234 F, 75 F, 0 F, 26 F, 197 F, 62 F, 94 F, 252 F, 219 F, 203 F, 117 F,   \
      35 F, 11 F, 32 F, 57 F, 177 F, 33 F, 88 F, 237 F, 149 F, 56 F, 87 F,     \
      174 F, 20 F, 125 F, 136 F, 171 F, 168 F, 68 F, 175 F, 74 F, 165 F, 71 F, \
      134 F, 139 F, 48 F, 27 F, 166 F, 77 F, 146 F, 158 F, 231 F, 83 F, 111 F, \
      229 F, 122 F, 60 F, 211 F, 133 F, 230 F, 220 F, 105 F, 92 F, 41 F, 55 F, \
      46 F, 245 F, 40 F, 244 F, 102 F, 143 F, 54 F, 65 F, 25 F, 63 F, 161 F,   \
      1 F, 216 F, 80 F, 73 F, 209 F, 76 F, 132 F, 187 F, 208 F, 89 F, 18 F,    \
      169 F, 200 F, 196 F, 135 F, 130 F, 116 F, 188 F, 159 F, 86 F, 164 F,     \
      100 F, 109 F, 198 F, 173 F, 186 F, 3 F, 64 F, 52 F, 217 F, 226 F, 250 F, \
      124 F, 123 F, 5 F, 202 F, 38 F, 147 F, 118 F, 126 F, 255 F, 82 F, 85 F,  \
      212 F, 207 F, 206 F, 59 F, 227 F, 47 F, 16 F, 58 F, 17 F, 182 F, 189 F,  \
      28 F, 42 F, 223 F, 183 F, 170 F, 213 F, 119 F, 248 F, 152 F, 2 F, 44 F,  \
      154 F, 163 F, 70 F, 221 F, 153 F, 101 F, 155 F, 167 F, 43 F, 172 F, 9 F, \
      129 F, 22 F, 39 F, 253 F, 19 F, 98 F, 108 F, 110 F, 79 F, 113 F, 224 F,  \
      232 F, 178 F, 185 F, 112 F, 104 F, 218 F, 246 F, 97 F, 228 F, 251 F,     \
      34 F, 242 F, 193 F, 238 F, 210 F, 144 F, 12 F, 191 F, 179 F, 162 F,      \
      241 F, 81 F, 51 F, 145 F, 235 F, 249 F, 14 F, 239 F, 107 F, 49 F, 192 F, \
      214 F, 31 F, 181 F, 199 F, 106 F, 157 F, 184 F, 84 F, 204 F, 176 F,      \
      115 F, 121 F, 50 F, 45 F, 127 F, 4 F, 150 F, 254 F, 138 F, 236 F, 205 F, \
      93 F, 222 F, 114 F, 67 F, 29 F, 24 F, 72 F, 243 F, 141 F, 128 F, 195 F,  \
      78 F, 66 F, 215 F, 61 F, 156 F, 180 F

static const int perm[] = {
   SIMPLEX_NOISE_P(+0),
   SIMPLEX_NOISE_P(+0)
};

static const int permMod12[] = {
   SIMPLEX_NOISE_P(%12),
   SIMPLEX_NOISE_P(%12)
};

#define SQRT_3 (1.7320508075688772f)
#define SQRT_5 (2.23606797749979f)

// Skewing and unskewing factors for 2, 3, and 4 dimensions
#define F2 i2f(0.5f*(SQRT_3-1.0f))
#define G2 i2f((3.0f-SQRT_3)/6.0f)
#define F3 i2f(1.0f/3.0f)
#define G3 i2f(1.0f/6.0f)
#define F4 i2f((SQRT_5-1.0f)/4.0f)
#define G4 i2f((5.0f-SQRT_5)/20.f)

static inline int fastfloor(f32 x) {
   return f2i(x);
}

static inline f32 dotg2(f32 const *g, f32 x, f32 y)
{
   return g[0]*x + g[1]*y;
}

static inline f32 dotg3(f32 const *g, f32 x, f32 y, f32 z)
{
   return dotg2(g, x, y) + g[2]*z;
}

static inline f32 dotg4(f32 const *g, f32 x, f32 y, f32 z, f32 w)
{
   return dotg3(g, x, y, z) + g[3]*w;
}

static inline int signbit(f32 x)
{
   return x < 0;
}

// 2D simplex noise
f32 snoise2(f32 xin, f32 yin) {
   f32 n0, n1, n2; // Noise contributions from the three corners
   // Skew the input space to determine which simplex cell we're in
   f32 s = f2i((xin+yin)*F2); // Hairy factor for 2D
   int i = fastfloor(xin+s);
   int j = fastfloor(yin+s);
   f32 t = f2i((i+j)*G2);
   f32 X0 = i-t; // Unskew the cell origin back to (x,y) space
   f32 Y0 = j-t;
   f32 x0 = xin-X0; // The x,y distances from the cell origin
   f32 y0 = yin-Y0;
   // For the 2D case, the simplex shape is an equilateral triangle.
   // Determine which simplex we are in.
   int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
   if(x0>y0) {i1=1; j1=0;} // lower triangle, XY order: (0,0)->(1,0)->(1,1)
   else {i1=0; j1=1;}      // upper triangle, YX order: (0,0)->(0,1)->(1,1)
   // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
   // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
   // c = (3-sqrt(3))/6
   f32 x1 = x0 - i2f(i1) + G2; // Offsets for middle corner in (x,y) unskewed coords
   f32 y1 = y0 - i2f(j1) + G2;
   f32 x2 = x0 - i2f(1) + 2 * G2; // Offsets for last corner in (x,y) unskewed coords
   f32 y2 = y0 - i2f(1) + 2 * G2;
   // Work out the hashed gradient indices of the three simplex corners
   int ii = i & 255;
   int jj = j & 255;
   int gi0 = permMod12[ii+perm[jj]];
   int gi1 = permMod12[ii+i1+perm[jj+j1]];
   int gi2 = permMod12[ii+1+perm[jj+1]];
   // Calculate the contribution from the three corners
   f32 t0 = i2f(0.5f) - f2i(x0*x0)-f2i(y0*y0);
   if(signbit(t0)) n0 = 0;
   else {
      t0 = f2i(t0*t0);
      n0 = f2i(f2i(t0 * t0) * dotg2(grad3[gi0], x0, y0));  // (x,y) of grad3 used for 2D gradient
   }
   f32 t1 = i2f(0.5f) - f2i(x1*x1)-f2i(y1*y1);
   if(signbit(t1)) n1 = 0;
   else {
      t1 = f2i(t1*t1);
      n1 = f2i(f2i(t1 * t1) * dotg2(grad3[gi1], x1, y1));
   }
   f32 t2 = i2f(0.5f) - f2i(x2*x2)-f2i(y2*y2);
   if(signbit(t2)) n2 = 0;
   else {
      t2 = f2i(t2*t2);
      n2 = f2i(f2i(t2 * t2) * dotg2(grad3[gi2], x2, y2));
   }
   // Add contributions from each corner to get the final noise value.
   // The result is scaled to return values in the interval [-1,1].
   return 70 * (n0 + n1 + n2);
}
#else
#define SIMPLEX_NOISE_P(F)                                                     \
  151 F, 160 F, 137 F, 91 F, 90 F, 15 F, 131 F, 13 F, 201 F, 95 F, 96 F, 53 F, \
      194 F, 233 F, 7 F, 225 F, 140 F, 36 F, 103 F, 30 F, 69 F, 142 F, 8 F,    \
      99 F, 37 F, 240 F, 21 F, 10 F, 23 F, 190 F, 6 F, 148 F, 247 F, 120 F,    \
      234 F, 75 F, 0 F, 26 F, 197 F, 62 F, 94 F, 252 F, 219 F, 203 F, 117 F,   \
      35 F, 11 F, 32 F, 57 F, 177 F, 33 F, 88 F, 237 F, 149 F, 56 F, 87 F,     \
      174 F, 20 F, 125 F, 136 F, 171 F, 168 F, 68 F, 175 F, 74 F, 165 F, 71 F, \
      134 F, 139 F, 48 F, 27 F, 166 F, 77 F, 146 F, 158 F, 231 F, 83 F, 111 F, \
      229 F, 122 F, 60 F, 211 F, 133 F, 230 F, 220 F, 105 F, 92 F, 41 F, 55 F, \
      46 F, 245 F, 40 F, 244 F, 102 F, 143 F, 54 F, 65 F, 25 F, 63 F, 161 F,   \
      1 F, 216 F, 80 F, 73 F, 209 F, 76 F, 132 F, 187 F, 208 F, 89 F, 18 F,    \
      169 F, 200 F, 196 F, 135 F, 130 F, 116 F, 188 F, 159 F, 86 F, 164 F,     \
      100 F, 109 F, 198 F, 173 F, 186 F, 3 F, 64 F, 52 F, 217 F, 226 F, 250 F, \
      124 F, 123 F, 5 F, 202 F, 38 F, 147 F, 118 F, 126 F, 255 F, 82 F, 85 F,  \
      212 F, 207 F, 206 F, 59 F, 227 F, 47 F, 16 F, 58 F, 17 F, 182 F, 189 F,  \
      28 F, 42 F, 223 F, 183 F, 170 F, 213 F, 119 F, 248 F, 152 F, 2 F, 44 F,  \
      154 F, 163 F, 70 F, 221 F, 153 F, 101 F, 155 F, 167 F, 43 F, 172 F, 9 F, \
      129 F, 22 F, 39 F, 253 F, 19 F, 98 F, 108 F, 110 F, 79 F, 113 F, 224 F,  \
      232 F, 178 F, 185 F, 112 F, 104 F, 218 F, 246 F, 97 F, 228 F, 251 F,     \
      34 F, 242 F, 193 F, 238 F, 210 F, 144 F, 12 F, 191 F, 179 F, 162 F,      \
      241 F, 81 F, 51 F, 145 F, 235 F, 249 F, 14 F, 239 F, 107 F, 49 F, 192 F, \
      214 F, 31 F, 181 F, 199 F, 106 F, 157 F, 184 F, 84 F, 204 F, 176 F,      \
      115 F, 121 F, 50 F, 45 F, 127 F, 4 F, 150 F, 254 F, 138 F, 236 F, 205 F, \
      93 F, 222 F, 114 F, 67 F, 29 F, 24 F, 72 F, 243 F, 141 F, 128 F, 195 F,  \
      78 F, 66 F, 215 F, 61 F, 156 F, 180 F

static const int perm[] = {
   SIMPLEX_NOISE_P(+0),
   SIMPLEX_NOISE_P(+0)
};

static const int permMod12[] = {
   SIMPLEX_NOISE_P(%12),
   SIMPLEX_NOISE_P(%12)
};

static const f32 grad3[][3] = {
   {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
   {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
   {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
};

#define SQRT_3 (1.7320508075688772f)
#define SQRT_5 (2.23606797749979f)

// Skewing and unskewing factors for 2, 3, and 4 dimensions
#define F2 i2f((0.5f*(SQRT_3-1.0f)))
#define G2 i2f(((3.0f-SQRT_3)/6.0f))

static inline int signbit(f32 i) {
   return i < 0;
}

static inline int fastfloor(f32 x) {
   return f2i(x);
}

static inline f32 dotg2(f32 const *g, f32 x, f32 y)
{
   return g[0]*x + g[1]*y;
}

// 2D simplex noise
f32 snoise2(f32 xin, f32 yin) {
   f32 n0, n1, n2; // Noise contributions from the three corners
   // Skew the input space to determine which simplex cell we're in
   f32 s = f2i((xin+yin)*F2); // Hairy factor for 2D
   int i = fastfloor(xin+s);
   int j = fastfloor(yin+s);
   f32 t = f2i((i+j)*G2);
   f32 X0 = i-t; // Unskew the cell origin back to (x,y) space
   f32 Y0 = j-t;
   f32 x0 = xin-X0; // The x,y distances from the cell origin
   f32 y0 = yin-Y0;
   // For the 2D case, the simplex shape is an equilateral triangle.
   // Determine which simplex we are in.
   int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
   if(x0>y0) {i1=1; j1=0;} // lower triangle, XY order: (0,0)->(1,0)->(1,1)
   else {i1=0; j1=1;}      // upper triangle, YX order: (0,0)->(0,1)->(1,1)
   // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
   // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
   // c = (3-sqrt(3))/6
   f32 x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
   f32 y1 = y0 - j1 + G2;
   f32 x2 = x0 - i2f(1) + 2 * G2; // Offsets for last corner in (x,y) unskewed coords
   f32 y2 = y0 - i2f(1) + 2 * G2;
   // Work out the hashed gradient indices of the three simplex corners
   int ii = i & 255;
   int jj = j & 255;
   int gi0 = permMod12[ii+perm[jj]];
   int gi1 = permMod12[ii+i1+perm[jj+j1]];
   int gi2 = permMod12[ii+1+perm[jj+1]];
   // Calculate the contribution from the three corners
   f32 t0 = i2f(0.5f) - f2i(x0*x0)-f2i(y0*y0);
   if(signbit(t0)) n0 = 0;
   else {
      t0 = f2i(t0*t0);
      n0 = f2i(f2i(t0 * t0) * dotg2(grad3[gi0], x0, y0));  // (x,y) of grad3 used for 2D gradient
   }
   f32 t1 = i2f(0.5f) - f2i(x1*x1)-f2i(y1*y1);
   if(signbit(t1)) n1 = 0;
   else {
      t1 = f2i(t1*t1);
      n1 = f2i(f2i(t1 * t1) * dotg2(grad3[gi1], x1, y1));
   }
   f32 t2 = i2f(0.5f) - f2i(x2*x2)-f2i(y2*y2);
   if(signbit(t2)) n2 = 0;
   else {
      t2 = f2i(t2*t2);
      n2 = f2i(f2i(t2 * t2) * dotg2(grad3[gi2], x2, y2));
   }
   // Add contributions from each corner to get the final noise value.
   // The result is scaled to return values in the interval [-1,1].
   return (70 * (n0 + n1 + n2));
}
#endif

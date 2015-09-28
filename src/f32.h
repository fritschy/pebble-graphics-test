#define Q 10  // works quite well with 10 bits
#define F (1 << Q)
#define M (F - 1)
#define i2f(i) ((f32)((i)*F))
#define f2i(f) ((f32)((f) / F))
#define f2f(f) ((f) / (float)F)
#define ll(f) ((int64_t)(f))
typedef int32_t f32;

static inline f32 sqrtx(f32 f)
{
   f32 v = f / 2;
#define IT() v = (v + i2f(f) / v) / 2
   IT();
   IT();
   IT();
   IT();
   IT();  // 5 is not enough for a nice animation
   IT();
   IT();
   IT();  // 8 looks good
//   IT();
//   IT();  // 10 looks even better
#undef IT
   return v;
}

static inline unsigned GameRand(void)
{
   static unsigned int low = 16180, high = 31415;
   high = (high << 16) + (high >> 16);
   high += low;
   low += high;
   return high;
}

static inline f32 xrand(f32 m) { return f2i(m * (GameRand() & M)); }

f32 snoise2(f32 xin, f32 yin);

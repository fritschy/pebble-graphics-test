#include <pebble.h>
#include "f32.h"

#if 0
#undef APP_LOG
#define APP_LOG(...)
#define START_TIME_MEASURE() {
#define END_TIME_MEASURE(x) }
#define DBG(...)
#else
#define DBG(...) APP_LOG(APP_LOG_LEVEL_DEBUG, __VA_ARGS__)
static unsigned int get_time(void) {
   time_t s;
   uint16_t ms;
   time_ms(&s, &ms);
   return (s & 0xfffff) * 1000 + ms;
}

#define START_TIME_MEASURE() \
   {                         \
   unsigned tm_0 = get_time()
#define END_TIME_MEASURE(x)                                       \
   unsigned tm_1 = get_time();                                    \
   APP_LOG(APP_LOG_LEVEL_DEBUG, "%s: took %dms", x, tm_1 - tm_0); \
   }
#endif

struct App {
   Window *w;
   Animation *a;
   uint8_t color;
};

struct App *g;

static uint8_t *fb;

#define FBW 144
#define FBH 168
#define FBSIZE (FBW * FBH)
#define FBW2 (FBW / 2)
#define FBH2 (FBH / 2)
#define FBC GPoint(FBW2, FBH2)
#define FBBOUNDS GRect(0, 0, FBW, FBH)

/*
static unsigned hash1(unsigned n) {
   return (sin_lookup((n ^ 74623u) & 0xffff) * 27581u) & 0xffff;
}

static unsigned hash2(GPoint p) {
   return hash1(hash1(p.x * 7393u) * hash1(p.y * 17573u) + hash1(1019u * p.x * p.y));
}
*/

static void fbClear(uint8_t col) {
   // clear of framebuffer:
   // memset of framebuffer: ~1.25ms
   // loop of uint32 of framebuffer -O3: ~0.6ms
   // loop of uint32 of framebuffer -Os: ~0.8ms
   // loop of uint8 of framebuffer -O3: ~1.2ms

   uint32_t c = col;
   c |= c << 8;
   c |= c << 16;
   for (int i = 0; i < FBSIZE / 4; i++)
      ((uint32_t *)fb)[i] = c;
}

static void fbSetPixel(GPoint p, uint8_t col) {
   fb[p.x + p.y * FBW] = col;
}

static void fbFillRect(GRect r, uint8_t col) {
   for (int y = r.origin.y, yend = r.origin.y + r.size.h; y < yend; y++) {
      uint8_t *row = fb + y * FBW;
      for (int x = r.origin.x, xend = r.origin.x + r.size.w; x < xend; x++) {
         row[x] = col;
      }
   }
}

/*
static void fbFillRandom(GRect r) {
   for (int y = r.origin.y, yend = r.origin.y + r.size.h; y < yend; y++) {
      uint8_t *row = fb + y * FBW;
      for (int x = r.origin.x, xend = r.origin.x + r.size.w; x < xend; x++) {
         row[x] = (uint8_t)((hash2(GPoint(x, y)) >> 8) & 0xff);
      }
   }
}
*/

static unsigned f0(unsigned x, unsigned y) {
   return sin_lookup(x * x + y * y);
}

static void fbFillSine(GRect r) {
   static int ta = 0;
   static int tdir = 1;
   for (int y = r.origin.y, yend = r.origin.y + r.size.h; y < yend; y++) {
      uint8_t *row = fb + y * FBW;
      for (int x = r.origin.x, xend = r.origin.x + r.size.w; x < xend; x++) {
         uint8_t a = f0(x - FBW2 + ta, y - FBH2 + ta) >> 6;
         uint8_t b = f0(x - FBW2 + 30 + ta, y - FBH2 + 42 - ta) >> 6;
         uint8_t c = f0(x - FBW2 - 51 - ta, y - FBH2 + 124 + ta) >> 6;
         uint8_t z = (a + b + c) >> 8;
         z |= (z<<2) | (z<<4);
         z |= 0xc0;
         row[x] = z;
      }
   }
   ta += tdir;
   if (ta == 0x1f) {
      tdir = -1;
   } else if (ta == -0x1f) {
      tdir = 1;
   }
}

static uint8_t colormap[] = {
   0b11000000,
   0b11010000,
   0b11100000,
   0b11110000,
   0b11110001,
   0b11110010,
   0b11110011,
   0b11100111,
   0b11011011,
   0b11001111
};

#if 1
static f32 fract(f32 f) { return f & M; }
static f32 f32sin(f32 f) {
   return sin_lookup(f << (16 - Q));
}
static f32 mix(f32 x, f32 y, f32 a) {
   return f2i(x * (i2f(1) - a)) + f2i(y * a);
}
static f32 f32floor(f32 f) {
   return f & ~M;
}

typedef struct vec2 {
   f32 x, y;
} vec2;

f32 hash1(f32 n) { return fract(f32sin(n) * 10000); }
f32 hash2(vec2 p) { return fract(10000 * f2i(f32sin(17 * p.x + f2i(p.y * i2f(0.1f))) * (i2f(0.1f) + abs(f32sin(p.y * 13 + p.x))))); }

f32 noise1(f32 x) {
   f32 i = f32floor(x);
   f32 f = fract(x);
   f32 u = f * f * (3.0 - 2.0 * f);
   return mix(hash1(i), hash1(i + 1.0), u);
}


f32 noise2(vec2 x) {
   vec2 i = (vec2){f32floor(x.x), f32floor(x.y)};
   vec2 f = (vec2){fract(x.x), fract(x.y)};

   // Four corners in 2D of a tile
   f32 a = hash2(i);
   f32 b = hash2((vec2){i.x+i2f(1), i.y});
   f32 c = hash2((vec2){i.x, i.y+i2f(1)});
   f32 d = hash2((vec2){i.x+i2f(1), i.y+i2f(1)});

   // Simple 2D lerp using smoothstep envelope between the values.
   // return vec3(mix(mix(a, b, smoothstep(0.0, 1.0, f.x)),
   //			mix(c, d, smoothstep(0.0, 1.0, f.x)),
   //			smoothstep(0.0, 1.0, f.y)));

   // Same code, with the clamps in smoothstep and common subexpressions
   // optimized away.
   vec2 u = (vec2){f2i(f2i(f.x * f.x) * (i2f(3) - 2 * f.x)), f2i(f2i(f.y * f.y) * (i2f(3) - 2 * f.y))};
   return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}
#endif

static void fbFillNoise(GRect r) {
   static int t = 0;
   for (int y = r.origin.y, yend = r.origin.y + r.size.h; y < yend; y++) {
      uint8_t *row = fb + y * FBW;
      for (int x = r.origin.x, xend = r.origin.x + r.size.w; x < xend; x++) {
         f32 n = snoise2((x * 10 - t * 10), (y * 10 - t  * 10));
         n += i2f(1);
         n >>= 1;
         n /= 113; // normalize 1<<10 f32 to 0..9
         if (n > 9) n = 9;
         row[x] = colormap[n];
      }
   }
   t++;
}

typedef struct rgb {
   int32_t r, g, b;
} rgb_t;

#define mix256(x,y,a) ((x * (0xff - a)) / 256 + (y * a) / 255)

static inline rgb_t mixrgb(rgb_t x, rgb_t y, int a) {
   return (rgb_t){
      mix256(x.r, y.r, a),
      mix256(x.g, y.g, a),
      mix256(x.b, y.b, a),
   };
}

static inline GPoint mixpoint(GPoint x, GPoint y, int a) {
   return GPoint(mix256(x.x, x.y, a), mix256(x.y, y.y, a));
}

#ifdef INTRINSICS
//       asm ( assembler template 
//           : output operands                  /* optional */
//           : input operands                   /* optional */
//           : list of clobbered registers      /* optional */
//           );

static inline uint32_t qadd8(uint32_t a, uint32_t b) {
   uint32_t c;
   __asm__("qadd8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b) : );
   return c;
}

static inline uint32_t ssub8(uint32_t a, uint32_t b) {
   uint32_t c;
   __asm__("ssub8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b) : );
   return c;
}

static inline uint32_t shadd8(uint32_t a, uint32_t b) {
   uint32_t c;
   __asm__("shadd8 %0, %1, %2" : "=r"(c) : "r"(a), "r"(b) : );
   return c;
}
#endif

static inline rgb_t to_rgb(uint32_t o) {
   return (rgb_t) { ((o>>4)&3)<<6, ((o>>2)&3)<<6, (o&3)<<6 };
}

static inline uint32_t from_rgb(rgb_t oc) {
   return (uint32_t)((((uint32_t)oc.r >> 6) << 4) | (((uint32_t)oc.g >> 6) << 2) |
                     (((uint32_t)oc.b >> 6) << 0) | 0xc0);
}

#define PM ((uint8_t)0x3f)
static inline rgb_t palette(rgb_t c) {
#ifdef INTRINSICS
   union t {
      uint32_t i;
      rgb_t r;
   };
   union t tc = { .r = c };
   union t tm = { .i = 0x20202020 };
   union t tr;
   tr.i = qadd8(tc.i, tm.i);
   tr.i = tr.i & 0xc0c0c0c0;
   return tr.r;
#else
   return (rgb_t) {
      (c.r + (PM+1) / 2) & ~PM,
      (c.g + (PM+1) / 2) & ~PM,
      (c.b + (PM+1) / 2) & ~PM,
   };
#endif
}

static inline uint32_t select(uint32_t mask, int32_t a, int32_t b) {
   return (mask & a) | (~mask & b);
}

static inline uint32_t lt(int32_t a, int32_t b) {
   return (uint32_t)((a - b) >> (sizeof(a)*8-1));
}

/* #define MIN(a,b) select(lt(a,b), a, b) */
#define MIN(a,b) ((a) < (b) ? (a) : (b))

static inline rgb_t addrgb(rgb_t a, rgb_t b) {
#ifdef INTRINSICS
   union t {
      uint32_t i;
      rgb_t r;
   };
   union t ta = { .r = a };
   union t tb = { .r = b };
   union t tc;
   tc.i = qadd8(ta.i, tb.i);
   return tc.r;
#else
   return (rgb_t) { a.r + b.r, a.g + b.g, a.b + b.b };
#endif
}

static inline rgb_t subrgb(rgb_t a, rgb_t b) {
   return (rgb_t) { a.r - b.r, a.g - b.g, a.b - b.b };
}

static inline rgb_t minrgb(rgb_t a, int i) {
   return (rgb_t) { MIN(a.r, i), MIN(a.g, i), MIN(a.b, i) };
}

static inline rgb_t clamprgb(rgb_t v) {
   return minrgb(v, 0xff);
}

static inline rgb_t muldivrgb(rgb_t a, int m, int d) {
   return (rgb_t) { a.r * m / d, a.g * m / d, a.b * m / d };
}

#define mulshrrgb(a,m,s) ((rgb_t) { (a.r * m) >> s, (a.g * m) >> s, (a.b * m) >> s })

#define assert(x)                                              \
   do {                                                        \
      __typeof__(x) result = (x);                              \
      if (!result) {                                           \
         DBG("Assertion failed: %s, result=%lld (0x%llx)", #x, \
             (long long)result, (long long)result);            \
         *(int *)0 = 42;                                       \
      }                                                        \
   } while (0)

static void xmemset(uint32_t *p, uint32_t v, size_t len) {
   for (size_t i = 0; i < len; i++)
      p[i] = 0;
}

static void fbFillGradient(GPoint p0, GPoint p1, uint8_t c0_, uint8_t c1_) {
   rgb_t c0 = to_rgb(c0_);
   rgb_t c1 = to_rgb(c1_);

   assert(c0_ == from_rgb(to_rgb(c0_)));
   assert(c1_ == from_rgb(to_rgb(c1_)));

   static int frame;
   frame++;

#define FLOYD_STEINBERG
#ifdef FLOYD_STEINBERG
   // width+2 items to eliminate extra branche sin the inner loop and...
   static rgb_t errors[2*(FBW+2)];
   // init error[0] to zero each time ...
   xmemset((void*) errors, 0, sizeof(errors) / 2 / 4);
   // ... set the error buffers 1 item ahead to eliminate extra branches in the inner loop
   rgb_t *error[2] = { errors+1, errors+(FBW+2)+1 };

   for (int y = 0; y < FBH; y++) {
      rgb_t const c = mixrgb(c0, c1, y * 256 / 168); // this is our target color
      rgb_t next = { 0,0,0 };                        // holds the "ahead" error
      for (int x = 0; x < FBW; x++) {
         rgb_t const oldp = addrgb(c, addrgb(next, error[0][x]));
         error[0][x] = (rgb_t) { 0, 0, 0 };          // set to zero for next row

         rgb_t const newp = palette(oldp);
         fb[y*FBW+x] = from_rgb(clamprgb(newp));
         rgb_t const quante = subrgb(oldp, newp);

/* #define SIERRA_LITE */
#ifdef SIERRA_LITE
         next = mulshrrgb(quante, 2, 2);
         error[1][x - 1] = addrgb(error[1][x - 1], mulshrrgb(quante, 1, 2)); // accumulate
         error[1][x    ] = mulshrrgb(quante, 1, 2);                          // overwrite
#else // FLOYD_STEINBERG
         next = mulshrrgb(quante, 7, 4);
         error[1][x - 1] = addrgb(error[1][x - 1], mulshrrgb(quante, 3, 4)); // accumulate
         error[1][x    ] = addrgb(error[1][x    ], mulshrrgb(quante, 5, 4)); // accumulate
         error[1][x + 1] = mulshrrgb(quante, 1, 4);                          // overwrite
#endif
      }

      // swap error buffers
      rgb_t *es = error[0];
      error[0] = error[1];
      error[1] = es;
   }
#else
   int32_t coeff[8][8] =
   {
      {  1, 49, 13, 61,  4, 52, 16, 64 },
      { 33, 17, 45, 29, 36, 20, 48, 32 },
      {  9, 57,  5, 53, 12, 60,  8, 56 },
      { 41, 25, 37, 21, 44, 28, 40, 24 },
      {  3, 51, 15, 63,  2, 50, 14, 62 },
      { 35, 19, 47, 31, 34, 18, 46, 30 },
      { 11, 59,  7, 55, 10, 58,  6, 54 },
      { 43, 27, 39, 23, 42, 26, 38, 22 }
   };

   for (int y = 0; y < FBH; y++) {
      rgb_t c = mixrgb(c0, c1, y * 256 / 168);
      for (int x = 0; x != FBW; x++) {
         rgb_t oldp = addrgb(c, mulshrrgb(c, coeff[y&7][x&7], 6));
         rgb_t newp = clamprgb(palette(oldp));
         fb[y*FBW+x] = from_rgb(newp);
      }
   }
#endif
}

static void draw(void) {
   START_TIME_MEASURE();
   fbClear(g->color | 0xc0);
   fbFillGradient(GPoint(0,0), GPoint(FBW, FBH), 0xf0, 0xff); // red -> blue
   END_TIME_MEASURE("drawing");
}

static void update(Layer *layer, GContext *ctx) {
   GBitmap *bmp = graphics_capture_frame_buffer(ctx);
   fb = gbitmap_get_data(bmp);

   draw();

   graphics_release_frame_buffer(ctx, bmp);
   fb = NULL;
}

static void a_setup(Animation *a) {}

static void a_update(Animation *a, AnimationProgress t) {
   layer_mark_dirty(window_get_root_layer(g->w));
}

static void a_teardown(Animation *a) {}

static AnimationImplementation a_impl = {
   .setup = a_setup, .update = a_update, .teardown = a_teardown};

static void window_load(Window *w) {
   struct App *a = g;
   layer_set_update_proc(window_get_root_layer(w), update);
   a->a = animation_create();
   animation_set_implementation(a->a, &a_impl);
   animation_set_duration(a->a, ANIMATION_DURATION_INFINITE);
   animation_schedule(a->a);
}

static void window_unload(Window *w) {
   animation_destroy(g->a);
   g->a = NULL;
}

static void init(struct App *a) {
   g = a;
   a->w = window_create();
   window_set_user_data(a->w, a);
   window_set_window_handlers(a->w,
                              (WindowHandlers){
                                 .load = window_load, .unload = window_unload,
                              });
   window_stack_push(a->w, false);
}

static void fini(struct App *a) {
   window_destroy(a->w);
   a->w = NULL;
   g = NULL;
}

int main(void) {
   struct App a;
   init(&a);
   app_event_loop();
   fini(&a);
   return 0;
}

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;

typedef struct {
  u8 l : 4;
} r4;

typedef union {
  struct {
    u8 l : 4;
    u8 m : 4;
  };
  u8 x;
} r8;

typedef struct {
  r4 a;
  r4 x;
  r8 b;
  r8 sb;
  bool c;
  bool ime;
  bool ifa;
  bool ifb;
  r4 re;
} rfile;

rfile r;
r4 ram[256];

#define A r.a.l
#define X r.x.l
#define B r.b.x
#define BL r.b.l
#define BM r.b.m
#define SB r.sb.x
#define SBL r.sb.l
#define SBM r.sb.m
#define C r.c
#define IME r.ime
#define IFA r.ifa
#define IFB r.ifb
#define RE r.re.l

#define RAM(i) ram[(i)&0xff].l

#define BIT(i) (1 << (i))

#define RAM_BIT_RESET(a, b) (RAM(a) &= ~BIT(b))
#define RAM_BIT_SET(a, b) (RAM(a) |= BIT(b))
#define RAM_BIT_TEST(a, b) (RAM(a) & BIT(b))

#define SWAP(a, b) \
  do {             \
    u8 c = a;      \
    a = b;         \
    b = c;         \
  } while (0)

#define SPIN(n)

u8 readIO(u8 port);
void writeIO(u8 port, u8 value);
void sync(void);
void fatalError(void);
void notImpl(u8 pu, u8 pl);

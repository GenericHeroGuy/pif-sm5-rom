#include "cmodel.h"

#include <stdio.h>
#include <stdlib.h>

// C model reference implementation of SM5 PIF ROM
// Goals:
// - reproduce high-level behavior including port I/O and RAM usage
// - be readable
// - be executable
// Non-goals:
// - model every register and memory state transition
// - model timing

FILE* input;

void (*continuation)(void);

// The only non-code data in the ROM, taken from 04:00.
const u8 rom[] = {
    0x19, 0x4a, 0xf1, 0x88, 0xb5, 0x5a, 0x71, 0xc3,
    0xde, 0x61, 0x10, 0xed, 0x9e, 0x8c, 0x3c, 0x2b,
};

void sub_030(void);
void sub_10A(void);
void sub_10C(void);
bool sub_10E(void);
void sub_110(void);
void sub_116(void);
void sub_339(void);
void sub_40E(void);
void sub_414(void);
void sub_500(void);
void sub_52D(void);
void sub_700(void);
void sub_90D(void);
bool sub_C00(void);
bool sub_C10(void);
void sub_D31(void);
void sub_E00(void);
void sub_E15(void);
void sub_F00(void);
void sub_F1B(void);

// 00:00
void sub_000(void) {
  writeIO(0x5, 1);
  writeIO(0xe, 1);

  sub_110();

  B = 0x34;
  sub_116();

  B = 0x5e;
  sub_10E();
  switch (RAM(0x5e)) {
    case 0x1:
      RAM(0x5e) = 0x4;
      break;
    case 0x9:
      RAM(0x5e) = 0xc;
      break;
    default:
      sub_10C();
      break;  // fatal error
  }

  B = 0x80;
  do {
    sub_116();
  } while (++BM);

  B = 0x1a;
  while (!sub_10E())
    ;  // stops when B wraps to 0x10

  BL = 0xa;
  sub_10A();
  BL = 0xa;
  sub_10A();

  A = RAM(0x5e);
  RAM(0x5e) &= ~BIT(1);
  RAM(0x5e) &= ~BIT(3);
  SWAP(A, RAM(0x1b));

  B = 0x1a;
  C = 0;

  sub_030();
}

// 00:30
void sub_030(void) {
  B = 0xfe;
  RAM(0xfe) |= BIT(3);
  IME = 1;
  continuation = sub_500;
}

// 01:02
void sub_102(void) {
  sub_40E();
}

// 01:04
bool sub_104(void) {
  return sub_C10();
}

// 01:06
void sub_106(void) {
  sub_90D();
}

// 01:0A
void sub_10A(void) {
  sub_E15();
}

// 01:0C
void sub_10C(void) {
  sub_339();
}

// 01:0E
bool sub_10E(void) {
  return sub_C00();
}

// 01:10
void sub_110(void) {
  sub_D31();
}

// 01:12
// write 0xfb at B and zero rest of segment
void sub_112(void) {
  A = 0xf;
  SWAP(A, RAM(B));
  A = 0xb;
  SWAP(A, RAM(B));
  sub_116();
}

// 01:16
// zero memory from B to end of segment
void sub_116(void) {
  do {
    A = 0;
    SWAP(A, RAM(B));
  } while (++BL);
}

// 01:1A
// fill [0x40..0x45] with 8
void sub_11A(void) {
  B = 0x45;
  do {
    A = 8;
    SWAP(A, RAM(B));
  } while (BL--);
}

// 01:20
void sub_120(void) {
  writeIO(0x5, A);
  for (A = 0xb; A; ++A)
    ;  // spin
  writeIO(0x5, 0x1);
  for (A = 0xc; A; ++A)
    ;  // spin
}

// 01:2A
// read bit from CIC into C
void sub_12A(void) {
  writeIO(5, 3);
  for (A = 0xb; A; ++A)
    ;  // spin
  C = readIO(5) & BIT(3);
  writeIO(5, 1);
  for (A = 0xc; A; ++A)
    ;  // spin
}

// 03:0B
void sub_30B(void) {
  notImpl(0x03, 0x0B);
}

// 03:39
// disable interrupts and strobe R8 forever
void sub_339(void) {
  IME = 0;
  do {
    writeIO(0x8, A);
    A = ~A;
    fatalError();
  } while (1);
}

// 04:0E
// swap internal and external memory
void sub_40E(void) {
  B = 0xcb;
  sub_414();  // swap [0x1b..0x1f] <-> [0xcb..0xcf]
  B = 0xe4;
  sub_414();  // swap [0x34..0x3f] <-> [0xe4..0xef]
}

// 04:14
void sub_414(void) {
  do {
    SWAP(RAM(B), RAM(B - 0xB0));
  } while (++BL);
  B = 0xfe;
}

// 05:00
void sub_500(void) {
  IME = 0;
  sub_102();
  sub_116();  // zero [0xfe..0xff]
  BL = 0xe;
  IME = 1;

  // wait for rom lockout bit of PIF status byte to become set
  do {
    readCommand();
  } while (!(RAM(0xfe) & BIT(0)));
  IME = 0;
  A = 0x1;
  BL = 0x6;
  writeIO(0x6, 0x1);
  BL = 0x2;
  A = 0x1;
  writeIO(0x2, 0x1);
  sub_11A();
  B = 0xfe;
  IME = 1;

  // wait for checksum verification bit
  do {
    readCommand();
  } while (!(RAM(0xfe) & BIT(1)));
  IME = 0;
  sub_102();
  RAM(0xfe) |= BIT(3);
  IME = 1;

  // wait for clear pif ram bit
  do {
    readCommand();
  } while (!(RAM(0xfe) & BIT(2)));
  IME = 0;
  A = 0x0;
  SWAP(A, RAM(B));
  if (C == 0)
    sub_E00();
  B = 0x34;

  // compare checksum
  do {
    A = 0x0;
    SWAP(A, RAM(B));
    BM ^= 0x1;
    if (A != RAM(B)) {
      sub_10C();
    }
    BM = 0x3;
  } while (++BL);

  B = 0x4a;
  sub_112();
  IME = 1;

  sub_52D();
}

// 05:2D
void sub_52D(void) {
  B = 0xff;

  readCommand();
  if (!(RAM(0xff) & BIT(3))) {
    // handle command other than 'terminate boot process'
    continuation = sub_700;
    return;
  }
  IME = 0;
  BL = 0xe;
  sub_116();  // zero [0xfe..0xff]
  B = 0x5e;
  A = 0x5;
  writeIO(0xe, 0x5);
  RAM(0x5e) |= BIT(3);
  IFB = 0;

  continuation = sub_30B;
}

// 06:29
bool sub_629(void) {
  SWAP(A, RAM(B));
  A += 0x1;
  if (A)
    goto loc_632;
  SWAP(A, RAM(B));
  if (BL--)
    SWAP(A, RAM(B));
  A += 0x1;
  if (A)
    goto loc_634;
  SWAP(A, RAM(B));
  if (BL--)
    return true;

loc_632:
  SWAP(A, RAM(B));
  return false;

loc_634:
  SWAP(A, RAM(B));
  if (++BL)
    return false;

  abort();
}

// 07:00
void sub_700(void) {
  notImpl(0x07, 0x00);
}

// 09:0D
void sub_90D(void) {
  // spin
  A = 0;
  u8 X = 0;
  do {
    while (++X & 0xf)
      ;
  } while (++A);
}

// 0C:00
// read nibble from CIC into [B]
// increments BL, returns true on carry
bool sub_C00(void) {
  A = 0xf;
  SWAP(A, RAM(B));
  sub_12A();
  if (C != 1)
    RAM(B) &= ~BIT(3);
  sub_12A();
  if (C != 1)
    RAM(B) &= ~BIT(2);
  sub_12A();
  if (C != 1)
    RAM(B) &= ~BIT(1);
  sub_12A();
  if (C != 1)
    RAM(B) &= ~BIT(0);
  C = 0;
  return (++BL == 0);
}

// 0C:10
bool sub_C10(void) {
  A = 0x3;
  if (!(RAM(B) & BIT(3)))
    A = 0x2;
  sub_120();
  A = 0x3;
  if (!(RAM(B) & BIT(2)))
    A = 0x2;
  sub_120();
  A = 0x3;
  if (!(RAM(B) & BIT(1)))
    A = 0x2;
  sub_120();
  A = 0x3;
  if (!(RAM(B) & BIT(0)))
    A = 0x2;
  sub_120();
  return (++BL == 0);
}

// 0D:31
void sub_D31(void) {
  B = 0x56;
  SWAP(B, SB);
}

// 0D:35
// increment B and wrap to 0x80 on overflow
void sub_D35(void) {
  if (++B == 0)
    B = 0x80;
}

// 0E:00
void sub_E00(void) {
  B = 0x1b;
  RAM(0x1b) |= BIT(1);
  sub_F1B();
  BL = 0x5;
  A = 0x3;
  writeIO(0x5, 0x3);
  sub_106();
  A = 0x1;
  writeIO(0x5, 0x1);
  B = 0x20;

  while (!sub_10E())
    ;  // stops when B wraps to 0x20

  sub_10A();
  sub_10A();
  sub_10A();
  sub_10A();

  sub_F00();
}

// 0E:15
// encode CIC seed or checksum (one round)
void sub_E15(void) {
  A = 0xf;
  do {
    A = ~A;
    A += RAM(B);
    SWAP(A, RAM(B));
  } while (++BL);
}

// 0F:00
void sub_F00(void) {
  B = 0x60;
  A = 0x0;
  SWAP(A, RAM(0x60));
  SWAP(B, SB);
  B = 0x62;
  do {
    SWAP(B, SB);
    A = RAM(B);
    A += 0x1;
    if (A)
      SWAP(A, RAM(B));
    SWAP(B, SB);
    u8 byte = rom[A];
    A = byte & 0xf;
    u8 X = byte >> 4;
    SWAP(A, RAM(B));
    BM ^= 0x1;
    SWAP(A, X);
    SWAP(A, RAM(B));
    BM ^= 0x1;
  } while (++BL);
  BL = 0x1;
  sub_104();
  B = 0x71;
  sub_104();
  sub_D31();
}

// 0F:1B
void sub_F1B(void) {
  B = 0x69;
  A = 0x1;
  writeIO(0x9, 0x1);

  do {
    sub_629();
    BL = 0x9;
  } while (!(readIO(9) & BIT(3)));
  A = 0x0;
  writeIO(0x9, 0x0);

  u8 X = {0};  // todo: confirm the value on entry is unimportant
  SWAP(A, RAM(B));
  BL--;
  SWAP(A, X);
  SWAP(A, RAM(B));
  BL = 0x1;
  SWAP(A, RAM(B));
  BM ^= 0x1;
  SWAP(A, X);
  SWAP(A, RAM(B));
  BM ^= 0x1;
  // todo: confirm this is equivalent to the above
  // RAM(0x61) = RAM(0x68);
  // RAM(0x71) = RAM(0x69);
  // RAM(0x68) = X;
  // RAM(0x69) = 0;
}

// end PIF ROM

int scanValue(void) {
  // remove comments before next token
  int num;
  do {
    num = 0;
    fscanf(input, " #%n%*[^\n]", &num);
  } while (num > 0);

  int value;
  if (1 != fscanf(input, "%i", &value)) {
    printf("scanf error\n");
    exit(3);
  }
  return value;
}

u8 readIO(u8 port) {
  printf("r %x\n", port);
  int value = scanValue();
  printf("  %x\n", value);
  return value & 0xf;
}

void writeIO(u8 port, u8 value) {
  printf("w %x %x\n", port, value);
}

void readCommand(void) {
  printf("r cmd\n");
  int value = scanValue();
  printf("  %x\n", value);
  RAM(0xfe) = value >> 4;
  RAM(0xff) = value & 0xf;
}

void fatalError(void) {
  printf("fatal error\n");
  exit(1);
}

void notImpl(u8 pu, u8 pl) {
  printf("not impl %x:%02x\n", pu, pl);
  exit(2);
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    input = fopen(argv[1], "r");
  } else {
    input = stdin;
  }
  continuation = sub_000;
  while (continuation) {
    (*continuation)();
  }
}

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

void sub_10A(void);
void sub_10C(void);
bool sub_10E(void);
void sub_110(void);
void sub_116(void);
void sub_339(void);
void sub_414(void);
void sub_D31(void);
bool sub_C00(void);
void sub_E15(void);

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

  notImpl(0x00, 0x30);
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

// end PIF ROM

u8 readIO(u8 port) {
  printf("r %x\n", port);
  int value;
  scanf("%i", &value);
  return value & 0xf;
}

void writeIO(u8 port, u8 value) {
  printf("w %x %x\n", port, value);
}

void fatalError(void) {
  printf("fatal error\n");
  exit(1);
}

void notImpl(u8 pu, u8 pl) {
  printf("not impl %x:%02x\n", pu, pl);
  exit(2);
}

int main(void) {
  sub_000();
}

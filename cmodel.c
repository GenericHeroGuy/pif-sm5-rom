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

typedef void (*continuation_t)(void);

void (*continuation)(void);

bool IFA = 0;

void checkInterrupt(void);

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
void sub_22C(void);
void sub_23B(void);
void sub_306(void);
void sub_339(void);
void sub_40E(void);
void sub_414(void);
void sub_500(void);
void sub_52D(void);
void sub_600(void);
bool sub_629(void);
void sub_700(void);
void sub_709(void);
void sub_713(void);
void sub_71B(void);
void sub_900(void);
void sub_909(void);
void sub_90D(void);
bool sub_916(void);
void sub_91F(void);
void sub_922(void);
void sub_92D(void);
bool sub_C00(void);
bool sub_C10(void);
void sub_C26(void);
void sub_C32(void);
void sub_D00(void);
void sub_D1B(void);
void sub_D31(void);
void sub_D35(void);
void sub_E00(void);
void sub_E15(void);
void sub_E1B(void);
void sub_F00(void);
void sub_F1B(void);
void sub_F2F(void);

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

// 01:00
bool sub_100(void) {
  return sub_629();
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

// 01:08
void sub_108(void) {
  sub_E1B();
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
  RAM(B) = 0xf;
  ++BL;
  RAM(B) = 0xb;
  ++BL;
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

// 02:00
void sub_200(void) {
  SWAP(B, SB);
  SWAP(A, RAM(B));
  ++BL;

  if (!(readIO(BL) & BIT(3)))
    goto loc_21D;
  if (!(readIO(BL) & BIT(2)))
    goto loc_239;
  B = 0xff;
  readCommand();
  if (!(RAM(0xff) & BIT(1))) {
    sub_713();
    return;
  }
  B = 0x5e;
  if (!(RAM(0x5e) & BIT(3)))
    goto loc_239;
  sub_709();
  return;

loc_21D:
  sub_306();
  B = 0xff;
  readCommand();
  if (!(RAM(0xff) & BIT(0)))
    goto loc_237;
  RAM(B) &= ~BIT(0);
  sub_F2F();
  B = 0x10;
  SWAP(B, SB);
  sub_11A();
  sub_92D();

  sub_22C();
  return;

loc_237:
  BM = 0x5;

  sub_23B();
  return;

loc_239:
  sub_306();

  sub_23B();
}

// 02:04
void sub_204(void) {
  SWAP(B, SB);
  SWAP(A, RAM(B));
  BL = 0xe;
  RAM(B) &= ~BIT(3);
  A = 0x1;
  writeIO(0xe, 0x1);
  BL = 0x8;
  A = 0x2;
  writeIO(0x8, 0x2);

  sub_23B();
}

// 02:2C
void sub_22C(void) {
  B = 0x59;
  C = 1;
  if (!(RAM(0x59) & BIT(0)))
    C = 0;
  if (BL--)
    SWAP(A, RAM(B));
  u8 X = 0;  // todo: is this value important?
  if (BL--)
    SWAP(A, X);
  sub_C32();

  sub_23B();
}

// 02:3B
void sub_23B(void) {
  BL = 0x6;
  SWAP(A, RAM(B));
  SWAP(B, SB);
  IME = 1;
}

// 03:06
void sub_306(void) {
  BL = 0xe;
  A = 0x1;
  writeIO(0xe, 0x1);

  // todo: should we do anything for halt/standby?
  // HALT = 1;

  if (RAM(B) & BIT(3)) {
    A = 0x5;
    writeIO(0xe, 0x5);
  }
}

// 03:0B
void sub_30B(void) {
  IME = 1;
  static bool checkOnce;  // todo: remove, for testing purposes
  if (!checkOnce) {
    checkOnce = true;
    IFA = 1;
    checkInterrupt();
  }

loc_30C:
  B = 0x5e;
  if (!(RAM(0x5e) & BIT(3))) {
    continuation = sub_600;
    return;
  }
  if (!(RAM(0x5e) & BIT(1)))
    goto loc_316;
  RAM(B) &= ~BIT(1);
  continuation = sub_D00;
  return;

loc_316:
  A = 0x2;
  sub_120();
  A = 0x2;
  sub_120();
  BM = 0x6;
  sub_108();
  sub_108();
  sub_108();
  BM = 0x7;
  sub_108();
  sub_108();
  sub_108();
  B = 0x77;
  A = RAM(0x77);
  if (A)
    A += 0xf;
  A += 0x1;
  if (A)
    SWAP(A, BL);

loc_329:
  BM = 0x6;
  A = 0x3;
  if (!(RAM(B) & BIT(0)))
    A = 0x2;
  sub_120();
  BM = 0x7;
  sub_12A();
  if (C == 0)
    goto loc_337;
  if (!(RAM(B) & BIT(0))) {
    sub_339();
    return;
  }

loc_334:
  if (++BL)
    goto loc_329;
  goto loc_30C;

loc_337:
  if (!(RAM(B) & BIT(0)))
    goto loc_334;

  sub_339();
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

// 06:00
void sub_600(void) {
  A = 0x3;
  sub_120();
  A = 0x3;
  sub_120();
  BL = 0x5;
  A = 0x3;
  writeIO(0x5, 0x3);
  B = 0x0c;
  sub_116();

loc_60A:
  B = 0xfe;
  RAM(0xfe) |= BIT(3);
  B = 0x05;
  if (!(readIO(0x5) & BIT(3)))
    goto loc_618;
  BL = 0xf;
  if (!sub_100() || !sub_100())
    goto loc_60A;
  sub_10C();
  BL = 0x5;

loc_618:
  A = 0x1;
  writeIO(0x5, 0x1);
  BL = 0x8;
  A = 0x0;

  while (!(readIO(0x8) & BIT(3)))
    ;

  IME = 0;
  BL = 0x6;
  writeIO(0x6, A);
  A = 0x9;
  BL = 0x8;
  writeIO(0x8, 0x9);
  A = 0x8;
  writeIO(0x8, 0x8);
  C = 1;

  continuation = sub_030;
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
  BM = 0x4;
  if (!sub_100() || !sub_100() || !sub_100()) {
    continuation = sub_52D;
    return;
  }

  sub_10C();

  abort();
}

// 07:09
void sub_709(void) {
  RAM(0xff) &= ~BIT(1);
  RAM(0x5e) |= BIT(1);
  B = 0x56;
  SWAP(A, RAM(0x56));
  SWAP(B, SB);
}

// 07:13
void sub_713(void) {
  sub_F2F();
  BL = 0xa;
  A = 0x4;
  goto loc_71F;

loc_718:
  BL = 0x2;
  A = 0x1;
  writeIO(0x2, 0x1);

loc_71B:
  BL = 0xa;
  A = readIO(0xa);
  // todo: read upper half into X?
  A += 0xf;
  if (A == 0xf)
    goto loc_738;

loc_71F:
  writeIO(0xa, A);
  SWAP(A, BL);
  BM = 0x4;
  if (!(RAM(B) & BIT(0)))
    goto loc_727;
  sub_C26();
  goto loc_71B;

loc_727:
  if (!(RAM(B) & BIT(3)))
    goto loc_72A;
  goto loc_71B;

loc_72A:
  BM = 0x1;
  sub_C32();
  B = 0x22;
  if (!sub_916())
    goto loc_733;
  goto loc_71B;

loc_733:
  sub_91F();
  BL = 0x3;
  goto loc_818;

loc_738:
  BM = 0x5;
  sub_306();

  sub_22C();
  return;

loc_800:
  SWAP(A, RAM(B));
  if (BL--)
    SWAP(A, RAM(B));
  A += 0xf;
  if (A == 0xf)
    goto loc_81D;
  SWAP(A, RAM(B));
  if (!++BL) abort();

loc_805:
  if (!(readIO(BL) & BIT(2)))
    goto loc_423;
  if (!(readIO(BL) & BIT(3)))
    goto loc_805;
  SWAP(B, SB);
  A = RAM(B);
  if (++BL)
    writeIO(0x0, A);
  A = RAM(B);
  writeIO(0x0, A);
  if (++BL)
    goto loc_817;
  SWAP(A, BM);
  A += 0x1;
  if (A)
    goto loc_816;
  A = 0x8;

loc_816:
  SWAP(A, BM);

loc_817:
  SWAP(B, SB);

loc_818:
  SWAP(A, RAM(B));
  A += 0xf;
  if (A == 0xf)
    goto loc_800;
  SWAP(A, RAM(B));
  goto loc_805;

loc_81D:
  sub_900();
  B = 0x33;
  goto loc_838;

loc_822:
  SWAP(A, RAM(B));
  if (BL--)
    SWAP(A, RAM(B));
  A += 0xf;
  if (A == 0xf)
    goto loc_718;
  SWAP(A, RAM(B));
  if (!++BL) abort();

loc_828:
  if (!(readIO(BL) & BIT(2)))
    goto loc_423;
  if (!(readIO(BL) & BIT(3)))
    goto loc_828;
  A = readIO(0x1);
  SWAP(B, SB);
  SWAP(A, RAM(B));
  if (++BL)
    A = readIO(0x1);
  SWAP(A, RAM(B));
  if (++BL)
    goto loc_837;
  SWAP(A, BM);
  BM = 0x8;
  A += 0x1;
  if (A)
    SWAP(A, BM);

loc_837:
  SWAP(B, SB);

loc_838:
  SWAP(A, RAM(B));
  A += 0xf;
  if (A == 0xf)
    goto loc_822;
  SWAP(A, RAM(B));
  goto loc_828;

// 04:23
loc_423:
  BL = 0x2;
  A = 0x0;
  writeIO(0x2, 0x0);
  A = 0x1;
  writeIO(0x2, 0x1);
  BL = 0xa;
  A = readIO(0xa);
  // todo: read upper half into X?
  SWAP(A, BL);
  BM = 0x1;
  sub_C32();
  BL = 0x4;
  if (!(readIO(0x4) & BIT(3)))
    goto loc_433;
  A = 0x8;
  goto loc_434;

loc_433:
  A = 0x4;

loc_434:
  SWAP(B, SB);
  if (++BL)
    sub_D35();
  A += RAM(B);
  SWAP(A, RAM(B));
  SWAP(B, SB);
  A = 0x0;
  writeIO(0x4, 0x0);

  goto loc_71B;
}

// 09:00
void sub_900(void) {
  B = 0xff;
  if (!(RAM(0xff) & BIT(2))) {
    sub_909();
    return;
  }
  if (!(RAM(0xff) & BIT(3)))
    sub_909();
  sub_106();
}

// 09:09
void sub_909(void) {
  BL = 0x2;
  A = 0x2;
  writeIO(0x2, 0x2);
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

// 09:16
bool sub_916(void) {
  SWAP(B, SB);
  if (!(RAM(B) & BIT(0x3)))
    goto loc_91A;
  return true;

loc_91A:
  if (!(RAM(B) & BIT(0x2))) {
    sub_922();
    return false;
  }
  sub_C26();
  return true;
}

// 09:1F
void sub_91F(void) {
  SWAP(B, SB);
  RAM(B) &= ~BIT(0x3);
  RAM(B) &= ~BIT(0x2);

  sub_922();
}

// 09:22
void sub_922(void) {
  A = RAM(B);
  SWAP(B, SB);
  SWAP(A, RAM(B));
  if (++BL)
    SWAP(B, SB);
  if (++BL)
    A = RAM(B);
  sub_D35();
  SWAP(B, SB);
  SWAP(A, RAM(B));
  BM ^= 0x1;
  if (BL--)
    return;

  abort();
}

// 09:2D
void sub_92D(void) {
  B = 0x80;

loc_92F:
  do {
    A = 0xf;
    if (0xf != RAM(B))
      goto loc_A0E;
    ++BL;
    A = RAM(B);
    A += 0x1;
    if (A)
      goto loc_A03;
  } while (++BL);

  SWAP(A, BM);
  A += 0x1;

  SWAP(A, BM);
  goto loc_92F;

loc_A03:
  A += 0x1;
  if (!A)
    return;

  A += 0x1;
  if (A)
    goto loc_A1F;
  SWAP(B, SB);
  BM = 0x4;
  RAM(B) |= BIT(0);
  BM = 0x1;
  SWAP(B, SB);
  goto loc_A14;

loc_A0E:
  A = 0x0;
  if (0 != RAM(B))
    goto loc_A20;
  ++BL;
  if (0 != RAM(B))
    goto loc_A1F;

loc_A14:
  if (++BL)
    goto loc_A1B;
  SWAP(A, BM);
  A += 0x1;
  if (!A)
    return;

  SWAP(A, BM);

loc_A1B:
  SWAP(B, SB);
  if (++BL)
    goto loc_B33;

loc_A1F:
  if (!BL--)
    goto loc_A21;

loc_A20:
  SWAP(A, BM);
loc_A21:
  u8 X = A;
  SWAP(A, BM);
  SWAP(A, X);
  SWAP(B, SB);
  SWAP(A, RAM(B));
  BM ^= 0x1;
  SWAP(B, SB);
  SWAP(A, BL);
  X = A;
  SWAP(A, BL);
  SWAP(A, X);
  SWAP(B, SB);
  SWAP(A, RAM(B));
  BM ^= 0x1;
  if (++BL)
    SWAP(B, SB);
  A = RAM(B);
  RAM(B) &= ~BIT(2);
  RAM(B) &= ~BIT(3);
  SWAP(A, RAM(B));
  if (++BL)
    SWAP(A, X);
  A = RAM(B);
  SWAP(B, SB);
  BM = 0x4;
  if (BL--)
    RAM(B) &= ~BIT(3);
  if (++BL)
    BM = 0x1;
  SWAP(A, X);

  SWAP(A, RAM(B));
  BM ^= 0x1;
  SWAP(A, X);
  SWAP(A, RAM(B));
  BM ^= 0x1;
  SWAP(B, SB);
  if (++BL)
    goto loc_B0B;
  SWAP(A, BM);
  A += 0x1;
  if (A)
    goto loc_B0A;
  goto loc_B3A;

loc_B0A:
  SWAP(A, BM);

loc_B0B:
  A = RAM(B);
  RAM(B) &= ~BIT(3);
  RAM(B) &= ~BIT(2);
  SWAP(A, RAM(B));
  if (++BL)
    SWAP(A, X);
  A = RAM(B);
  SWAP(B, SB);
  BM = 0x0;
  C = A + RAM(B) >= 0x10;
  A = A + RAM(B);
  SWAP(A, RAM(B));
  BM ^= 0x1;
  SWAP(A, X);
  bool Cy = A + RAM(B) + C >= 0x10;
  A = A + RAM(B) + C;
  C = Cy;
  if (!Cy) {
    SWAP(A, RAM(B));
    BM ^= 0x1;
  }
  A = RAM(B);
  C = A + RAM(B) >= 0x10;
  A = A + RAM(B);
  SWAP(A, RAM(B));
  BM ^= 0x1;
  A = RAM(B);
  Cy = A + RAM(B) + C >= 0x10;
  A = A + RAM(B) + C;
  C = Cy;
  if (!Cy) {
    SWAP(A, RAM(B));
    BM ^= 0x1;
  }
  SWAP(B, SB);
  SWAP(A, BL);
  SWAP(B, SB);
  C = A + RAM(B) + 1 >= 0x10;
  A = A + RAM(B) + 1;
  BM = 0x1;
  SWAP(B, SB);
  SWAP(A, BL);
  SWAP(A, BM);
  SWAP(B, SB);
  Cy = A + RAM(B) + C >= 0x10;
  A = A + RAM(B) + C;
  C = Cy;
  if (!Cy)
    goto loc_B30;
  goto loc_B3B;

loc_B30:
  SWAP(B, SB);
  SWAP(A, BM);
  SWAP(B, SB);

loc_B33:
  A = 0x5;
  if (5 == BL)
    return;
  SWAP(B, SB);
  goto loc_92F;

loc_B3A:
  SWAP(B, SB);

loc_B3B:
  if (BL--)
    BM = 0x4;
  RAM(B) &= ~BIT(0);
  RAM(B) |= BIT(3);
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

// 0C:26
void sub_C26(void) {
  while (!(readIO(0x3) & BIT(3)))
    writeIO(0x4, 0x0);

  writeIO(0x2, 0x3);
  A = 0x1;
  writeIO(0x2, 0x1);
  BL = 0x3;

  while (!(readIO(0x3) & BIT(3)))
    ;
}

// 0C:32
void sub_C32(void) {
  A = RAM(B);
  BM ^= 0x1;
  SWAP(B, SB);
  SWAP(A, BM);
  SWAP(B, SB);
  A = RAM(B);
  BM ^= 0x1;
  SWAP(B, SB);
  SWAP(A, BL);
  SWAP(B, SB);
}

// 0D:00
void sub_D00(void) {
  A = 0x3;
  sub_120();
  A = 0x2;
  sub_120();
  B = 0x0a;
  sub_10E();
  sub_10E();
  B = 0xdd;
  sub_D1B();
  B = 0x0b;

  while (!sub_100())
    ;
  sub_12A();
  B = 0xdf;
  sub_D1B();
  BM = 0x5;
  sub_306();
  sub_110();

  continuation = sub_30B;
}

void sub_D1B(void) {
  SWAP(B, SB);
  B = 0xe0;

loc_D1E:
  SWAP(B, SB);
  A = RAM(B);
  A += 0xf;
  if (A == 0xf)
    return;
  SWAP(A, RAM(B));
  A = 0xd;
  if (0xd != BL)
    goto loc_D2B;
  SWAP(B, SB);
  sub_104();
  if (!sub_104())
    goto loc_D1E;
  goto loc_D2F;

loc_D2B:
  SWAP(B, SB);
  sub_10E();
  if (!sub_10E())
    goto loc_D1E;

loc_D2F:
  BM = 0xf;
  goto loc_D1E;
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

// 0E:1B
void sub_E1B(void) {
  BL = 0xf;
  A = RAM(B);

  do {
    u8 X = A;
    BL = 0x1;
    A = A + RAM(B) + 1;
    SWAP(A, RAM(B));
    A = RAM(B);
    ++BL;
    A = A + RAM(B) + 1;
    A = ~A;
    SWAP(A, RAM(B));
    ++BL;
    bool Cy = A + RAM(B) + 1 >= 0x10;
    A = A + RAM(B) + 1;
    if (!Cy) {
      SWAP(A, RAM(B));
      ++BL;
    }
    A += RAM(B);
    SWAP(A, RAM(B));
    A = RAM(B);
    ++BL;
    A += RAM(B);
    SWAP(A, RAM(B));
    ++BL;
    Cy = A + 0x8 >= 0x10;
    A += 0x8;
    if (!Cy)
      A += RAM(B);
    SWAP(A, RAM(B));
    ++BL;

    do {
      A += 0x1;
      A += RAM(B);
      SWAP(A, RAM(B));
      A = RAM(B);
    } while (++BL);
    SWAP(A, X);
    A += 0xf;
  } while (A != 0xf);
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

// 0F:2F
void sub_F2F(void) {
  B = 0x57;
  SWAP(B, SB);
  SWAP(A, BM);
  SWAP(B, SB);
  SWAP(A, RAM(0x57));
  BM ^= 0x1;
  SWAP(B, SB);
  SWAP(A, BL);
  SWAP(B, SB);
  SWAP(A, RAM(0x47));
  BM ^= 0x1;
  ++BL;
  u8 X = 0;  // todo: is this an in or out param?
  SWAP(A, X);
  SWAP(A, RAM(0x58));
  ++BL;
  RAM(0x59) |= BIT(0);
  if (C == 0)
    RAM(0x59) &= ~BIT(0);
  C = 0;
}

// end PIF ROM

void checkInterrupt(void) {
  if (IFA && (RE & BIT(0)) && IME) {
    IFA = 0;
    IME = 0;
    continuation_t saved = continuation;
    continuation = NULL;
    sub_200();
    if (continuation)
      abort();
    continuation = saved;
    return;
  }
}

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
  if (port == 0xe) {
    RE = value;
  }
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
    checkInterrupt();
    (*continuation)();
  }
}

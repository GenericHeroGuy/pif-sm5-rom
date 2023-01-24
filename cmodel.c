#include "cmodel.h"

#include <assert.h>
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

bool IFA = 0;

void checkInterrupt(void);

enum {
  PORT_CIC = 5,
  REG_INT_EN = 0xe,
};

enum {
  CIC_SEED_BUF = 0x1a,
  CIC_SEED = 0x1c,
  CIC_SEED_END = 0x20,
  OSINFO = 0x1b,
  OSINFO_RESET = 1,
  OSINFO_VERSION = 2,
  OSINFO_64DD = 3,
  CIC_CHECKSUM_BUF = 0x20,
  CIC_CHECKSUM = 0x24,
  CIC_CHECKSUM_END = 0x30,
  PIF_CHECKSUM = 0x34,
  PIF_CHECKSUM_END = 0x40,
  BOOT_TIMER = 0x4a,
  BOOT_TIMER_END = 0x50,
  STATUS = 0x5e,
  STATUS_CHALLENGE = 1,
  STATUS_TERMINATE_RECV = 3,
  PIF_CMD_U = 0xfe,
  PIF_CMD_U_LOCKOUT = 0,
  PIF_CMD_U_CHECKSUM = 1,
  PIF_CMD_U_CLEAR = 2,
  PIF_CMD_U_CHECKSUM_ACK = 3,
  PIF_CMD_L = 0xff,
  PIF_CMD_L_JOYBUS = 0,
  PIF_CMD_L_CHALLENGE = 1,
  PIF_CMD_L_2 = 2,
  PIF_CMD_L_TERMINATE = 3,
};

// The only non-code data in the ROM, taken from 04:00.
const u8 rom[] = {
    0x19, 0x4a, 0xf1, 0x88, 0xb5, 0x5a, 0x71, 0xc3,
    0xde, 0x61, 0x10, 0xed, 0x9e, 0x8c, 0x3c, 0x2b,
};

void memZero(u8 address);
void sub_22C(void);
void interruptEpilog(void);
void halt(void);
void cicCompare(void);
void signalError(void);
void memSwapRanges(void);
void memSwap(u8 address);
void boot(void);
void cicReset(void);
bool increment8(void);
void bootTimer(void);
void sub_709(void);
void sub_713(void);
void sub_71B(void);
void sub_900(void);
void sub_909(void);
void spin256(void);
bool sub_916(void);
void sub_91F(void);
void sub_922(void);
void sub_92D(void);
void cicReadNibble(u8 address);
void cicWriteNibble(u8 address);
void sub_C26(void);
void sub_C32(void);
void cicChallenge(void);
void sub_D1B(void);
void initSB(void);
void sub_D35(void);
void cicChecksum(void);
void cicDecode(u8 address);
void sub_E1B(void);
void sub_F00(void);
void sub_F1B(void);
void sub_F2F(void);

// 00:00
void start(void) {
  writeIO(PORT_CIC, 1);
  writeIO(REG_INT_EN, 1);

  initSB();

  memZero(PIF_CHECKSUM);

  cicReadNibble(STATUS);
  switch (RAM(STATUS)) {
    case 1:
      RAM(STATUS) = BIT(OSINFO_VERSION);
      break;
    case 9:
      RAM(STATUS) = BIT(OSINFO_VERSION) | BIT(OSINFO_64DD);
      break;
    default:
      signalError();
      break;
  }

  for (u8 address = 0x80; address != 0; address += 0x10)
    memZero(address);

  for (u8 address = CIC_SEED_BUF; address < CIC_SEED_END; ++address)
    cicReadNibble(address);

  cicDecode(CIC_SEED_BUF);
  cicDecode(CIC_SEED_BUF);

  u8 a = RAM(STATUS);
  RAM(STATUS) &= ~BIT(STATUS_CHALLENGE);
  RAM(STATUS) &= ~BIT(STATUS_TERMINATE_RECV);
  RAM(OSINFO) = a;

  C = 0;

  for (;;) {
    RAM(PIF_CMD_U) |= BIT(PIF_CMD_U_CHECKSUM_ACK);
    IME = 1;
    boot();
  }
}

// 01:16
// zero memory from B to end of segment
void memZero(u8 address) {
  do {
    RAM(address) = 0;
  } while (++address & 0xf);
}

// 01:1A
// fill [0x40..0x45] with 8
void memFill8(void) {
  for (u8 address = 0x45; address >= 0x40; --address)
    RAM(address) = 8;
}

// 01:20
void cicWriteBit(bool value) {
  writeIO(PORT_CIC, value | 2);
  SPIN(5);
  writeIO(PORT_CIC, 1);
  SPIN(4);
}

// 01:2A
// read bit from CIC
bool cicReadBit(void) {
  writeIO(PORT_CIC, 3);
  SPIN(5);
  bool c = readIO(PORT_CIC) & BIT(3);
  writeIO(PORT_CIC, 1);
  SPIN(4);
  return c;
}

// 02:00
void interruptA(void) {
  assert(SB == 0x56);
  SWAP(B, SB);
  SWAP(A, RAM(B));
  ++BL;

  if (!(readIO(BL) & BIT(3)))
    goto loc_21D;
  if (!(readIO(BL) & BIT(2)))
    goto loc_239;
  B = PIF_CMD_L;
  readCommand();
  if (!(RAM(PIF_CMD_L) & BIT(PIF_CMD_L_CHALLENGE))) {
    sub_713();
    return;
  }
  B = STATUS;
  if (!(RAM(STATUS) & BIT(STATUS_TERMINATE_RECV)))
    goto loc_239;
  sub_709();
  return;

loc_21D:
  halt();
  B = PIF_CMD_L;
  readCommand();
  if (!(RAM(PIF_CMD_L) & BIT(PIF_CMD_L_JOYBUS)))
    goto loc_237;
  RAM(PIF_CMD_L) &= ~BIT(PIF_CMD_L_JOYBUS);
  sub_F2F();
  B = 0x10;
  SWAP(B, SB);
  memFill8();
  sub_92D();

  sub_22C();
  return;

loc_237:
  BM = 5;

  interruptEpilog();
  return;

loc_239:
  halt();

  interruptEpilog();
}

// 02:04
void interruptB(void) {
  assert(SB == 0x56);
  SWAP(B, SB);
  SWAP(A, RAM(0x56));
  RAM(STATUS) &= ~BIT(STATUS_TERMINATE_RECV);
  writeIO(REG_INT_EN, 1);
  writeIO(8, 2);

  interruptEpilog();
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

  interruptEpilog();
}

// 02:3B
void interruptEpilog(void) {
  assert(BM == 0x5);
  BL = 6;
  SWAP(A, RAM(0x56));
  SWAP(B, SB);
  IME = 1;
}

// 03:06
void halt(void) {
  assert(BM == 5);
  BL = 0xe;
  A = 1;
  writeIO(REG_INT_EN, 1);

  // todo: should we do anything for halt/standby?
  // HALT = 1;

  if (RAM(STATUS) & BIT(STATUS_TERMINATE_RECV)) {
    A = 5;
    writeIO(REG_INT_EN, 5);
  }
}

// 03:0B
void cicLoop(void) {
  for (;;) {
    IME = 1;
    static bool checkOnce;  // todo: remove, for testing purposes
    if (!checkOnce) {
      checkOnce = true;
      IFA = 1;
      checkInterrupt();
    }

    for (;;) {
      if (!(RAM(STATUS) & BIT(STATUS_TERMINATE_RECV))) {
        cicReset();
        return;
      }
      if (RAM(STATUS) & BIT(STATUS_CHALLENGE)) {
        RAM(STATUS) &= ~BIT(STATUS_CHALLENGE);
        cicChallenge();
        break;
      }

      cicCompare();
    }
  }
}

void cicCompare(void) {
  cicWriteBit(0);
  cicWriteBit(0);
  BM = 6;
  sub_E1B();
  sub_E1B();
  sub_E1B();
  BM = 7;
  sub_E1B();
  sub_E1B();
  sub_E1B();
  B = 0x77;
  A = RAM(0x77);
  if (A)
    A += 0xf;
  A += 1;
  if (A)
    SWAP(A, BL);

loc_329:
  BM = 6;
  cicWriteBit(RAM(B) & BIT(0));
  BM = 7;
  C = cicReadBit();
  if (C == 0)
    goto loc_337;
  if (!(RAM(B) & BIT(0))) {
    signalError();
    return;
  }

loc_334:
  if (++BL)
    goto loc_329;
  return;

loc_337:
  if (!(RAM(B) & BIT(0)))
    goto loc_334;

  signalError();
}

// 03:39
// disable interrupts and strobe R8 forever
void signalError(void) {
  IME = 0;
  do {
    writeIO(8, A);
    A = ~A;
    fatalError();
  } while (1);
}

// 04:0E
// swap internal and external memory
void memSwapRanges(void) {
  memSwap(OSINFO + 0xb0);        // swap [0x1b..0x1f] <-> [0xcb..0xcf]
  memSwap(PIF_CHECKSUM + 0xb0);  // swap [0x34..0x3f] <-> [0xe4..0xef]
}

// 04:14
void memSwap(u8 address) {
  do {
    SWAP(RAM(address), RAM(address - 0xb0));
  } while (++address & 0xf);
}

// 05:00
void boot(void) {
  IME = 0;

  memSwapRanges();
  memZero(PIF_CMD_U);

  IME = 1;

  // wait for rom lockout bit of PIF status byte to become set
  do {
    readCommand();
  } while (!(RAM(PIF_CMD_U) & BIT(PIF_CMD_U_LOCKOUT)));

  IME = 0;

  writeIO(6, 1);
  writeIO(2, 1);
  memFill8();

  IME = 1;

  // wait for checksum verification bit
  do {
    readCommand();
  } while (!(RAM(PIF_CMD_U) & BIT(PIF_CMD_U_CHECKSUM)));

  IME = 0;

  memSwapRanges();
  RAM(PIF_CMD_U) |= BIT(PIF_CMD_U_CHECKSUM_ACK);

  IME = 1;

  // wait for clear pif ram bit
  do {
    readCommand();
  } while (!(RAM(PIF_CMD_U) & BIT(PIF_CMD_U_CLEAR)));

  IME = 0;

  RAM(PIF_CMD_U) = 0;
  if (C == 0)  // only run on cold boot, not on reset
    cicChecksum();

  // compare checksum
  for (u8 i = 0; i < 0xc; ++i) {
    u8 a = RAM(PIF_CHECKSUM + i);
    RAM(PIF_CHECKSUM + i) = 0;
    if (a != RAM(CIC_CHECKSUM + i)) {
      signalError();
    }
  }

  RAM(BOOT_TIMER + 0) = 0xf;
  RAM(BOOT_TIMER + 1) = 0xb;
  memZero(BOOT_TIMER + 2);

  IME = 1;

  for (;;) {
    readCommand();
    if (RAM(PIF_CMD_L) & BIT(PIF_CMD_L_TERMINATE))
      break;

    bootTimer();
  }

  // terminate boot process
  IME = 0;
  memZero(PIF_CMD_U);
  writeIO(REG_INT_EN, 5);
  RAM(STATUS) |= BIT(STATUS_TERMINATE_RECV);
  IFB = 0;

  cicLoop();
}

// 06:00
void cicReset(void) {
  cicWriteBit(1);
  cicWriteBit(1);
  BL = 5;
  A = 3;
  writeIO(PORT_CIC, 3);
  memZero(0x0c);

loc_60A:
  B = PIF_CMD_U;
  RAM(PIF_CMD_U) |= BIT(PIF_CMD_U_CHECKSUM_ACK);
  B = 0x05;
  if (!(readIO(PORT_CIC) & BIT(3)))
    goto loc_618;
  BL = 0xf;
  if (!increment8() || !increment8())
    goto loc_60A;
  signalError();
  BL = 5;

loc_618:
  A = 1;
  writeIO(PORT_CIC, 1);
  BL = 8;
  A = 0;

  while (!(readIO(8) & BIT(3)))
    ;

  IME = 0;
  BL = 6;
  writeIO(6, A);
  A = 9;
  BL = 8;
  writeIO(8, 9);
  A = 8;
  writeIO(8, 8);
  C = 1;
}

// 06:29
bool increment8(void) {
  RAM(B) += 1;
  if (RAM(B)) {
    return false;
  }
  BL--;

  RAM(B) += 1;
  if (RAM(B)) {
    ++BL;
    return false;
  }
  BL--;

  return true;
}

// 07:00
void bootTimer(void) {
  B = BOOT_TIMER_END - 1;
  if (!increment8() || !increment8() || !increment8()) {
    return;
  }

  signalError();
}

// 07:09
void sub_709(void) {
  RAM(PIF_CMD_L) &= ~BIT(PIF_CMD_L_CHALLENGE);
  RAM(STATUS) |= BIT(STATUS_CHALLENGE);
  B = 0x56;
  SWAP(A, RAM(0x56));
  SWAP(B, SB);
}

// 07:13
void sub_713(void) {
  sub_F2F();
  BL = 0xa;
  A = 4;
  goto loc_71F;

loc_718:
  BL = 2;
  A = 1;
  writeIO(2, 1);

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
  BM = 4;
  if (!(RAM(B) & BIT(0)))
    goto loc_727;
  sub_C26();
  goto loc_71B;

loc_727:
  if (!(RAM(B) & BIT(3)))
    goto loc_72A;
  goto loc_71B;

loc_72A:
  BM = 1;
  sub_C32();
  B = 0x22;
  if (!sub_916())
    goto loc_733;
  goto loc_71B;

loc_733:
  sub_91F();
  BL = 3;
  goto loc_818;

loc_738:
  BM = 5;
  halt();

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
  if (!++BL)
    abort();

loc_805:
  if (!(readIO(BL) & BIT(2)))
    goto loc_423;
  if (!(readIO(BL) & BIT(3)))
    goto loc_805;
  SWAP(B, SB);
  A = RAM(B);
  if (++BL)
    writeIO(0, A);
  A = RAM(B);
  writeIO(0, A);
  if (++BL)
    goto loc_817;
  SWAP(A, BM);
  A += 1;
  if (A)
    goto loc_816;
  A = 8;

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
  if (!++BL)
    abort();

loc_828:
  if (!(readIO(BL) & BIT(2)))
    goto loc_423;
  if (!(readIO(BL) & BIT(3)))
    goto loc_828;
  A = readIO(1);
  SWAP(B, SB);
  SWAP(A, RAM(B));
  if (++BL)
    A = readIO(1);
  SWAP(A, RAM(B));
  if (++BL)
    goto loc_837;
  SWAP(A, BM);
  BM = 8;
  A += 1;
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
  BL = 2;
  A = 0;
  writeIO(2, 0);
  A = 1;
  writeIO(2, 1);
  BL = 0xa;
  A = readIO(0xa);
  // todo: read upper half into X?
  SWAP(A, BL);
  BM = 1;
  sub_C32();
  BL = 4;
  if (!(readIO(4) & BIT(3)))
    goto loc_433;
  A = 8;
  goto loc_434;

loc_433:
  A = 4;

loc_434:
  SWAP(B, SB);
  if (++BL)
    sub_D35();
  A += RAM(B);
  SWAP(A, RAM(B));
  SWAP(B, SB);
  A = 0;
  writeIO(4, 0);

  goto loc_71B;
}

// 09:00
void sub_900(void) {
  B = PIF_CMD_L;
  if (!(RAM(PIF_CMD_L) & BIT(PIF_CMD_L_2))) {
    sub_909();
    return;
  }
  if (!(RAM(PIF_CMD_L) & BIT(PIF_CMD_L_TERMINATE)))
    sub_909();
  spin256();
}

// 09:09
void sub_909(void) {
  BL = 2;
  A = 2;
  writeIO(2, 2);
}

// 09:0D
void spin256(void) {
  A = 0;
  do {
    SPIN(16);
  } while (++A);
}

// 09:16
bool sub_916(void) {
  SWAP(B, SB);
  if (!(RAM(B) & BIT(3)))
    goto loc_91A;
  return true;

loc_91A:
  if (!(RAM(B) & BIT(2))) {
    sub_922();
    return false;
  }
  sub_C26();
  return true;
}

// 09:1F
void sub_91F(void) {
  SWAP(B, SB);
  RAM(B) &= ~BIT(3);
  RAM(B) &= ~BIT(2);

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
  BM ^= 1;
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
    A += 1;
    if (A)
      goto loc_A03;
  } while (++BL);

  SWAP(A, BM);
  A += 1;

  SWAP(A, BM);
  goto loc_92F;

loc_A03:
  A += 1;
  if (!A)
    return;

  A += 1;
  if (A)
    goto loc_A1F;
  SWAP(B, SB);
  BM = 4;
  RAM(B) |= BIT(0);
  BM = 1;
  SWAP(B, SB);
  goto loc_A14;

loc_A0E:
  A = 0;
  if (0 != RAM(B))
    goto loc_A20;
  ++BL;
  if (0 != RAM(B))
    goto loc_A1F;

loc_A14:
  if (++BL)
    goto loc_A1B;
  SWAP(A, BM);
  A += 1;
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
  BM ^= 1;
  SWAP(B, SB);
  SWAP(A, BL);
  X = A;
  SWAP(A, BL);
  SWAP(A, X);
  SWAP(B, SB);
  SWAP(A, RAM(B));
  BM ^= 1;
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
  BM = 4;
  if (BL--)
    RAM(B) &= ~BIT(3);
  if (++BL)
    BM = 1;
  SWAP(A, X);

  SWAP(A, RAM(B));
  BM ^= 1;
  SWAP(A, X);
  SWAP(A, RAM(B));
  BM ^= 1;
  SWAP(B, SB);
  if (++BL)
    goto loc_B0B;
  SWAP(A, BM);
  A += 1;
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
  BM = 0;
  C = A + RAM(B) >= 0x10;
  A = A + RAM(B);
  SWAP(A, RAM(B));
  BM ^= 1;
  SWAP(A, X);
  bool Cy = A + RAM(B) + C >= 0x10;
  A = A + RAM(B) + C;
  C = Cy;
  if (!Cy) {
    SWAP(A, RAM(B));
    BM ^= 1;
  }
  A = RAM(B);
  C = A + RAM(B) >= 0x10;
  A = A + RAM(B);
  SWAP(A, RAM(B));
  BM ^= 1;
  A = RAM(B);
  Cy = A + RAM(B) + C >= 0x10;
  A = A + RAM(B) + C;
  C = Cy;
  if (!Cy) {
    SWAP(A, RAM(B));
    BM ^= 1;
  }
  SWAP(B, SB);
  SWAP(A, BL);
  SWAP(B, SB);
  C = A + RAM(B) + 1 >= 0x10;
  A = A + RAM(B) + 1;
  BM = 1;
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
  A = 5;
  if (5 == BL)
    return;
  SWAP(B, SB);
  goto loc_92F;

loc_B3A:
  SWAP(B, SB);

loc_B3B:
  if (BL--)
    BM = 4;
  RAM(B) &= ~BIT(0);
  RAM(B) |= BIT(3);
}

// 0C:00
// read nibble from CIC into [B]
// increments BL, returns true on carry
void cicReadNibble(u8 address) {
  RAM(address) = 0xf;
  if (!cicReadBit())
    RAM(address) &= ~BIT(3);
  if (!cicReadBit())
    RAM(address) &= ~BIT(2);
  if (!cicReadBit())
    RAM(address) &= ~BIT(1);
  if (!cicReadBit())
    RAM(address) &= ~BIT(0);
  C = 0;
}

// 0C:10
void cicWriteNibble(u8 address) {
  cicWriteBit(RAM(address) & BIT(3));
  cicWriteBit(RAM(address) & BIT(2));
  cicWriteBit(RAM(address) & BIT(1));
  cicWriteBit(RAM(address) & BIT(0));
}

// 0C:26
void sub_C26(void) {
  while (!(readIO(3) & BIT(3)))
    writeIO(4, 0);

  writeIO(2, 3);
  A = 1;
  writeIO(2, 1);
  BL = 3;

  while (!(readIO(3) & BIT(3)))
    ;
}

// 0C:32
void sub_C32(void) {
  A = RAM(B);
  BM ^= 1;
  SWAP(B, SB);
  SWAP(A, BM);
  SWAP(B, SB);
  A = RAM(B);
  BM ^= 1;
  SWAP(B, SB);
  SWAP(A, BL);
  SWAP(B, SB);
}

// 0D:00
void cicChallenge(void) {
  cicWriteBit(1);
  cicWriteBit(0);
  cicReadNibble(0x0a);
  cicReadNibble(0x0b);
  B = 0xdd;
  sub_D1B();

  B = 0x0b;
  while (!increment8())
    ;
  C = cicReadBit();
  B = 0xdf;
  sub_D1B();
  BM = 5;
  halt();
  initSB();
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
  cicWriteNibble(B);
  ++BL;
  cicWriteNibble(B);
  if (++BL)
    goto loc_D1E;
  goto loc_D2F;

loc_D2B:
  SWAP(B, SB);
  cicReadNibble(B);
  ++BL;
  cicReadNibble(B);
  if (++BL)
    goto loc_D1E;

loc_D2F:
  BM = 0xf;
  goto loc_D1E;
}

// 0D:31
void initSB(void) {
  SB = 0x56;
}

// 0D:35
// increment B and wrap to 0x80 on overflow
void sub_D35(void) {
  if (++B == 0)
    B = 0x80;
}

// 0E:00
void cicChecksum(void) {
  B = OSINFO;
  RAM(OSINFO) |= BIT(OSINFO_RESET);
  sub_F1B();
  BL = 5;
  A = 3;
  writeIO(PORT_CIC, 3);
  spin256();
  A = 1;
  writeIO(PORT_CIC, 1);

  for (u8 address = CIC_CHECKSUM_BUF; address < CIC_CHECKSUM_END; ++address)
    cicReadNibble(address);

  cicDecode(CIC_CHECKSUM_BUF);
  cicDecode(CIC_CHECKSUM_BUF);
  cicDecode(CIC_CHECKSUM_BUF);
  cicDecode(CIC_CHECKSUM_BUF);

  sub_F00();
}

// 0E:15
// decode CIC seed or checksum (one round)
void cicDecode(u8 address) {
  u8 a = 0xf;
  do {
    u8 b = RAM(address);
    RAM(address) -= a + 1;
    a = b;
  } while (++address & 0xf);
}

// 0E:1B
void sub_E1B(void) {
  BL = 0xf;
  A = RAM(B);

  do {
    u8 X = A;
    BL = 1;
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
    Cy = A + 8 >= 0x10;
    A += 8;
    if (!Cy)
      A += RAM(B);
    SWAP(A, RAM(B));
    ++BL;

    do {
      A += 1;
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
  A = 0;
  SWAP(A, RAM(0x60));
  SWAP(B, SB);
  B = 0x62;
  do {
    SWAP(B, SB);
    A = RAM(B);
    A += 1;
    if (A)
      SWAP(A, RAM(B));
    SWAP(B, SB);
    u8 byte = rom[A];
    A = byte & 0xf;
    u8 X = byte >> 4;
    SWAP(A, RAM(B));
    BM ^= 1;
    SWAP(A, X);
    SWAP(A, RAM(B));
    BM ^= 1;
  } while (++BL);
  cicWriteNibble(0x61);
  cicWriteNibble(0x71);
  initSB();
}

// 0F:1B
void sub_F1B(void) {
  B = 0x69;
  A = 1;
  writeIO(9, 1);

  do {
    increment8();
    BL = 9;
  } while (!(readIO(9) & BIT(3)));
  A = 0;
  writeIO(9, 0);

  u8 X = {0};  // todo: confirm the value on entry is unimportant
  SWAP(A, RAM(B));
  BL--;
  SWAP(A, X);
  SWAP(A, RAM(B));
  BL = 1;
  SWAP(A, RAM(B));
  BM ^= 1;
  SWAP(A, X);
  SWAP(A, RAM(B));
  BM ^= 1;
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
  BM ^= 1;
  SWAP(B, SB);
  SWAP(A, BL);
  SWAP(B, SB);
  SWAP(A, RAM(0x47));
  BM ^= 1;
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
    interruptA();
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
  RAM(PIF_CMD_U) = value >> 4;
  RAM(PIF_CMD_L) = value & 0xf;
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
  start();
}

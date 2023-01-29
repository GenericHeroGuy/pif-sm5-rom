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

bool reset = 0;

void checkInterrupt(void);

enum {
  PORT_CIC = 5,
  REG_INT_EN = 0xe,
};

enum {
  CIC_CHALLENGE_TIMER_U = 0x0a,
  CIC_CHALLENGE_TIMER_L = 0x0b,
  RESET_TIMER = 0x0c,
  RESET_TIMER_END = 0x10,
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
  SAVE_SBL = 0x47,
  BOOT_TIMER = 0x4a,
  BOOT_TIMER_END = 0x50,
  SAVE_A = 0x56,
  SAVE_SBM = 0x57,
  SAVE_X = 0x58,
  SAVE_C = 0x59,
  STATUS = 0x5e,
  STATUS_CHALLENGE = 1,
  STATUS_TERMINATE_RECV = 3,
  CIC_COMPARE_LO = 0x60,
  CIC_COMPARE_LO_END = 0x70,
  CIC_COMPARE_HI = 0x70,
  CIC_COMPARE_HI_END = 0x80,
  RAM_EXTERNAL = 0x80,
  CIC_CHALLENGE_COUNT_OUT = 0xdd,
  CIC_CHALLENGE_COUNT_IN = 0xdf,
  CIC_CHALLENGE_LO = 0xe0,
  CIC_CHALLENGE_HI = 0xf0,
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
    0x19, 0x4a, 0xf1, 0x88, 0xb5, 0x5a, 0x71,
    0xc3, 0xde, 0x61, 0x10, 0xed, 0x9e, 0x8c,
};

void start(void);
void bootTimerInit(u8 address);
void memZero(u8 address);
void memFill8(void);
void cicWriteBit(bool value);
bool cicReadBit(void);
void interruptA(void);
void interruptB(void);
void regRestore(void);
void interruptEpilog(void);
void halt(void);
void cicLoop(void);
void cicCompare(void);
void signalError(void);
void memSwapRanges(void);
void memSwap(u8 address);
void boot(void);
void cicReset(void);
bool increment8(u8* address);
void bootTimer(void);
void interruptEpilogID(void);
void sub_713(void);
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
void regRestoreSB(u8 address);
void cicChallenge(void);
void cicChallengeTransfer(u8 address);
void regInitSB(void);
u8 incrementPtr(u8 address);
void cicChecksum(void);
void cicDecode(u8 address);
void cicCompareRound(u8 address);
void cicCompareInit(void);
void cicCompareSeed(void);
void regSave(void);

// 00:00
void start(void) {
  writeIO(PORT_CIC, 1);
  writeIO(REG_INT_EN, 1);

  regInitSB();

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

  for (u8 address = RAM_EXTERNAL; address != 0; address += 0x10)
    memZero(address);

  for (u8 address = CIC_SEED_BUF; address < CIC_SEED_END; ++address)
    cicReadNibble(address);

  cicDecode(CIC_SEED_BUF);
  cicDecode(CIC_SEED_BUF);

  u8 a = RAM(STATUS);
  RAM_BIT_RESET(STATUS, STATUS_CHALLENGE);
  RAM_BIT_RESET(STATUS, STATUS_TERMINATE_RECV);
  RAM(OSINFO) = a;

  reset = 0;

  for (;;) {
    RAM_BIT_SET(PIF_CMD_U, PIF_CMD_U_CHECKSUM_ACK);
    IME = 1;
    boot();
  }
}

// 01:12
void bootTimerInit(u8 address) {
  RAM(address + 0) = 0xf;
  RAM(address + 1) = 0xb;
  memZero(address + 2);
}

// 01:16
// zero memory from address to end of segment
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
  SB = B;
  RAM(SAVE_A) = A;

  if (readIO(7) & BIT(3)) {
    if (readIO(7) & BIT(2)) {
      readCommand();
      if (!RAM_BIT_TEST(PIF_CMD_L, PIF_CMD_L_CHALLENGE)) {
        sub_713();
        return;
      }

      if (RAM_BIT_TEST(STATUS, STATUS_TERMINATE_RECV)) {
        interruptEpilogID();
        return;
      }
    }

    halt();
  } else {
    halt();

    readCommand();
    if (RAM_BIT_TEST(PIF_CMD_L, PIF_CMD_L_JOYBUS)) {
      RAM_BIT_RESET(PIF_CMD_L, PIF_CMD_L_JOYBUS);

      regSave();
      SB = 0x10;
      memFill8();
      sub_92D();
      regRestore();
      return;
    }
  }

  interruptEpilog();
}

// 02:04
void interruptB(void) {
  SB = B;
  RAM(SAVE_A) = A;
  RAM_BIT_RESET(STATUS, STATUS_TERMINATE_RECV);
  writeIO(REG_INT_EN, 1);
  writeIO(8, 2);

  interruptEpilog();
}

// 02:2C
void regRestore(void) {
  C = 1;
  if (!RAM_BIT_TEST(SAVE_C, 0))
    C = 0;
  X = RAM(SAVE_X);
  regRestoreSB(SAVE_SBM);

  interruptEpilog();
}

// 02:3B
void interruptEpilog(void) {
  A = RAM(SAVE_A);
  B = SB;
  IME = 1;
}

// 03:06
void halt(void) {
  writeIO(REG_INT_EN, 1);

  // todo: should we do anything for halt/standby?
  // HALT = 1;

  if (RAM_BIT_TEST(STATUS, STATUS_TERMINATE_RECV)) {
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
      if (!RAM_BIT_TEST(STATUS, STATUS_TERMINATE_RECV)) {
        cicReset();
        return;
      }
      if (RAM_BIT_TEST(STATUS, STATUS_CHALLENGE)) {
        RAM_BIT_RESET(STATUS, STATUS_CHALLENGE);
        cicChallenge();
        break;
      }

      cicCompare();
    }
  }
}

// 03:16
void cicCompare(void) {
  cicWriteBit(0);
  cicWriteBit(0);
  cicCompareRound(CIC_COMPARE_LO);
  cicCompareRound(CIC_COMPARE_LO);
  cicCompareRound(CIC_COMPARE_LO);
  cicCompareRound(CIC_COMPARE_HI);
  cicCompareRound(CIC_COMPARE_HI);
  cicCompareRound(CIC_COMPARE_HI);
  u8 offset = RAM(CIC_COMPARE_HI + 7);
  if (!offset)
    offset = 1;

  for (; offset < 0x10; ++offset) {
    cicWriteBit(RAM_BIT_TEST(CIC_COMPARE_LO + offset, 0));
    bool c = cicReadBit();
    if (c != RAM_BIT_TEST(CIC_COMPARE_HI + offset, 0)) {
      signalError();
    }
  }
}

// 03:39
// disable interrupts and strobe R8 forever
void signalError(void) {
  IME = 0;
  u8 a = 0;  // incoming value doesn't really matter
  do {
    writeIO(8, a);
    a = ~a;
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
  } while (!RAM_BIT_TEST(PIF_CMD_U, PIF_CMD_U_LOCKOUT));

  IME = 0;

  writeIO(6, 1);
  writeIO(2, 1);
  memFill8();

  IME = 1;

  // wait for checksum verification bit
  do {
    readCommand();
  } while (!RAM_BIT_TEST(PIF_CMD_U, PIF_CMD_U_CHECKSUM));

  IME = 0;

  memSwapRanges();
  RAM_BIT_SET(PIF_CMD_U, PIF_CMD_U_CHECKSUM_ACK);

  IME = 1;

  // wait for clear pif ram bit
  do {
    readCommand();
  } while (!RAM_BIT_TEST(PIF_CMD_U, PIF_CMD_U_CLEAR));

  IME = 0;

  RAM(PIF_CMD_U) = 0;
  if (reset == 0)  // only run on cold boot, not on reset
    cicChecksum();

  // compare checksum
  for (u8 i = 0; i < 0xc; ++i) {
    u8 a = RAM(PIF_CHECKSUM + i);
    RAM(PIF_CHECKSUM + i) = 0;
    if (a != RAM(CIC_CHECKSUM + i)) {
      signalError();
    }
  }

  bootTimerInit(BOOT_TIMER);

  IME = 1;

  for (;;) {
    readCommand();
    if (RAM_BIT_TEST(PIF_CMD_L, PIF_CMD_L_TERMINATE))
      break;

    bootTimer();
  }

  // terminate boot process
  IME = 0;
  memZero(PIF_CMD_U);
  writeIO(REG_INT_EN, 5);
  RAM_BIT_SET(STATUS, STATUS_TERMINATE_RECV);
  IFB = 0;

  cicLoop();
}

// 06:00
void cicReset(void) {
  cicWriteBit(1);
  cicWriteBit(1);
  writeIO(PORT_CIC, 3);
  memZero(RESET_TIMER);

  for (;;) {
    RAM_BIT_SET(PIF_CMD_U, PIF_CMD_U_CHECKSUM_ACK);
    if (!(readIO(PORT_CIC) & BIT(3)))
      break;

    u8 b = RESET_TIMER_END - 1;
    if (increment8(&b) && increment8(&b))
      signalError();
  }

  writeIO(PORT_CIC, 1);

  while (!(readIO(8) & BIT(3)))
    ;

  IME = 0;
  writeIO(6, 0);
  writeIO(8, 9);
  writeIO(8, 8);
  reset = 1;
}

// 06:29
bool increment8(u8* address) {
  RAM(*address) += 1;
  if (RAM(*address)) {
    return false;
  }
  *address -= 1;

  RAM(*address) += 1;
  if (RAM(*address)) {
    *address += 1;
    return false;
  }
  *address -= 1;

  return true;
}

// 07:00
void bootTimer(void) {
  u8 b = BOOT_TIMER_END - 1;
  if (!increment8(&b) || !increment8(&b) || !increment8(&b)) {
    return;
  }

  signalError();
}

// 07:09
// leaves interrupts disabled
void interruptEpilogID(void) {
  RAM_BIT_RESET(PIF_CMD_L, PIF_CMD_L_CHALLENGE);
  RAM_BIT_SET(STATUS, STATUS_CHALLENGE);
  A = RAM(SAVE_A);
  B = SB;
}

// 07:13
void sub_713(void) {
  regSave();
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
  if (!RAM_BIT_TEST(B, 0))
    goto loc_727;
  sub_C26();
  goto loc_71B;

loc_727:
  if (!RAM_BIT_TEST(B, 3))
    goto loc_72A;
  goto loc_71B;

loc_72A:
  BM = 1;
  regRestoreSB(B);
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

  regRestore();
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
  regRestoreSB(B);
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
    B = incrementPtr(B);
  A += RAM(B);
  SWAP(A, RAM(B));
  SWAP(B, SB);
  A = 0;
  writeIO(4, 0);

  goto loc_71B;
}

// 09:00
void sub_900(void) {
  if (!RAM_BIT_TEST(PIF_CMD_L, PIF_CMD_L_2)) {
    sub_909();
    return;
  }
  if (!RAM_BIT_TEST(PIF_CMD_L, PIF_CMD_L_TERMINATE))
    sub_909();
  spin256();
}

// 09:09
void sub_909(void) {
  writeIO(2, 2);
}

// 09:0D
void spin256(void) {
  u8 a = 0;
  do {
    SPIN(16);
  } while (++a & 0xf);
}

// 09:16
bool sub_916(void) {
  SWAP(B, SB);
  if (RAM_BIT_TEST(B, 3))
    return true;

  if (!RAM_BIT_TEST(B, 2)) {
    sub_922();
    return false;
  }
  sub_C26();
  return true;
}

// 09:1F
void sub_91F(void) {
  SWAP(B, SB);
  RAM_BIT_RESET(B, 3);
  RAM_BIT_RESET(B, 2);

  sub_922();
}

// 09:22
void sub_922(void) {
  u8 a = RAM(B);
  SWAP(B, SB);
  SWAP(a, RAM(B));
  ++BL;
  SWAP(B, SB);
  if (++BL)
    a = RAM(B);
  B = incrementPtr(B);
  SWAP(B, SB);
  SWAP(a, RAM(B));
  BM ^= 1;
  BL--;
}

// 09:2D
void sub_92D(void) {
  B = RAM_EXTERNAL;

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
  RAM_BIT_SET(B, 0);
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
  X = A;
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
  RAM_BIT_RESET(B, 2);
  RAM_BIT_RESET(B, 3);
  SWAP(A, RAM(B));
  if (++BL)
    SWAP(A, X);
  A = RAM(B);
  SWAP(B, SB);
  BM = 4;
  if (BL--)
    RAM_BIT_RESET(B, 3);
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
  RAM_BIT_RESET(B, 3);
  RAM_BIT_RESET(B, 2);
  SWAP(A, RAM(B));
  if (++BL)
    SWAP(A, X);
  A = RAM(B);
  SWAP(B, SB);
  BM = 0;
  bool c = A + RAM(B) >= 0x10;
  A = A + RAM(B);
  SWAP(A, RAM(B));
  BM ^= 1;
  SWAP(A, X);
  bool Cy = A + RAM(B) + c >= 0x10;
  A = A + RAM(B) + c;
  c = Cy;
  if (!Cy) {
    SWAP(A, RAM(B));
    BM ^= 1;
  }
  A = RAM(B);
  c = A + RAM(B) >= 0x10;
  A = A + RAM(B);
  SWAP(A, RAM(B));
  BM ^= 1;
  A = RAM(B);
  Cy = A + RAM(B) + c >= 0x10;
  A = A + RAM(B) + c;
  c = Cy;
  if (!Cy) {
    SWAP(A, RAM(B));
    BM ^= 1;
  }
  SWAP(B, SB);
  SWAP(A, BL);
  SWAP(B, SB);
  c = A + RAM(B) + 1 >= 0x10;
  A = A + RAM(B) + 1;
  BM = 1;
  SWAP(B, SB);
  SWAP(A, BL);
  SWAP(A, BM);
  SWAP(B, SB);
  Cy = A + RAM(B) + c >= 0x10;
  A = A + RAM(B) + c;
  c = Cy;
  if (!Cy)
    goto loc_B30;
  goto loc_B3B;

loc_B30:
  SWAP(B, SB);
  SWAP(A, BM);
  SWAP(B, SB);

loc_B33:
  A = 5;
  if (BL == 5)
    return;
  SWAP(B, SB);
  goto loc_92F;

loc_B3A:
  SWAP(B, SB);

loc_B3B:
  if (BL--)
    BM = 4;
  RAM_BIT_RESET(B, 0);
  RAM_BIT_SET(B, 3);
}

// 0C:00
// read nibble from CIC into [address]
void cicReadNibble(u8 address) {
  RAM(address) = 0xf;
  if (!cicReadBit())
    RAM_BIT_RESET(address, 3);
  if (!cicReadBit())
    RAM_BIT_RESET(address, 2);
  if (!cicReadBit())
    RAM_BIT_RESET(address, 1);
  if (!cicReadBit())
    RAM_BIT_RESET(address, 0);
}

// 0C:10
void cicWriteNibble(u8 address) {
  cicWriteBit(RAM_BIT_TEST(address, 3));
  cicWriteBit(RAM_BIT_TEST(address, 2));
  cicWriteBit(RAM_BIT_TEST(address, 1));
  cicWriteBit(RAM_BIT_TEST(address, 0));
}

// 0C:26
void sub_C26(void) {
  while (!(readIO(3) & BIT(3)))
    writeIO(4, 0);

  writeIO(2, 3);
  writeIO(2, 1);

  while (!(readIO(3) & BIT(3)))
    ;
}

// 0C:32
void regRestoreSB(u8 address) {
  SBM = RAM(address);
  SBL = RAM(address ^ 0x10);
}

// 0D:00
void cicChallenge(void) {
  cicWriteBit(1);
  cicWriteBit(0);
  cicReadNibble(CIC_CHALLENGE_TIMER_U);
  cicReadNibble(CIC_CHALLENGE_TIMER_L);
  cicChallengeTransfer(CIC_CHALLENGE_COUNT_OUT);

  u8 b = CIC_CHALLENGE_TIMER_L;
  while (!increment8(&b))
    ;
  cicReadBit();  // return value discarded
  cicChallengeTransfer(CIC_CHALLENGE_COUNT_IN);

  halt();
  regInitSB();
}

// 0D:1B
void cicChallengeTransfer(u8 address) {
  for (u8 b = CIC_CHALLENGE_LO; RAM(address) != 0; b += 2) {
    RAM(address) += 0xf;
    if (address != CIC_CHALLENGE_COUNT_OUT) {
      cicReadNibble(b + 0);
      cicReadNibble(b + 1);
    } else {
      cicWriteNibble(b + 0);
      cicWriteNibble(b + 1);
    }
  }
}

// 0D:31
void regInitSB(void) {
  SB = SAVE_A;
}

// 0D:35
// increment address and wrap to 0x80 on overflow
u8 incrementPtr(u8 address) {
  if (++address == 0)
    address = RAM_EXTERNAL;
  return address;
}

// 0E:00
void cicChecksum(void) {
  RAM_BIT_SET(OSINFO, OSINFO_RESET);

  cicCompareSeed();

  writeIO(PORT_CIC, 3);
  spin256();
  writeIO(PORT_CIC, 1);

  for (u8 address = CIC_CHECKSUM_BUF; address < CIC_CHECKSUM_END; ++address)
    cicReadNibble(address);

  cicDecode(CIC_CHECKSUM_BUF);
  cicDecode(CIC_CHECKSUM_BUF);
  cicDecode(CIC_CHECKSUM_BUF);
  cicDecode(CIC_CHECKSUM_BUF);

  cicCompareInit();
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
void cicCompareRound(u8 address) {
  for (u8 x = RAM(address + 0xf); x < 0x10; --x) {
    u8 a = x;
    u8 b = address + 1;
    a += RAM(b) + 1;
    RAM(b) = a;
    ++b;
    a += RAM(b) + 1;
    a = ~a;
    SWAP(a, RAM(b));
    ++b;
    bool Cy = (a & 0xf) + RAM(b) + 1 >= 0x10;
    a += RAM(b) + 1;
    if (!Cy) {
      SWAP(a, RAM(b));
      ++b;
    }
    a += RAM(b);
    RAM(b) = a;
    ++b;
    a += RAM(b);
    SWAP(a, RAM(b));
    ++b;
    Cy = (a & 0xf) + 8 >= 0x10;
    a += 8;
    if (!Cy)
      a += RAM(b);
    SWAP(a, RAM(b));
    ++b;
    do {
      a += RAM(b) + 1;
      RAM(b) = a;
    } while (++b & 0xf);
  }
}

// 0F:00
void cicCompareInit(void) {
  RAM(CIC_COMPARE_LO) = 0;
  for (u8 offset = 2; offset < 0x10; ++offset) {
    u8 byte = rom[RAM(CIC_COMPARE_LO)];
    RAM(CIC_COMPARE_LO) += 1;
    RAM(CIC_COMPARE_LO + offset) = byte & 0xf;
    RAM(CIC_COMPARE_HI + offset) = byte >> 4;
  }
  cicWriteNibble(CIC_COMPARE_LO + 1);
  cicWriteNibble(CIC_COMPARE_HI + 1);
  regInitSB();
}

// 0F:1B
void cicCompareSeed(void) {
  writeIO(9, 1);

  do {
    u8 b = CIC_COMPARE_LO + 9;
    increment8(&b);
  } while (!(readIO(9) & BIT(3)));

  writeIO(9, 0);

  RAM(CIC_COMPARE_LO + 1) = RAM(CIC_COMPARE_LO + 8);
  RAM(CIC_COMPARE_HI + 1) = RAM(CIC_COMPARE_LO + 9);
  RAM(CIC_COMPARE_LO + 8) = 0;  // X is stored here but it's guaranteed to be 0
  RAM(CIC_COMPARE_LO + 9) = 0;
}

// 0F:2F
// save SB, X, C
void regSave(void) {
  RAM(SAVE_SBM) = SBM;
  RAM(SAVE_SBL) = SBL;
  RAM(SAVE_X) = X;
  RAM_BIT_SET(SAVE_C, 0);
  if (C == 0)
    RAM_BIT_RESET(SAVE_C, 0);
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

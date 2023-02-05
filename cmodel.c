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

void checkInterrupt(void);

enum {
  PORT_JOYBUS_WRITE = 0,
  PORT_JOYBUS_READ = 1,
  PORT_JOYBUS_CTRL = 2,
  PORT_JOYBUS_STATUS = 3,
  PORT_JOYBUS_ERROR = 4,
  PORT_CIC = 5,
  PORT_ROM = 6,
  PORT_RCP_XFER = 7,
  PORT_RESET = 8,
  PORT_RNG = 9,    // Used as a RNG, maybe it's an ADC?
  PORT_JOYBUS_CHANNEL = 0xa,
  REG_INT_EN = 0xe,

  JOYBUS_STATUS_CLOCK = BIT(3),

  JOYBUS_CTRL_WRITESTOPBIT = BIT(1),

  JOYBUS_ERROR_RESET = 0,
  JOYBUS_ERROR_NOANSWER = BIT(3),

  INT_A_EN = BIT(0),
  INT_B_EN = BIT(2),

  ROM_LOCKOUT = BIT(0),

  RCP_XFER_READ = BIT(3),
  RCP_XFER_64B = BIT(2),

  RESET_CPU_IRQ = BIT(1),
  RESET_CPU_NMI = BIT(0),
  RESET_BUTTON = BIT(3),

  RNG_START = BIT(0),
  RNG_DATA = BIT(3),
};

enum {
  JOYBUS_ADDR_L = 0x00,     // pointer to the start of the frame in external RAM for each joybus channel (low nibble)
  JOYBUS_ADDR_U = 0x10,     // pointer to the start of the frame in external RAM for each joybus channel (high nibble)
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
  JOYBUS_SEND_COUNT_U = 0x22,
  JOYBUS_SEND_COUNT_L = 0x23,
  JOYBUS_SENDERR_NO_DEVICE = 8,
  JOYBUS_SENDERR_TIMEOUT = 4,
  PIF_CHECKSUM = 0x34,
  PIF_CHECKSUM_END = 0x40,
  JOYBUS_RECV_COUNT_U = 0x32,
  JOYBUS_RECV_COUNT_L = 0x33,
  JOYBUS_STATUS = 0x40,
  JOYBUS_STATUS_RESET = 0,
  JOYBUS_STATUS_SKIP = 3,
  JOYBUS_STATUS_END = 0x46,
  SAVE_SBL = 0x47,
  BOOT_TIMER = 0x4a,
  BOOT_TIMER_END = 0x50,
  SAVE_A = 0x56,
  SAVE_SBM = 0x57,
  SAVE_X = 0x58,
  SAVE_C = 0x59,
  STATUS = 0x5e,
  STATUS_CHALLENGE = 1,
  STATUS_RUNNING = 3,
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
  PIF_CMD_U_GET_CHECKSUM = 1,
  PIF_CMD_U_CHECK_CHECKSUM = 2,
  PIF_CMD_U_ACK = 3,
  PIF_CMD_L = 0xff,
  PIF_CMD_L_JOYBUS = 0,
  PIF_CMD_L_CHALLENGE = 1,
  PIF_CMD_L_2 = 2,
  PIF_CMD_L_TERMINATE = 3,
};

bool reset = 0;
bool regionPAL = 0;  // compile time constant in real PIF ROMs

// The only non-code data in the ROM, taken from 04:00.
const u8 romNTSC[] = {
    0x19, 0x4a, 0xf1, 0x88, 0xb5, 0x5a, 0x71,
    0xc3, 0xde, 0x61, 0x10, 0xed, 0x9e, 0x8c,
};
const u8 romPAL[] = {
    0x14, 0x2f, 0x35, 0xf1, 0x82, 0x21, 0x77,
    0x11, 0x99, 0x88, 0x15, 0x17, 0x55, 0xca,
};

void start(void);
void bootTimerInit(u8 address);
void memZero(u8 address);
void joybusStatusInit(void);
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
void joybusHandleError(void);
void boot(void);
void cicReset(void);
bool increment8(u8* address);
void bootTimer(void);
void interruptEpilogID(void);
void joybusTransfer(void);
void joybusTransferChannel(u8 n);
void joybusWait(void);
void joybusWriteStopBit(void);
void spin256(void);
bool joybusCopySendCount(u8 b, u8* sb);
void joybusCopyRecvCount(u8 b, u8* sb);
void joybusCopyByte(u8 b, u8* sb);
void joybusCommandParse(void);
bool joybusCommandAdvance(u8* address, u8 channel);
void cicReadNibble(u8 address);
void cicWriteNibble(u8 address);
void joybusResetChannel(void);
u8 readByte(u8 address);
void cicChallenge(void);
void cicChallengeTransfer(u8 address);
void regInitSB(void);
u8 incrementPtr(u8 address);
void cicCompareInit(void);
void cicDecode(u8 address);
void cicCompareRound(u8 address);
void cicCompareExpandSeed(void);
void cicCompareCreateSeed(void);
void regSave(void);

// 00:00
void start(void) {
  writeIO(PORT_CIC, 1);
  writeIO(REG_INT_EN, INT_A_EN);

  regInitSB();

  memZero(PIF_CHECKSUM);

  cicReadNibble(STATUS);
  if ((RAM(STATUS) & 3) == 1 && RAM_BIT_TEST(STATUS, 2) == regionPAL) {
    if (RAM_BIT_TEST(STATUS, 3)) {
      RAM(STATUS) = BIT(OSINFO_VERSION) | BIT(OSINFO_64DD);
    } else {
      RAM(STATUS) = BIT(OSINFO_VERSION);
    }
  } else {
    signalError();
  }

  for (u8 address = RAM_EXTERNAL; address != 0; address += 0x10)
    memZero(address);

  for (u8 address = CIC_SEED_BUF; address < CIC_SEED_END; ++address)
    cicReadNibble(address);

  cicDecode(CIC_SEED_BUF);
  cicDecode(CIC_SEED_BUF);

  u8 a = RAM(STATUS);
  RAM_BIT_RESET(STATUS, STATUS_CHALLENGE);
  RAM_BIT_RESET(STATUS, STATUS_RUNNING);
  RAM(OSINFO) = a;

  reset = 0;

  for (;;) {
    RAM_BIT_SET(PIF_CMD_U, PIF_CMD_U_ACK);
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
void joybusStatusInit(void) {
  for (u8 address = JOYBUS_STATUS_END - 1; address >= JOYBUS_STATUS; --address)
    RAM(address) = BIT(JOYBUS_STATUS_SKIP);
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
// triggered by DMA 64B 
void interruptA(void) {
  SB = B;
  RAM(SAVE_A) = A;

  if (readIO(PORT_RCP_XFER) & RCP_XFER_READ) {
    if (readIO(PORT_RCP_XFER) & RCP_XFER_64B) {

      // A 64B read was issued by RCP. If the 6105 challenge is requested do that,
      // otherwise execute the joybus transfer that was last programmed.
      readCommand();
      if (!RAM_BIT_TEST(PIF_CMD_L, PIF_CMD_L_CHALLENGE)) {
        joybusTransfer();
        return;
      }

      if (RAM_BIT_TEST(STATUS, STATUS_RUNNING)) {
        interruptEpilogID();
        return;
      }
    }

    halt();
  } else {
    halt();

    // This is either a 4B or a 64B write. Either case, as soon as we see the
    // joybus command bit (0x1), turn it off and parse the joybus packet
    // into internal memory (see JOYBUS_*).
    readCommand();
    if (RAM_BIT_TEST(PIF_CMD_L, PIF_CMD_L_JOYBUS)) {
      RAM_BIT_RESET(PIF_CMD_L, PIF_CMD_L_JOYBUS);

      regSave();
      joybusStatusInit();
      joybusCommandParse();
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
  RAM_BIT_RESET(STATUS, STATUS_RUNNING);  // no more in running mode, we're going to reset
  writeIO(REG_INT_EN, INT_A_EN);          // disable interrupt B
  writeIO(PORT_RESET, RESET_CPU_IRQ);     // trigger pre-NMI on VR4300

  interruptEpilog();
}

// 02:2C
void regRestore(void) {
  C = 1;
  if (!RAM_BIT_TEST(SAVE_C, 0))
    C = 0;
  X = RAM(SAVE_X);
  SB = readByte(SAVE_SBM);

  interruptEpilog();
}

// 02:3B
void interruptEpilog(void) {
  A = RAM(SAVE_A);
  B = SB;
  IME = 1;
}

// 03:06
// wait for interrupt A
void halt(void) {
  writeIO(REG_INT_EN, INT_A_EN);   // enable A and disable B

  // todo: should we do anything for halt/standby?
  // HALT = 1;

  if (RAM_BIT_TEST(STATUS, STATUS_RUNNING)) {
    writeIO(REG_INT_EN, INT_A_EN | INT_B_EN);   // reenable B (unless we're already resetting)
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
      if (!RAM_BIT_TEST(STATUS, STATUS_RUNNING)) {  // if we're not in running mode, start reset processs
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

  for (; (offset & 0xf) != 0; offset += (regionPAL ? -1 : +1)) {
    cicWriteBit(RAM_BIT_TEST(CIC_COMPARE_LO + offset, 0));
    bool c = cicReadBit();
    if (c != RAM_BIT_TEST(CIC_COMPARE_HI + offset, 0)) {
      signalError();
    }
  }
}

// 03:39
// disable interrupts and strobe the VR4300 NMI forever
void signalError(void) {
  IME = 0;
  u8 a = 0;  // incoming value doesn't really matter, as we're continuously toggling all bits anyway
  do {
    writeIO(PORT_RESET, a);
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

// 04:23
void joybusHandleError(void) {
  writeIO(PORT_JOYBUS_CTRL, 0);
  writeIO(PORT_JOYBUS_CTRL, 1);

  u8 n = readIO(PORT_JOYBUS_CHANNEL);
  u8 sb = readByte(JOYBUS_ADDR_U + n);
  u8 a;
  if ((readIO(PORT_JOYBUS_ERROR) & JOYBUS_ERROR_NOANSWER)) {
    a = JOYBUS_SENDERR_NO_DEVICE;
  } else {
    a = JOYBUS_SENDERR_TIMEOUT;
  }

  sb = incrementPtr(sb + 1);
  RAM(sb) += a;
  writeIO(PORT_JOYBUS_ERROR, JOYBUS_ERROR_RESET);
}

// 05:00
void boot(void) {
  IME = 0;

  memSwapRanges();  // copy OSINFO (including CIC seeds) from internal memory to external memory
  memZero(PIF_CMD_U);

  IME = 1;

  // wait for rom lockout bit of PIF status byte to become set
  do {
    readCommand();
  } while (!RAM_BIT_TEST(PIF_CMD_U, PIF_CMD_U_LOCKOUT));

  IME = 0;

  writeIO(PORT_ROM, ROM_LOCKOUT);  // enable ROM lockout
  writeIO(PORT_JOYBUS_CTRL, 1);
  joybusStatusInit();

  IME = 1;

  // wait for get checksum bit
  do {
    readCommand();
  } while (!RAM_BIT_TEST(PIF_CMD_U, PIF_CMD_U_GET_CHECKSUM));

  IME = 0;

  memSwapRanges();                        // copy PIF_CHECKSUM from external memory to internal memory
  RAM_BIT_SET(PIF_CMD_U, PIF_CMD_U_ACK);  // ack that we received the checksum

  IME = 1;

  // wait for check checksum bit
  do {
    readCommand();
  } while (!RAM_BIT_TEST(PIF_CMD_U, PIF_CMD_U_CHECK_CHECKSUM));

  IME = 0;

  RAM(PIF_CMD_U) = 0;
  if (reset == 0)  // only run on cold boot, not on reset
    cicCompareInit();

  // compare checksum received from CPU to that received from CIC
  // halt the CPU if it doesn't match
  for (u8 i = 0; i < 0xc; ++i) {
    u8 a = RAM(PIF_CHECKSUM + i);
    RAM(PIF_CHECKSUM + i) = 0;
    if (a != RAM(CIC_CHECKSUM + i)) {
      signalError();
    }
  }

  // Now start waiting until the CPU set the PIF_CMD_L_TERMINATE bit.
  // Notice that at this point the RESET button is still disabled (INT_B is disabled). 
  // The CPU will first initialize RDRAM (in IPL3) and then set the PIF_CMD_L_TERMINATE
  // bit (in game code[1]). After this bit is set, the RESET button will be enabled.
  // If the bit is not set before a timeout (approximately 5 seconds), PIF will freeze
  // the CPU.
  //
  // It is important that the RESET button is enabled only after the RDRAM is initialized;
  // in fact, RDRAM initialization will be skipped[2] by CPU/IPL3 on warm boots (resets,
  // so if RESET was pressed *before* or *during* RDRAM initialization, on next run
  // IPL3 would not initialize RDRAM and the console would likely freeze.
  //
  // [1] We don't know exactly why IPL3 doesn't set the PIF_CMD_L_TERMINATE bit itself, and
  //     leave it to game code. Maybe it's just a security by obscurity measure, so that the code
  //     setting the bit is "hidden" somewhere in game code, making harder to find
  //     out about it.
  // [2] Skipping RDRAM initialization on warm boots is a design decision to allow game code
  //     to store data in RDRAM that will be preserved across reset (see osAppNMIBuffer in
  //     libultra).
  bootTimerInit(BOOT_TIMER);

  IME = 1;

  for (;;) {
    readCommand();
    if (RAM_BIT_TEST(PIF_CMD_L, PIF_CMD_L_TERMINATE))
      break;

    bootTimerCheck();
  }

  // terminate boot process
  IME = 0;
  memZero(PIF_CMD_U);
  writeIO(REG_INT_EN, INT_A_EN | INT_B_EN);
  RAM_BIT_SET(STATUS, STATUS_RUNNING);
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
    RAM_BIT_SET(PIF_CMD_U, PIF_CMD_U_ACK);
    if (!(readIO(PORT_CIC) & BIT(3)))
      break;

    u8 b = RESET_TIMER_END - 1;
    if (increment8(&b) && increment8(&b))
      signalError();
  }

  writeIO(PORT_CIC, 1);

  while (!(readIO(PORT_RESET) & RESET_BUTTON)) // keep the reset on hold until the button is kept pressed
    ;

  IME = 0;
  writeIO(PORT_ROM, 0);  // disable ROM lockout
  writeIO(PORT_RESET, RESET_BUTTON | RESET_CPU_NMI);  // pulse NMI on VR4300, not sure why RESET_BUTTON is set here
  writeIO(PORT_RESET, RESET_BUTTON);
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
void bootTimerCheck(void) {
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
void joybusTransfer(void) {
  regSave();

  u8 n = 4;
  do {
    joybusTransferChannel(n);
    n = readIO(PORT_JOYBUS_CHANNEL);
  } while (n--);

  halt();

  regRestore();
}

// 07:1F
void joybusTransferChannel(u8 n) {
  writeIO(PORT_JOYBUS_CHANNEL, n);

  if (RAM_BIT_TEST(JOYBUS_STATUS + n, JOYBUS_STATUS_RESET)) {
    joybusResetChannel();
    return;
  }

  if (RAM_BIT_TEST(JOYBUS_STATUS + n, JOYBUS_STATUS_SKIP))
    return;

  u8 sb = readByte(JOYBUS_ADDR_U + n);
  if (joybusCopySendCount(JOYBUS_SEND_COUNT_U, &sb))
    return;

  joybusCopyRecvCount(JOYBUS_RECV_COUNT_U, &sb);

  for (;;) {
    RAM(JOYBUS_SEND_COUNT_L) -= 1;
    if (RAM(JOYBUS_SEND_COUNT_L) == 0xf) {
      RAM(JOYBUS_SEND_COUNT_U) -= 1;
      if (RAM(JOYBUS_SEND_COUNT_U) == 0xf)
        break;
    }

    do {
      if (!(readIO(PORT_JOYBUS_STATUS) & BIT(2))) {
        joybusHandleError();
        return;
      }
    } while (!(readIO(PORT_JOYBUS_STATUS) & JOYBUS_STATUS_CLOCK));

    writeIO(PORT_JOYBUS_WRITE, RAM(sb + 0));
    writeIO(PORT_JOYBUS_WRITE, RAM(sb + 1));
    sb += 2;
    if (!sb)
      sb = RAM_EXTERNAL;
  }

  joybusWait();

  for (;;) {
    RAM(JOYBUS_RECV_COUNT_L) -= 1;
    if (RAM(JOYBUS_RECV_COUNT_L) == 0xf) {
      RAM(JOYBUS_RECV_COUNT_U) -= 1;
      if (RAM(JOYBUS_RECV_COUNT_U) == 0xf)
        break;
    }

    do {
      if (!(readIO(PORT_JOYBUS_STATUS) & BIT(2))) {
        joybusHandleError();
        return;
      }
    } while (!(readIO(PORT_JOYBUS_STATUS) & JOYBUS_STATUS_CLOCK));
    RAM(sb + 0) = readIO(PORT_JOYBUS_READ);
    RAM(sb + 1) = readIO(PORT_JOYBUS_READ);
    sb += 2;
    if (!sb)
      sb = RAM_EXTERNAL;
  }

  writeIO(PORT_JOYBUS_CTRL, 1);
}

// 09:00
void joybusWait(void) {
  if (!RAM_BIT_TEST(PIF_CMD_L, BIT(2))) {
    joybusWriteStopBit();
  } else {
    if (!RAM_BIT_TEST(PIF_CMD_L, BIT(3))) {
      joybusWriteStopBit();
    }

    spin256();
  }
}

// 09:09
void joybusWriteStopBit(void) {
  writeIO(PORT_JOYBUS_CTRL, JOYBUS_CTRL_WRITESTOPBIT);
}

// 09:0D
void spin256(void) {
  u8 a = 0;
  do {
    SPIN(16);
  } while (++a & 0xf);
}

// 09:16
bool joybusCopySendCount(u8 b, u8* sb) {
  if (RAM_BIT_TEST(*sb, 3)) {
    return true;
  }

  if (RAM_BIT_TEST(*sb, 2)) {
    joybusResetChannel();
    return true;
  }

  joybusCopyByte(b, sb);
  return false;
}

// 09:1F
void joybusCopyRecvCount(u8 b, u8* sb) {
  RAM_BIT_RESET(*sb, 3);
  RAM_BIT_RESET(*sb, 2);

  joybusCopyByte(b, sb);
}

// 09:22
void joybusCopyByte(u8 b, u8* sb) {
  RAM(b + 0) = RAM(*sb + 0);
  RAM(b + 1) = RAM(*sb + 1);
  *sb = incrementPtr(*sb + 1);
}

// 09:2D
// Parse the JoyBus commands in external RAM. No joybus transactions
// are actually performed here. The commands are parsed and the pointer
// to the start of the frame of each channel is written to JOYBUS_ADDR_U/L[channel].
void joybusCommandParse(void) {
  u8 b = RAM_EXTERNAL;
  u8 n = 0;

  // Joybus commands in PIF-RAM are a sequences of frames, one frame per channel, in strict
  // order 0-4 (max 5 channels).
  //
  // This is a frame:
  //   TX RX tt[...] rr[...]
  //
  // where:
  //   * TX contains the number of bytes to transmit to the device (00-3F, top 2 bits are ignored)
  //   * RX contains the number of bytes to receive from the device (00-3F, top 2 bits are ignored)
  //   * tt is the data to transmit (must be exactly TX bytes)
  //   * rr is the space where received data will be written (must be exactly RX bytes)
  // 
  // There are some special values for TX that are used as special 1-byte "commands" (with no RX
  // or other data):
  //   * 0x00: skip current channel
  //   * 0xfd: reset current channel
  //   * 0xfe: abort processing (no other frames are parsed)
  //   * 0xff: nop
  //
  // After 5 frames, parsing is interrupted and the rest of RAM is ignored.
  //
  // At joybus execution time, the top 2 bits of TX are used for special functions:
  //   * Bit 7 set (TX values 0x80-0xFC): channel is skipped (identical behavior to 0x00)
  //   * Bit 6 set (TX values 0x40-0x7F): channel is reset (identical behavior to 0xfd)

  do {
    // Read tx count
    u8 tx = (RAM(b) << 4) | RAM(b + 1);

    // stop processing
    if (tx == 0xfe)
      break;

    // reset channel
    if (tx == 0xfd) {
      RAM_BIT_SET(JOYBUS_STATUS + n, JOYBUS_STATUS_RESET);
    }

    if (tx == 0xff || tx == 0xfd || tx == 0x00) {
      // fixed length commands
      b += 2;
      if (!b)
        break;

      if (tx != 0xff)
        ++n;
    } else {
      // variable length commands
      RAM(JOYBUS_ADDR_U + n) = b >> 4;
      RAM(JOYBUS_ADDR_L + n) = b & 0xf;
      RAM_BIT_RESET(JOYBUS_STATUS + n, JOYBUS_STATUS_SKIP);
      ++n;

      if (joybusCommandAdvance(&b, n)) {
        n--;
        RAM_BIT_RESET(JOYBUS_STATUS + n, JOYBUS_STATUS_RESET);
        RAM_BIT_SET(JOYBUS_STATUS + n, JOYBUS_STATUS_SKIP);
        break;
      }
    }
  } while (n != 5);
}

// 0B:00
bool joybusCommandAdvance(u8* address, u8 channel) {
  u8 b = *address;
  u8 n = channel;

  u8 sendU = RAM(b + 0) & 3;
  u8 sendL = RAM(b + 1);
  RAM(JOYBUS_ADDR_U + n) = sendU;   // NOTE: this is just used as scratch space (it will be overwritten later)
  RAM(JOYBUS_ADDR_L + n) = sendL;   // NOTE: this is just used as scratch space (it will be overwritten later)
  b += 2;
  if (!b)
    return true;

  u8 recvU = RAM(b + 0) & 3;
  u8 recvL = RAM(b + 1);
  u8 count = (sendU << 4) | sendL;
  count += (recvU << 4) | recvL;
  count += count;
  RAM(JOYBUS_ADDR_L + n) = count & 0xf;   // NOTE: this is just used as scratch space (it will be overwritten later)
  RAM(JOYBUS_ADDR_U + n) = count >> 4;    // NOTE: this is just used as scratch space (it will be overwritten later)

  u16 next = b + count + 2;
  if (next >= 0x100)
    return true;

  *address = next;
  return false;
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
void joybusResetChannel(void) {
  while (!(readIO(PORT_JOYBUS_STATUS) & JOYBUS_STATUS_CLOCK))
    writeIO(PORT_JOYBUS_ERROR, JOYBUS_ERROR_RESET);

  writeIO(PORT_JOYBUS_CTRL, 3);
  writeIO(PORT_JOYBUS_CTRL, 1);

  while (!(readIO(PORT_JOYBUS_STATUS) & JOYBUS_STATUS_CLOCK))
    ;
}

// 0C:32
// read byte from adjacent memory segments
u8 readByte(u8 address) {
  return (RAM(address) << 4) | RAM(address ^ 0x10);
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
    RAM(address) -= 1;
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
void cicCompareInit(void) {
  RAM_BIT_SET(OSINFO, OSINFO_RESET);

  cicCompareCreateSeed();

  writeIO(PORT_CIC, 3);
  spin256();
  writeIO(PORT_CIC, 1);

  for (u8 address = CIC_CHECKSUM_BUF; address < CIC_CHECKSUM_END; ++address)
    cicReadNibble(address);

  cicDecode(CIC_CHECKSUM_BUF);
  cicDecode(CIC_CHECKSUM_BUF);
  cicDecode(CIC_CHECKSUM_BUF);
  cicDecode(CIC_CHECKSUM_BUF);

  cicCompareExpandSeed();
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
void cicCompareExpandSeed(void) {
  RAM(CIC_COMPARE_LO) = 0;
  for (u8 offset = 2; offset < 0x10; ++offset) {
    u8 byte = (regionPAL ? romPAL : romNTSC)[RAM(CIC_COMPARE_LO)];
    RAM(CIC_COMPARE_LO) += 1;
    RAM(CIC_COMPARE_LO + offset) = byte & 0xf;
    RAM(CIC_COMPARE_HI + offset) = byte >> 4;
  }
  cicWriteNibble(CIC_COMPARE_LO + 1);
  cicWriteNibble(CIC_COMPARE_HI + 1);
  regInitSB();
}

// 0F:1B
void cicCompareCreateSeed(void) {
  writeIO(PORT_RNG, RNG_START);

  // Keep incrementing CIC_COMPARE_LO+9 until the RNG bit is 0.
  // When it becomes 1, stop incrementing. We assume that this is
  // a way to obtain a random number in CIC_COMPARE_LO+9, which is
  // then used to drive the CIC compare communication.
  do {
    u8 b = CIC_COMPARE_LO + 9;
    increment8(&b);
  } while (!(readIO(PORT_RNG) & RNG_DATA));

  writeIO(PORT_RNG, 0);

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

void readRegion(void) {
  printf("r region\n");
  int value = scanValue();
  printf("  %x\n", value);
  regionPAL = value;
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
  readRegion();
  start();
}

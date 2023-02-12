#include "cmodel.h"

#include <stdio.h>
#include <stdlib.h>

bool regionPAL = 0;
bool challenge = 0;
const u8* romSecret = NULL;

const u8 romNTSC[] = {
    0x19, 0x4a, 0xf1, 0x88, 0xb5, 0x5a, 0x71,
    0xc3, 0xde, 0x61, 0x10, 0xed, 0x9e, 0x8c,
};

const u8 romPAL[] = {
    0x14, 0x2f, 0x35, 0xf1, 0x82, 0x21, 0x77,
    0x11, 0x99, 0x88, 0x15, 0x17, 0x55, 0xca,
};

const u8 rom6101[] = {
    0x3f, 0x3f, 0x45, 0xcc, 0x73, 0xee, 0x31, 0x7a,
};

const u8 rom7102[] = {
    0x3f, 0x3f, 0x44, 0x16, 0x0e, 0xc5, 0xd9, 0xaf,
};

const u8 rom6102[] = {
    0x3f, 0x3f, 0xa5, 0x36, 0xc0, 0xf1, 0xd8, 0x59,
};

const u8 rom6103[] = {
    0x78, 0x78, 0x58, 0x6f, 0xd4, 0x70, 0x98, 0x67,
};

const u8 rom6105[] = {
    0x91, 0x91, 0x86, 0x18, 0xa4, 0x5b, 0xc2, 0xd3,
};

const u8 rom6106[] = {
    0x85, 0x85, 0x2b, 0xba, 0xd4, 0xe6, 0xeb, 0x74,
};

void start(void);
void signalError(void);
void writeBit0(void);
void writeBit(bool a);
bool readBit(void);
bool readBitTail(void);
bool loadSecretBit(u8* sb);
bool readBitDelay(void);
void readNibble(u8 b);
void writeNibble(u8 b);
void cicEncodeChecksum(u8 b);
void cicEncode(u8 b);
void cicEncodeSeed(void);
void loadSeed(void);
void loadChecksum(void);
void loadSecret(u8 b, u8 sb);
void cicReset(void);
void cicLoop(void);
void cicCompareRound(u8 address);
void start2(void);
void prefixChecksum(void);
void nop3(void);
void cicChallenge(void);
void cicChallengeExec(void);
void cicChallengeExec6105(u8 a, u8 b);

// 00:00
void start(void) {
  writeIO(2, 1);
  if (readIO(2) & BIT(2)) {
    if (!readBitDelay()) {
      for (;;)
        fatalError();
    }
  } else {
    writeIO(2, 0);
    readBit();
  }

  writeBit(regionPAL);
  writeBit(0);
  writeBit(1);
  loadSeed();
  cicEncodeSeed();

  for (u8 b = 0x0a; b < 0x10; ++b)
    writeNibble(b);

  loadChecksum();
  prefixChecksum();
  cicEncodeChecksum(0x00);
  writeBit0();

  for (u8 b = 0x00; b < 0x10; ++b)
    writeNibble(b);

  start2();
}

// 01:02
void signalError(void) {
  for (;;)
    fatalError();
}

// 01:04
void writeBit0(void) {
  writeBit(0);
}

// 01:06
void writeBit(bool a) {
  while (readIO(2) & BIT(1))
    ;

  writeIO(2, a);

  while (!(readIO(2) & BIT(1)))
    ;

  writeIO(2, 1);
}

// 01:12
bool readBit(void) {
  writeIO(0xf, 0);

  while (readIO(2) & BIT(1))
    ;

  return readBitTail();
}

// 01:1B
bool readBitTail(void) {
  bool c = readIO(2) & BIT(0);

  writeIO(0xf, 1);

  while (!(readIO(2) & BIT(1)))
    ;

  return c;
}

// 01:26
bool loadSecretBit(u8* sb) {
  bool c = romSecret[(*sb >> 3) & 7] & BIT(7 - (*sb & 7));

  if (!++*sb)
    *sb = 0xf0;

  return c;
}

// 01:32
bool readBitDelay(void) {
  writeIO(0xf, 0);

  while (readIO(2) & BIT(1))
    ;

  SPIN(3);

  return readBitTail();
}

// 02:00
void readNibble(u8 b) {
  RAM(b) = 0xf;
  if (!readBit())
    RAM_BIT_RESET(b, 3);
  if (!readBit())
    RAM_BIT_RESET(b, 2);
  if (!readBit())
    RAM_BIT_RESET(b, 1);
  if (!readBit())
    RAM_BIT_RESET(b, 0);
}

// 02:0F
void writeNibble(u8 b) {
  writeBit(RAM_BIT_TEST(b, 3));
  writeBit(RAM_BIT_TEST(b, 2));
  writeBit(RAM_BIT_TEST(b, 1));
  writeBit(RAM_BIT_TEST(b, 0));
}

// 02:20
void cicEncodeChecksum(u8 b) {
  cicEncode(b);
  cicEncode(b);
  cicEncode(b);
  cicEncode(b);
}

// 02:2B
void cicEncode(u8 b) {
  for (; (b & 0xf) != 0xf; ++b)
    RAM(b + 1) += RAM(b) + 1;
}

// 02:2F
void cicEncodeSeed(void) {
  RAM(0x0a) = 0xb;
  RAM(0x0b) = 5;
  cicEncode(0x0a);
  cicEncode(0x0a);
}

// 03:00
void loadSeed(void) {
  loadSecret(0x0c, 0x40);
}

// 03:06
void loadChecksum(void) {
  loadSecret(0x04, 0x50);
}

// 03:0B
void loadSecret(u8 b, u8 sb) {
  do {
    RAM(b) = 0xf;
    if (!loadSecretBit(&sb))
      RAM_BIT_RESET(b, 3);
    if (!loadSecretBit(&sb))
      RAM_BIT_RESET(b, 2);
    if (!loadSecretBit(&sb))
      RAM_BIT_RESET(b, 1);
    if (!loadSecretBit(&sb))
      RAM_BIT_RESET(b, 0);
  } while (++b & 0xf);
}

// 03:1F
void cicReset(void) {
  RAM(0x00) = 0;
  RAM(0x10) = 0;
  u8 a = 0;

  do {
    for (u8 x = 0; x < 0x10; ++x)
      nop3();
  } while ((++a & 0xf) || ++RAM(0x00) || ++RAM(0x10));

  // undo final increment
  RAM(0x10)--;

  writeBit0();
}

// 04:0E
void cicLoop(void) {
  for (;;) {
    if (readBit()) {
      if (!readBit())
        cicChallenge();
      else
        cicReset();
    } else {
      if (readBit())
        signalError();

      cicCompareRound(0x00);
      cicCompareRound(0x00);
      cicCompareRound(0x00);
      cicCompareRound(0x10);
      cicCompareRound(0x10);
      cicCompareRound(0x10);

      u8 b = RAM(0x17);
      if (!b)
        b = 1;

      do {
        bool c = readBit();
        writeBit(RAM_BIT_TEST(0x10 + b, 0));
        if (c != RAM_BIT_TEST(0x00 + b, 0))
          signalError();

        b += regionPAL ? -1 : +1;
      } while (b & 0xf);
    }
  }
}

// 05:00
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

// 06:00
void start2(void) {
  RAM(0x00) = 0;
  RAM(0x11) = 0xb;
  u8 b = 0x02;

  do {
    u8 a = RAM(0x00)++;
    u8 byte = (regionPAL ? romPAL : romNTSC)[a];
    RAM(b) = byte & 0xf;
    RAM(b ^ 0x10) = byte >> 4;
  } while (++b & 0xf);

  readNibble(0x01);
  readNibble(0x11);

  cicLoop();
}

// 06:22
void prefixChecksum(void) {
  u8 a = 0, x = 0;  // todo: incoming values?
  SWAP(a, x);

  while (readIO(2) & BIT(1)) {
    if (!(++a & 0xf)) {
      SWAP(a, x);
      if (++a & 0xf) {
        SWAP(a, x);
      } else {
        SWAP(a, RAM(0x02));
        if (++a & 0xf)
          SWAP(a, RAM(0x02));
      }
    }
  }

  a += RAM(0x02);
  RAM(0x00) = a;
  RAM(0x01) = x;
}

// 06:37
void nop3(void) {
  // nop x 3
}

// 07:00
void cicChallenge(void) {
  u8 b = 0x20;
  RAM(0x20) = 0xa;
  writeNibble(b);
  writeNibble(b);

  for (u8 x = 0; x < 0x20; ++x) {
    readNibble(b);
    ++b;
  }

  cicChallengeExec();
  b = 0x20;
  writeBit0();

  for (u8 x = 0; x < 0x20; ++x) {
    writeNibble(b);
    ++b;
  }
}

// 07:2C
void cicChallengeExec(void) {
  u8 b = 0x20;

  if (challenge) {
    cicChallengeExec6105(5, b);
  } else {
    for (u8 x = 0; x < 0x20; ++x) {
      RAM(b) ^= 0xf;
      ++b;
    }
  }
}

// 09:00
void cicChallengeExec6105(u8 a, u8 b) {
  bool c = 1;

  for (u8 x = 0; x < 0x20; ++x) {
    u8 y = a + RAM(b);
    if (!RAM_BIT_TEST(b, 0))
      y += 8;
    if (!(a & BIT(1)))
      y += 4;
    u8 z = y + y;
    if (!c)
      z += 7;
    a = (y & 0xf) + (z & 0xf) + c;
    c = a & 0x10;
    a = ~a;
    RAM(b) = a;
    ++b;
  }
}

// end CIC ROM

FILE* input;

bool initCIC(int cic) {
  regionPAL = 0;
  challenge = 0;
  romSecret = NULL;

  switch (cic) {
    case 6101:
      romSecret = rom6101;
      break;
    case 7102:
      regionPAL = 1;
      romSecret = rom7102;
      break;
    case 6102:
      romSecret = rom6102;
      break;
    case 7101:
      regionPAL = 1;
      romSecret = rom6102;
      break;
    case 6103:
      romSecret = rom6103;
      break;
    case 7103:
      regionPAL = 1;
      romSecret = rom6103;
      break;
    case 6105:
      challenge = 1;
      romSecret = rom6105;
      break;
    case 7105:
      regionPAL = 1;
      challenge = 1;
      romSecret = rom6105;
      break;
    case 6106:
      romSecret = rom6106;
      break;
    case 7106:
      regionPAL = 1;
      romSecret = rom6106;
      break;
    default:
      return false;
  }

  return true;
}

int scanValue(void) {
  // remove comments before next token
  int num;
  do {
    num = 0;
    fscanf(input, " #%n%*[^\n]", &num);
  } while (num > 0);

  int next = fgetc(input);
  if (next == 'q') {
    printf("  %c\n", next);
    exit(0);
  }
  ungetc(next, input);

  int value;
  if (1 != fscanf(input, "%i", &value)) {
    printf("scanf error\n");
    exit(3);
  }
  return value;
}

void readCIC(void) {
  printf("r cic\n");
  int value = scanValue();
  printf("  %x\n", value);
  if (!initCIC(value)) {
    printf("unknown cic\n");
    exit(4);
  }
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

void fatalError(void) {
  printf("fatal error\n");
  exit(1);
}

int main(int argc, char* argv[]) {
  if (argc > 1) {
    input = fopen(argv[1], "r");
  } else {
    input = stdin;
  }
  readCIC();
  start();
}

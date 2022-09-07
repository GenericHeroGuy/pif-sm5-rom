# Sharp SM5Kx disassembler by GenericHeroGuy

import os
import argparse

parser = argparse.ArgumentParser("disassembler")
parser.add_argument("path", \
                    help="path to ROM image to disassemble")
parser.add_argument("-p", "--pseudocode", \
                    help="pseudocode output: s (short pseudocode), f (long pseudocode)", \
                    choices=["s", "f"])
args = parser.parse_args()

filePath = args.path
pseudoOption = args.pseudocode

if not os.path.exists(filePath):
    exit("Unable to find file " + fn)

optable = ["NOP",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",  "ADX",
           "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",  "LAX",
           "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX", "LBLX",
           "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX", "LBMX",
           "RM",   "RM",   "RM",   "RM",   "SM",   "SM",   "SM",   "SM",   "TM",   "TM",   "TM",   "TM",   "TPB",  "TPB",  "TPB",  "TPB",
           "LDA",  "LDA",  "LDA",  "LDA",  "EXC",  "EXC",  "EXC",  "EXC",  "EXCI", "EXCI", "EXCI", "EXCI", "EXCD", "EXCD", "EXCD", "EXCD",
           "RC",   "SC",   "ID",   "IE",   "EXAX", "ATX",  "EXBM", "EXBL", "EX",   "",     "PAT",  "TABL", "TA",   "TB",   "TC",   "TAM",
           "INL",  "OUTL", "ANP",  "ORP",  "IN",   "OUT",  "STOP", "HALT", "INCB", "COMA", "ADD",  "ADC",  "DECB", "RTN",  "RTNS", "RTNI",
           "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",
           "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",
           "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",
           "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",   "TR",
           "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",
           "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",  "TRS",
           "TL",   "TL",   "TL",   "TL",   "TL",   "TL",   "TL",   "TL",   "TL",   "TL",   "TL",   "TL",   "TL",   "TL",   "TL",   "TL",
           "CALL", "CALL", "CALL", "CALL", "CALL", "CALL", "CALL", "CALL", "CALL", "CALL", "CALL", "CALL", "CALL", "CALL", "CALL", "CALL"
]
                                             #vvv DTA on SM5Lx
exoptable = ["???",  "???",  "TT",   "DR",   "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",
             "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???",  "???"
]

opbytes = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
           2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
           2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
]

pseudocodes1 = {
    "TR": "; Relative jump to {0}",
    "TL": "; Long jump to {0}",
    "TRS": "; Push PC, jump to {0}",
    "CALL": "; Push PC, jump to {0}",
    "RTN": "; Pop PC",
    "RTNS": "; Pop PC, skip next instruction",
    "RTNI": "; Pop PC, enable interrupts",

    "LAX": "; Load {0} to A",
    "LBMX": "; Load {0} to Bm",
    "LBLX": "; Load {0} to Bl",
    "LDA": "; Load M to A, XOR Bm with {0}",
    "EXC": "; Exchange A with M, XOR Bm with {0}",
    "EXCI": "; Exchange A with M, XOR Bm with {0}, increment Bl, skip if Bl = 0",
    "EXCD": "; Exchange A with M, XOR Bm with {0}, decrement Bl, skip if Bl = 15",
    "EXAX": "; Exchange A with X",
    "ATX": "; Load A to X",
    "EXBM": "; Exchange A with Bm",
    "EXBL": "; Exchange A with Bl",
    "EX": "; Exchange B with SB",

    "ADX": "; Add {0} to A, skip if overflow",
    "ADD": "; Add M to A",
    "ADC": "; Add M + carry to A, set carry flag and skip if overflow",
    "COMA": "; Complement A",
    "INCB": "; Increment Bl, skip if Bl = 0",
    "DECB": "; Decrement Bl, skip if Bl = 15",

    "TAM": "; Skip if A = M",
    "TC": "; Skip if carry set",
    "TM": "; Skip if bit {0} of M = 1",
    "TABL": "; Skip if A = Bl",
    "TPB": "; Skip if bit {0} of Pj/Rj = 1",
    "TA": "; Skip if IFA = 1, clear IFA",
    "TB": "; Skip if IFB = 1, clear IFB",
    "TT": "; Skip if IFT = 1, clear IFT",

    "SM": "; Set bit {0} of M",
    "RM": "; Reset bit {0} of M",
    "SC": "; Set carry flag",
    "RC": "; Clear carry flag",
    "IE": "; Enable interrupts",
    "ID": "; Disable interrupts",

    "INL": "; Load P1 to A",
    "OUTL": "; Load A to P0",
    "ANP": "; Set Pj to Pj AND A",
    "ORP": "; Set Pj to Pj OR A",
    "IN": "; Load Pj/Rj to A/XA",
    "OUT": "; Load A/XA to Pj/Rj",

    "PAT": "; Push PC, set Pu to 4, set Pl to XA, load ROM value at PC to XA, pop PC",

    "DR": "; Clear divider",

    "STOP": "; Enter standby mode (STOP)",
    "HALT": "; Enter standby mode (HALT)",
    "NOP": "; No operation"
}

pseudocodes2 = {
    "TR": "; Pl <- {0}",
    "TL": "; PC <- {0}",
    "TRS": "; Push, PC <- {0}",
    "CALL": "; Push, PC <- {0}",
    "RTN": "; Pop",
    "RTNS": "; Pop, skip",
    "RTNI": "; Pop, IME <- 1",

    "LAX": "; A <- {0}",
    "LBMX": "; Bm <- {0}",
    "LBLX": "; Bl <- {0}",
    "LDA": "; A <- M, Bm <- Bm XOR {0}",
    "EXC": "; A <> M, Bm <- Bm XOR {0}",
    "EXCI": "; A <> M, Bm <- Bm XOR {0}, Bl++, skip if Bl = 0",
    "EXCD": "; A <> M, Bm <- Bm XOR {0}, Bl--, skip if Bl = 15",
    "EXAX": "; A <> X",
    "ATX": "; X <- A",
    "EXBM": "; A <> Bm",
    "EXBL": "; A <> Bl",
    "EX": "; B <> SB",

    "ADX": "; A <- A + {0}, skip if Cy = 1",
    "ADD": "; A <- A + M",
    "ADC": "; A <- A + M + C, C <- Cy, skip if Cy = 1",
    "COMA": "; A <- ~A",
    "INCB": "; Bl++, skip if Bl = 0",
    "DECB": "; Bl--, skip if Bl = 15",

    "TAM": "; Skip if A = M",
    "TC": "; Skip if C = 1",
    "TM": "; Skip if M.{0} = 1",
    "TABL": "; Skip if A = Bl",
    "TPB": "; Skip if Pj/Rj.{0} = 1",
    "TA": "; Skip if IFA = 1, IFA <- 0",
    "TB": "; Skip if IFB = 1, IFB <- 0",
    "TT": "; Skip if IFT = 1, IFT <- 0",

    "SM": "; M.{0} <- 1",
    "RM": "; M.{0} <- 0",
    "SC": "; C <- 1",
    "RC": "; C <- 0",
    "IE": "; IME <- 1",
    "ID": "; IME <- 0",

    "INL": "; A <- P1",
    "OUTL": "; P0 <- A",
    "ANP": "; Pj <- Pj AND A",
    "ORP": "; Pj <- Pj OR A",
    "IN": "; A/XA <- Pj/Rj",
    "OUT": "; Pj/Rj <- A/XA",

    "PAT": "; Push, Pu <- 4, Pl <- XA, XA <- ROM(PC), pop",

    "DR": "; Divider <- 0",

    "STOP": "",
    "HALT": "",
    "NOP": ""
}

length = os.path.getsize(filePath)
pc = 0
with open(filePath, "r+b") as f:
    for i in range(0, length):
        # exit if PC == end of file
        if pc == length:
            exit()

        # get first instruction byte
        opcode = int.from_bytes(f.read(1), byteorder="little")

        # get second instruction byte
        opcode2 = 0
        if opbytes[opcode] == 2:
            opcode2 = int.from_bytes(f.read(1), byteorder="little")

        # get mnemonic
        if opcode == 0x69: #prefix byte?
            mnemonic = exoptable[opcode2]
        else:
            mnemonic = optable[opcode]

        # get operand
        operand = ""
        if opcode != 0x00 and opcode <= 0x3F:
            operand = str(opcode & 0x0F)

        if opcode >= 0x40 and opcode <= 0x5F:
            operand = str(opcode & 0x03)

        if opcode >= 0x80 and opcode <= 0xBF:
            operand = f"${opcode & 0x3F:02x}"

        if opcode >= 0xC0 and opcode <= 0xDF:
            operand = f"$01:{(opcode & 0x3F) << 1:02x}"

        if opcode >= 0xE0:
            operand = f"${(opcode & 0x0F) << 2 | (opcode2 & 0xC0) >> 6:02x}:{opcode2 & 0x3F:02x}"

        # get pseudocode
        if pseudoOption == "f":
            pseudocode = pseudocodes1[mnemonic].format(operand)
        elif pseudoOption == "s":
            pseudocode = pseudocodes2[mnemonic].format(operand)
        else:
            pseudocode = ""

        # string formatting time!
        # PC
        output = f"${(pc & 0xFC0) >> 6:02x}:{pc & 0x03F:02x}: "
        # first opcode byte
        output += f"${opcode:02x} "
        # second opcode byte
        output += f"${opcode2:02x}" if opbytes[opcode] == 2 else ""
        # mnemonic and operand
        output = f"{output:<16} {mnemonic:s} {operand:s}"
        # pseudocode
        if pseudocode != "":
            output = f"{output:<32}{pseudocode}"

        # finally, print the line
        print(output)

        # advance PC
        pc += opbytes[opcode]

f.close()

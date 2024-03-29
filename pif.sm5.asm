architecture n64.pif

////////////////////////////////////////
// macros (actually defines)

// low 4 bits of value
define low(value) = {value} & $0f

// high 4 bits of value
define high(value) = ({value} & $f0) >> 4

// convert to bitmask
define bit(value) = 1 << {value}

////////////////////////////////////////
// compile-time constants (see Makefile)

// PIF region
constant regionNTSC = region == 0
constant regionPAL = region == 1
assert(regionNTSC || regionPAL)

////////////////////////////////////////
// RAM variables

// low 4 bits of PIF status byte (from VR4300 perspective)
constant pifCommandLo = $ff
constant pifCommandLo.joybus = 0        // run joybus protocol
constant pifCommandLo.challenge = 1     // send challenge to CIC
constant pifCommandLo.unknown = 2       // changes behavior of joybus console stop bit?
constant pifCommandLo.terminateBoot = 3 // terminate boot process
// high 4 bits of PIF status byte
constant pifCommandHi = $fe
constant pifCommandHi.lockRom = 0      // lock out PIF-ROM
constant pifCommandHi.readChecksum = 1 // when set by VR4300, copy checksum from PIF-RAM
constant pifCommandHi.testChecksum = 2 // when set by VR4300, begin comparing checksum
constant pifCommandHi.readAck = 3      // set by PIF to acknowledge reading checksum

// 15-byte area for holding challenge/response data
constant cicChallengeData = $e0 // .. $fd

// number of bytes to receive from CIC during challenge/response
constant cicResponseCount = $df
// number of bytes to send to CIC during challenge/response
constant cicChallengeCount = $dd


// used to generate the random CIC compare nibbles
constant cicRngTimerLo = $69
constant cicRngTimerHi = $68

// first nibbles of each CIC compare data array, randomly generated
constant cicRandomHi = $71
constant cicRandomLo = $61

// 14-nibble array for CIC compare data, one page for lo/hi nibble
constant cicCompareLo = $62 // $63, $64, $65, $66, $67, $68, $69, $6a, $6b, $6c, $6e, $6f
constant cicCompareHi = $72 // $73, $74, $75, $76, $77, $78, $79, $7a, $7b, $7c, $7e, $7f
constant cicCompareLoEnd = $6f
constant cicCompareHiEnd = $7f

// pointer for PAT table
constant patPointer = $60

// status nibble
constant pifStatus = $5e
constant pifStatus.doChallenge = 1  // if set, run CIC challenge protocol
constant pifStatus.resetPending = 3 // if clear, reset is pending

// 6-nibble boot timer
constant bootTimer = $4f // $4e, $4d, $4c, $4b, $4a
constant bootTimerEnd = $4a

// 6-byte checksum from IPL2
constant ipl2Checksum = $34 // $35, $36, $37, $38, $39, $3a, $3b, $3c, $3d, $3e, $3f

// 8-byte checksum from CIC
// only the last 6 bytes are used
constant cicChecksum = $20 // $21, $22, $23, $24, $25, $26, $27, $28, $29, $2a, $2b, $2c, $2d, $2e, $2f
constant cicChecksumEnd = $2f

// copy of Tx and Rx count for joybus transfer
constant txCountHi = $22
constant txCountLo = $23
constant rxCountHi = $32
constant rxCountLo = $33

// OS info
constant osInfo = $1b
constant osInfo.64dd = 3    // if set, 64DD is connected
constant osInfo.version = 2 // always set
constant osInfo.reset = 1   // set on cold boot, not visible to VR4300 until next reset

// 6-nibble CIC seed, used to seed VR4300 checksums
// only the last 4 nibbles are used ($1b is overwritten by osInfo, $1a is never sent to VR4300)
constant cicSeed = $1a // $1b, $1c, $1d, $1e, $1f

// 4-nibble reset timer
constant resetTimer = $0f // $0e, $0d, $0c

// delay before receiving response from challenge/response protocol
constant cicResponseTimerHi = $0a
constant cicResponseTimerLo = $0b

// status for each channel
constant joybusChStatus = $40 // $41, $42, $43, $44, $45
constant joybusChStatusEnd = $45
constant joybusChStatus.doTransaction = 3 // if bit clear, do transaction
constant joybusChStatus.reset = 0         // if bit set, reset channel

// pointer to data for each channel's joybus transaction
constant joybusDataPtrLo = $00 // $01, $02, $03, $04, $05
constant joybusDataPtrHi = $10 // $11, $12, $13, $14, $15

// saved registers
constant saveA = $56
constant saveBm = $57
constant saveBl = $47
constant saveX = $58
constant saveC = $59

////////////////////////////////////////
// I/O ports & bits

// joybus status port, provides status of currently selected channel
constant joyStatusPort = 3
constant joyStatusPort.error = 2 // if set, error occured during communication
constant joyStatusPort.busy = 3  // if set, joybus is busy

// joybus error port. when written to, acknowledges error
constant joyErrorPort = 4
constant joyErrorPort.type = 3

// CIC data port
constant cicPort = 5
constant cicPort.data = 0     // bidirectional data pin (CIC_15 on cart)
constant cicPort.clock = 1    // data clock (CIC_14 on cart)
constant cicPort.response = 3 // response from CIC

// ROM locking port
constant romPort = 6
constant romPort.lock = 0 // if set, lock out VR4300 from PIF-ROM

// RCP command ID port
constant rcpPort = 7
constant rcpPort.cmdType = 3 // if set, read. if clear, write
constant rcpPort.cmdSize = 2 // if set, 64B. if clear, 4B

// reset button port
constant resetPort = 8
constant resetPort.nmi = 0     // if set, trigger NMI on VR4300
constant resetPort.irq = 1     // if set, trigger pre-NMI IRQ on VR4300
constant resetPort.pressed = 3 // set when reset button is pressed, triggers interrupt B

// RNG port
constant rngPort = 9
constant rngPort.enable = 0
constant rngPort.output = 3

// joybus channel select port
constant joyChannelPort = 10

// interrupt enable mode register
constant intRegister = 14
constant intRegister.A = 0 // enable interrupt A (RCP SI)
constant intRegister.B = 2 // enable interrupt B (Reset button)

////////////////////////////////////////
// Enums

constant CIC_WRITE_0 = {bit(cicPort.clock)}
constant CIC_WRITE_1 = {bit(cicPort.data)} | {bit(cicPort.clock)}
constant CIC_WRITE_OFF = {bit(cicPort.data)}

constant JOYBUS_ERR_NO_DEVICE = {bit(3)}
constant JOYBUS_ERR_TIMEOUT = {bit(2)}

////////////////////////////////////////
// Page 0 (reset vector)
origin $000

ResetVector: // 00:00
	lax CIC_WRITE_OFF
	lblx cicPort
	out // P5 <- 1

// enable interrupt A (RCP)
	lblx intRegister
	out // RE <- 1
	trs TRS_SetSB // SB = saveA

// clear IPL2 checksum
	lbx ipl2Checksum
	trs ClearMemPage

// write CIC type to pifStatus
	lbx pifStatus
	trs TRS_CicReadNibble
	decb

// if type == 1 (NTSC) or 5 (PAL), cart
	lax 1 | region<<2
	tam
	tr NotCart

	lax {bit(osInfo.version)}
	tr WriteCicType

NotCart: // 00:12
// if type == 9 (NTSC) or 13 (PAL), 64DD
	lax 9 | region<<2
	tam
	trs TRS_SignalError // not cart or 64DD

	lax {bit(osInfo.64dd)} | {bit(osInfo.version)}

WriteCicType: // 00:16
	exc 0

// clear all of PIF-RAM
	lbx $80
	tr +
ClearPifRam: // 00:1A
	exbm
+;	trs ClearMemPage
	exbm
	adx 1
	tr ClearPifRam

// write CIC seed to [$1a..$1f]
// this will be sent to VR4300 to seed its checksum algorithms
	lbx cicSeed
-;	trs TRS_CicReadNibble
	tr -

// descramble seed (2 rounds)
	lblx {low(cicSeed)}
	trs TRS_CicDescramble
	lblx {low(cicSeed)}
	trs TRS_CicDescramble

// copy CIC type to osInfo, and clear pifStatus
// this will eventually be written to [$cb] (PIF-RAM $24 bits 0-3)
	lbx pifStatus
	lda 0
	rm pifStatus.doChallenge // clear challenge bit
	rm pifStatus.resetPending // set pending reset flag (for some reason?)
	lbx osInfo
	excd 0

	rc // reset carry, this is a cold boot

Reboot: // 00:30
	lbx pifCommandHi
	sm pifCommandHi.readAck
	ie
	tl SystemBoot

////////////////////////////////////////
// page 1 (TRS vectors)
origin $040

TRS_IncrementByte: // 01:00
	tl IncrementByte
TRS_SwapMem: // 01:02
	tl SwapMem
TRS_CicWriteNibble: // 01:04
	tl CicWriteNibble
TRS_LongDelay: // 01:06
	tl LongDelay
TRS_CicCompareRound: // 01:08
	tl CicCompareRound
TRS_CicDescramble: // 01:0A
	tl CicDescramble
TRS_SignalError: // 01:0C
	tl SignalError
TRS_CicReadNibble: // 01:0E
	tl CicReadNibble
TRS_SetSB: // 01:10
	tl SetSB


// write $fb to [B] and zero rest of page
InitBootTimer: // 01:12
	lax 15
	exci 0
	lax 11
	exci 0

// zero memory from B to end of page
ClearMemPage: // 01:16
	lax 0
	exci 0
	tr ClearMemPage
	rtn

// fill [$40..$45] with 8
ResetJoybusTransactions: // 01:1A
	lbx joybusChStatusEnd
-;	lax {bit(joybusChStatus.doTransaction)}
	excd 0
	tr -
	rtn

CicWriteBit: // 01:20
// save Bl in X, set Bl to CIC port
	exax
	lax cicPort
	exbl
	exax

	out // P5 <- CIC_WRITE_x

// short delay
	lax 11
-;	adx 1
	tr -
	tr CicEndIo

	nop // filler (TRS vectors must be 2-byte aligned)

CicReadBit: // 01:2A
// save Bl in X, set Bl to CIC port
	lax cicPort
	exbl
	exax

	lax CIC_WRITE_1
	out // P5 <- 3

// short delay
	lax 11
-;	adx 1
	tr -

// set carry to CIC response bit
	sc
	tpb cicPort.response // test P5.3
	rc

CicEndIo: // 01:35
	lax CIC_WRITE_OFF
	out // P5 <- 1

// another delay
	lax 12
-;	adx 1
	tr -

// restore Bl from X
	exax
	exbl
	rtn

////////////////////////////////////////
// page 2 (interrupt vectors)
origin $080

// triggered by RCP
InterruptA: // 02:00
	ex // B = saveA
	exci 0 // [saveA] <- A, Bl <- rcpPort
	tr InterruptACont
	nop

// triggered by reset button
InterruptB: // 02:04
	ex // B = saveA
	exc 0 // [saveA] <- A

// signal pending reset
	lblx {low(pifStatus)} // Bl <- intRegister
	rm pifStatus.resetPending

// disable interrupt B
	lax {bit(intRegister.A)}
	out // RE <- 1

// trigger pre-NMI IRQ on VR4300
	lblx resetPort
	lax {bit(resetPort.irq)}
	out // P8 <- 2
	tr InterruptExit

InterruptACont: // 02:0E
	tpb rcpPort.cmdType // test P7.3
	tr RcpWrite
	tpb rcpPort.cmdSize // test P7.2
	tr RcpWaitForRead // 4B read; halt and do nothing

// 64B read; unless CIC challenge has been requested, do joybus transactions
	lbx pifCommandLo
	tm pifCommandLo.challenge
	tl JoybusDoTransactions

// unless reset is pending, run CIC challenge protocol
	lbx pifStatus
	tm pifStatus.resetPending
	tr RcpWaitForRead
	tl SetChallengeBit

// 64B or 4B write; halt until it's finished
RcpWrite: // 02:1D
	call HaltCpu

// if bit 0 of PIF command byte is set, prepare for future joybus transactions
	lbx pifCommandLo
	tm pifCommandLo.joybus
	tr SkipPrepJoy

	rm pifCommandLo.joybus
	call SaveRegs
	lbx joybusDataPtrHi
	ex
	trs ResetJoybusTransactions // [$40..$45] = 8
	call PrepareJoybusTransactions

// restore registers saved by joybus protocol
JoybusExit: // 02:2C
// restore C
	lbx saveC
	sc
	tm 0
	rc

// restore X
	decb
	excd 0
	exax

// restore SB
	call ReadSplitByte
	tr InterruptExit

SkipPrepJoy: // 02:37
	lbmx {high(saveA)}
	tr InterruptExit

// halt so RCP can read PIF-RAM
RcpWaitForRead: // 02:39
	call HaltCpu

InterruptExit: // 02:3B
	lblx {low(saveA)}
	exc 0
	ex
	rtni

////////////////////////////////////////
// page 3 (standby exit vector)
origin $0C0

StandbyExit: // 03:00
	nop

// unless reset is pending, re-enable interrupts (is the check necessary?)
	tm pifStatus.resetPending
	rtn
	lax {bit(intRegister.A)} | {bit(intRegister.B)}
	out // RE <- 5
	rtn

// halting acknowledges command from RCP, allowing it to read/write PIF-RAM
HaltCpu: // 03:06
	lblx {low(pifStatus)} // Bl <- 14
	lax {bit(intRegister.A)}
	out // RE <- 1
	halt
	tr StandbyExit


MainLoopStart: // 03:0B
	ie
MainLoop: // 03:0C
	lbx pifStatus

// check if reset is pending
	tm pifStatus.resetPending
	tl ResetSystem

// check if challenge protocol should run
	tm pifStatus.doChallenge
	tr CicCompare

	rm pifStatus.doChallenge
	tl CicChallenge

CicCompare: // 03:16
	lax CIC_WRITE_0
	trs CicWriteBit
	lax CIC_WRITE_0
	trs CicWriteBit

	lbmx {high(cicCompareLo)}
	trs TRS_CicCompareRound
	trs TRS_CicCompareRound
	trs TRS_CicCompareRound

	lbmx {high(cicCompareHi)}
	trs TRS_CicCompareRound
	trs TRS_CicCompareRound
	trs TRS_CicCompareRound

// use cicCompareHi[5] to select which nibble to send
// if value is 0, add 1 (compare array spans 1-15)
	lbx cicCompareHi + 5
	lda 0
	adx 15
	lax 0
	adx 1
	exbl

CicCompareLoop: // 03:29
// write bit 0 of cicCompareLo[Bl] to CIC
// TODO CIC would probably halt if it receives an incorrect bit
	lbmx {high(cicCompareLo)}
	lax CIC_WRITE_1
	tm 0
	lax CIC_WRITE_0
	trs CicWriteBit

// read bit from CIC, then compare with bit 0 of cicCompareHi[Bl]
// if bits don't match, freeze the system
	lbmx {high(cicCompareHi)}
	trs CicReadBit
	tc
	tr CicCheck0

// CIC bit = 1, check if bit = 1
	tm 0
	tr SignalError

CicCompareNext: // 03:34
if regionNTSC {
	incb
} else if regionPAL {
	decb
	lax 0
	tabl
}
	tr CicCompareLoop
	tr MainLoop

CicCheck0: // 03:37
// CIC bit = 0, check if bit = 0
	tm 0
	tr CicCompareNext

// infinite loop, strobe reset port (NMI and pre-NMI IRQ)
SignalError: // 03:39
	id
	lblx resetPort
-;	out // P8 <- A
	coma
	tr -

////////////////////////////////////////
// page 4 (PAT data)
origin $100

// PAT table data, used to initialize CIC compare data
if regionNTSC {
	db $19, $4a, $f1, $88, $b5, $5a, $71, $c3, $de, $61, $10, $ed, $9e, $8c
} else if regionPAL {
	db $14, $2f, $35, $f1, $82, $21, $77, $11, $99, $88, $15, $17, $55, $ca
}

// swap internal and external memory
// osInfo, cicSeed: [$1b..$1f] <-> [$cb..$cf]
// ipl2Checksum: [$34..$3f] <-> [$e4..$ef]
SwapMem: // 04:0E
	lbx $cb
	call SwapMemLoop
	lbx $e4

SwapMemLoop: // 04:14
	exc 0
	exbm
	adx 5
	nop
	exbm

	exc 0
	exbm
	adx 11
	exbm
	exc 0

	incb
	tr SwapMemLoop

	lbx pifCommandHi
	rtn

// error occured when communicating with the joybus device.
// set either bit 7 or 6 of the RX count byte in PIF-RAM
JoybusError: // 04:23
	lblx 2
	lax 0
	out // P2 <- 0
	lax 1
	out // P2 <- 1

// read current channel's joybusDataPtr into SB
	lblx joyChannelPort
	in // A <- PA (get current channel)
	exbl
	lbmx {high(joybusDataPtrHi)}
	call ReadSplitByte

// if error port bit is set, no device was connected
// if error port bit is clear, transaction timed out
	lblx joyErrorPort
	tpb joyErrorPort.type // test P4.3
	tr +
	lax JOYBUS_ERR_NO_DEVICE
	tr JoybusWriteError
+;	lax JOYBUS_ERR_TIMEOUT

JoybusWriteError: // 04:34
	ex
	incb
	call IncrementPtr // go to RX byte
	add
	exc 0 // write error bit
	ex

//acknowledge error
	lax 0
	out // P4 <- 0
	tl JoybusNextChannel

////////////////////////////////////////
// page 5
origin $140

// Boot the system.
// Lock the PIF-ROM, receive checksum from VR4300 (calculated in IPL2),
// and compare it with checksum received from CIC.
SystemBoot: // 05:00
	id
	trs TRS_SwapMem // return with B = $fe
	trs ClearMemPage // clear PIF command byte
	lblx {low(pifCommandHi)}
	ie

// wait for rom lockout bit of PIF command byte to become set
-;	tm pifCommandHi.lockRom
	tr -

// lock the PIF-ROM
	id
	lax {bit(romPort.lock)}
	lblx romPort
	out // P6 <- 1

// reset joybus
	lblx 2
	lax 1
	out // P2 <- 1
	trs ResetJoybusTransactions

// wait for VR4300's signal to read checksum from PIF-RAM
	lbx pifCommandHi
	ie
-;	tm pifCommandHi.readChecksum
	tr -

// copy IPL2 checksum from PIF-RAM to internal RAM
	id
	trs TRS_SwapMem
	sm pifCommandHi.readAck // acknowledge reading checksum
	ie

// wait for VR4300's signal to compare checksums
-;	tm pifCommandHi.testChecksum
	tr -

	id
	lax 0
	exc 0 // clear pifCommandHi

// if this is a cold boot, read CIC's IPL3 checksum and initialize compare data
	tc
	call CicInit

// compare IPL2's checksum with CIC's checksum (and clear IPL2 checksum)
	lbx ipl2Checksum
-;	lax 0
	exc 1 // Bm = high(cicChecksum)
	tam
	trs TRS_SignalError
	lbmx {high(ipl2Checksum)}
	incb
	tr -

// initialize boot timer
	lbx bootTimerEnd
	trs InitBootTimer
	ie

WaitTerminateBit: // 05:2D
// must be set by VR4300 within 5 seconds after booting, else system locks up
	lbx pifCommandLo
	tm pifCommandLo.terminateBoot
	tl IncrementBootTimer

// bit set, prepare for main loop
	id
	lblx {low(pifCommandHi)}
	trs ClearMemPage // clear PIF command byte

	lbx pifStatus // Bl <- intRegister
	lax {bit(intRegister.A)} | {bit(intRegister.B)}
	out // RE <- 5
	sm pifStatus.resetPending // clear pending reset flag
	tb // clear interrupt B flag
	nop
	tl MainLoopStart

////////////////////////////////////////
// page 6
origin $180

ResetSystem: // 06:00
// send reset command to CIC
	lax CIC_WRITE_1
	trs CicWriteBit
	lax CIC_WRITE_1
	trs CicWriteBit

	lblx cicPort
	lax CIC_WRITE_1
	out // P5 <- 3

// clear reset timer
	lbx resetTimer - 3
	trs ClearMemPage // clear [$0c..$0f]

ResetWaitCic: // 06:0A
	lbx pifCommandHi
	sm pifCommandHi.readAck

// has CIC acknowledged reset?
	lbx $05
	tpb cicPort.response // test P5.3
	tr BeginReset

// increment timer
// if CIC does not reply to reset, freeze the system
	lblx {low(resetTimer)}
	trs TRS_IncrementByte
	tr ResetWaitCic
	trs TRS_IncrementByte
	tr ResetWaitCic
	trs TRS_SignalError

	lblx 5 // unused instruction

BeginReset: // 06:18
	lax CIC_WRITE_OFF
	out // P5 <- 1

// wait for user to release reset button
	lblx resetPort
	lax 0
-;	tpb resetPort.pressed // test P8.3
	tr -

	id

// unlock PIF-ROM
	lblx romPort
	out // P6 <- 0

// pulse VR4300 NMI pin
	lax {bit(resetPort.pressed)} | {bit(resetPort.nmi)}
	lblx resetPort
	out // P8 <- 9
	lax {bit(resetPort.pressed)}
	out // P8 <- 8

	sc // set carry, this is a reset
	tl Reboot


// increment byte at [B..B-1]
// on overflow, decrement Bl to next byte and return skip
IncrementByte: // 06:29
	exc 0
	adx 1
	tr IncNoOverflow

	excd 0
	exc 0
	adx 1
	tr IncNoOverflowHi

	excd 0
	rtns // overflow, return and skip

IncNoOverflow: // 06:32
	exc 0
	rtn

IncNoOverflowHi: // 06:34
	exci 0
	rtn

////////////////////////////////////////
// page 7
origin $1C0

// increment 6-nibble boot timer at [$4f..$4a]
// if all 6 nibbles overflow, freeze system
IncrementBootTimer: // 07:00
	lbmx {high(bootTimer)} // B = $4f
	trs TRS_IncrementByte
	tr +
	trs TRS_IncrementByte
	tr +
	trs TRS_IncrementByte
+;	tl WaitTerminateBit
	trs TRS_SignalError

// tell the main loop to run challenge protocol
// once finished, it will halt so RCP can read the result
SetChallengeBit: // 07:09
	lbx pifCommandLo
	rm pifCommandLo.challenge
	lbx pifStatus
	sm pifStatus.doChallenge

// restore A, exit while leaving interrupts disabled
	lblx {low(saveA)}
	exc 0
	ex
	rtn

// Use data written by PrepareJoybusTransactions to perform Joybus transactions.
JoybusDoTransactions: // 07:13
	call SaveRegs
	lblx joyChannelPort
	lax 4 // number of channels - 1
	tr JoybusCheckChannel

JoybusEndChannel: // 07:18
	lblx 2
	lax 1
	out // P2 <- 1

JoybusNextChannel: // 07:1B
	lblx joyChannelPort
	in // A <- PA (read back selected channel)
	adx 15 // decrement, go to next channel
	tr JoybusEndTransactions // on underflow, end protocol

JoybusCheckChannel: // 07:1F
	out // PA <- A (select channel)
	exbl
	lbmx {high(joybusChStatus)} // B = joybusChStatus[Bl]

// if reset bit is set, reset this channel
	tm joybusChStatus.reset
	tr +

	call JoybusResetChannel
	tr JoybusNextChannel

// if transaction bit is clear, do transaction
+;	tm joybusChStatus.doTransaction
	tr JoybusChannelTransaction

	tr JoybusNextChannel

JoybusChannelTransaction: // 07:2A
	lbmx {high(joybusDataPtrHi)}
	call ReadSplitByte // copy channel's PIF-RAM pointer to SB

// copy TX and RX counts to temp vars
	lbx txCountHi
	call JoybusCopyTxCount
	tr +
	tr JoybusNextChannel // skip channel if any of TX bits 7-6 were set

+;	call JoybusCopyRxCount

// start the joybus transaction
	lblx {low(txCountLo)} // Bl <- joyStatusPort
	tl JoybusDecTxCount

// halt so RCP can read transaction results from PIF-RAM
JoybusEndTransactions: // 07:38
	lbmx {high(pifStatus)}
	call HaltCpu
	tl JoybusExit

////////////////////////////////////////
// page 8
origin $200

JoybusDecTxCountHi: // 08:00
	excd 0
	exc 0
	adx 15
	tr JoybusEndTx // if high nibble underflows (txCount was 0), end TX
	exci 0

JoybusTxWait: // 08:05
	tpb joyStatusPort.error // test P3.2
	tl JoybusError
	tpb joyStatusPort.busy // test P3.3
	tr JoybusTxWait

// transmit high nibble of PIF-RAM [SB]
	ex
	lda 0
	incb
	outl // P0 <- A

// transmit low nibble
	lda 0
	outl // P0 <- A
	incb

// if low nibble of RAM ptr overflows, increment high nibble
	tr TxNoOver
	exbm
	adx 1 // increment hi
	tr +
	lax 8 // wrap from $FF to $80
+;	exbm
TxNoOver: // 08:17
	ex

// decrement txCount, are there more bytes left to send?
JoybusDecTxCount: // 08:18
	exc 0
	adx 15
	tr JoybusDecTxCountHi
	exc 0
	tr JoybusTxWait

// prepare for RX
JoybusEndTx: // 08:1D
	call JoybusConsoleStop
	lbx rxCountLo
	tr JoybusDecRxCount


JoybusDecRxCountHi: // 08:22
	excd 0
	exc 0
	adx 15
	tl JoybusEndChannel // if high nibble underflows (rxCount was 0), end transaction
	exci 0

JoybusRxWait: // 08:28
	tpb joyStatusPort.error // test P3.2
	tl JoybusError
	tpb joyStatusPort.busy // test P3.3
	tr JoybusRxWait

// receive high nibble to PIF-RAM [SB]
	inl // A <- P1
	ex
	exci 0

// receive low nibble
	inl // A <- P1
	exci 0
	tr RxNoOver

// low nibble of RAM ptr overflowed, increment high nibble and wrap from $F to $8
	exbm
	lbmx 8
	adx 1 // if Bm overflows, wrap to $8, else increment Bm
	exbm
RxNoOver: // 08:37
	ex

// decrement rxCount, more bytes left to receive?
JoybusDecRxCount: // 08:38
	exc 0
	adx 15
	tr JoybusDecRxCountHi
	exc 0
	tr JoybusRxWait

// unused infinite loop???
-;	nop
	nop
	tr -

////////////////////////////////////////
// page 9
origin $240

// send console stop bit to joybus device
JoybusConsoleStop: // 09:00
	lbx pifCommandLo
	tm pifCommandLo.unknown
	tr SendConsoleStop

// if unknown bit is set, send console stop only if terminateBoot is clear
	tm pifCommandLo.terminateBoot
	call SendConsoleStop
	trs TRS_LongDelay // and add a long delay
	rtn

SendConsoleStop: // 09:09
	lblx 2
	lax 2
	out // P2 <- 2
	rtn

// loop until XA overflows
LongDelay: // 09:0D
	lax 0
	atx
LongDelayLoop: // 09:0F
	exax
-;	adx 1
	tr -
	exax
	adx 1
	tr LongDelayLoop
	rtn

// copy TX count [SB..SB+1] to TX temp [$22..$23]
JoybusCopyTxCount: // 09:16
	ex
	tm 3 // skip channel if bit 7 of TX is set
	tr +
	rtns

+;	tm 2 // reset channel if bit 6 of TX is set
	tr JoybusCopyByte
	call JoybusResetChannel
	rtns

// copy RX count [SB..SB+1] to RX temp [$32..$33]
JoybusCopyRxCount: // 09:1F
	ex
	rm 3 // reset error bits in RX count
	rm 2

// copy byte from PIF-RAM to temp, increment SB (PIF-RAM ptr) to next byte
JoybusCopyByte: // 09:22
	lda 0
	ex
	exci 0 // temp hi <- PIF-RAM hi
	ex
	incb

	lda 0 // get lo nibble
	call IncrementPtr // increment PIF-RAM pointer and handle overflow
	ex
	excd 1 // temp lo <- PIF-RAM lo, flip between tx/rxTemp
	rtn

// This routine prepares all six channels for a Joybus transaction.
// joybusDataPtr[0..5]  = pointer to joybus transaction data in PIF-RAM
// joybusChStatus[0..5] = mark channel as ready for transfer, or reset it
PrepareJoybusTransactions: // 09:2D
	lbx $80
PrepJoyLoop: // 09:2F
	lax 15
	tam // are bits 7-4 of TX count = $f?
	tl PrepJoyCommand // if not, run normal joybus command

	incb
	lda 0
	adx 1 // jump if bits 3-0 != $f
	tl PrepJoySpecialCommand

// TX = $ff, command is a NOP. go to next byte
	incb
	tr PrepJoyLoop
	exbm
	adx 1
	tl + // fall through to next page
	rtn // end protocol if B overflows (end of PIF-RAM)

////////////////////////////////////////
// page A
origin $280

+;	exbm
	tl PrepJoyLoop

PrepJoySpecialCommand: // 0A:03
	adx 1
	tr +
	rtn // end protocol if TX = $fe

+;	adx 1
	tr PrepJoySetDataPtrDecb // if not $fd, handle as normal command

// TX = $fd, mark joybus channel for reset and skip this channel
	ex
	lbmx {high(joybusChStatus)}
	sm joybusChStatus.reset
	lbmx {high(joybusDataPtrHi)}
	ex
	tr PrepJoySkip

PrepJoyCommand: // 0A:0E
// check if TX = $00
	lax 0
	tam
	tr PrepJoySetDataPtr
	incb
	tam
	tr PrepJoySetDataPtrDecb

// TX = $00, skip this channel

PrepJoySkip: // 0A:14
// increment PIF-RAM pointer
	incb
	tr SkipNoOver
	exbm
	adx 1 // increment hi
	tr +
	rtn // end protocol if B overflows (end of PIF-RAM)
+;	exbm
SkipNoOver: // 0A:1B
	ex
	incb // skip channel
	tl PrepJoyNextCmd

// B = PIF-RAM pointer
// SB = joybusDataHi
PrepJoySetDataPtrDecb: // 0A:1F
	decb
PrepJoySetDataPtr: // 0A:20
// write high nibble of transaction data addr to joybusDataPtrHi
	exbm
	atx // X <- Bm
	exbm
	exax
	ex
	exc 1 // joybusDataPtrHi[SBl] <- X (Bm)
	ex

// SB = joybusDataLo
// write low nibble of transaction data addr to joybusDataPtrHi
	exbl
	atx // X <- Bl
	exbl
	exax
	ex
	exci 1 // joybusDataPtrLo[SBl] <- X (Bl)
	ex

// joybusDataLo/Hi now contains pointer to data for this channel's transaction

// copy TX count bits 7-4 to X, mask upper 2 bits of X
PrepJoyHandleTx:
	lda 0
	rm 2
	rm 3
	exci 0
	exax
// copy TX count bits 0-3 to A
	lda 0

// mark channel as ready for transaction
	ex
	lbmx {high(joybusChStatus)}
	decb
	rm joybusChStatus.doTransaction
	incb
	lbmx {high(joybusDataPtrHi)}
	exax
	tl + // fall through to next page

////////////////////////////////////////
// page B
origin $2C0

// now comes the lengthy process of jumping to the next command in PIF-RAM
// temporarily write no of bytes in this transaction to next channel's data ptr

// initialize with TX byte count...
+;	exc 1
	exax
	exc 1
// next, add number of RX bytes

// go to next PIF-RAM byte (RX count)
	ex
	incb
	tr PrepJoyHandleRx
	exbm
	adx 1
	tr PrepJoyHandleRxExbm
	tr PrepJoyCancelEx // end protocol if B overflows (end of PIF-RAM)

PrepJoyHandleRxExbm: // 0B:0A
	exbm
PrepJoyHandleRx: // 0B:0B
// copy RX count bits 7-4 to X, mask upper 2 bits of X
	lda 0
	rm 3
	rm 2
	exci 0
	exax
// copy RX count bits 0-3 to A
	lda 0

// add RX count lo to transaction byte count lo
	ex
	rc
	lbmx {low(joybusDataPtrLo)}
	adc
	nop
	exc 1 // write transaction byte count lo

// B = joybusDataPtrHi
// add RX count hi to transaction byte count hi
	exax
	adc
	exc 1 // write transaction byte count hi

// B = joybusDataPtrLo
// convert transaction byte count to nibble count
// by adding to self (i.e. multiply by 2)
	rc
	lda 0
	adc
	nop
	exc 1 // B = joybusDataPtrHi
	lda 0
	adc
	exc 1 // B = joybusDataPtrLo

// next channel's data pointer now temporarily holds the length of this
// channel's transaction in nibbles (minus 1)
// now add it to B to go to next command in PIF-RAM
	ex
	exbl // A <- low nibble of curr PIF-RAM address
	ex

	sc // add 1
	adc // add transaction nibble count lo
	nop
	lbmx {high(joybusDataPtrHi)}

	ex
	exbl // Bl <- next joybus command lo
	exbm // A <- high nibble of curr PIF-RAM address
	ex

	adc // add transaction nibble count hi + carry
	tr +
	tr PrepJoyCancel // if address overflows, cancel this channel's transaction and end protocol
+;	ex
	exbm // Bm <- next joybus command hi
	ex

// B now contains address of next command in PIF-RAM
// finally, go to next channel

PrepJoyNextCmd: // 0B:33
	lax 5
	tabl
	tr +
	rtn // if Bl = 5, last channel was just processed. end protocol
+;	ex
	tl PrepJoyLoop // else, process next channel

PrepJoyCancelEx: // 0B:3A
	ex
PrepJoyCancel: // 0B:3B
	decb
	lbmx {high(joybusChStatus)}
	rm joybusChStatus.reset // do not reset this channel
	sm joybusChStatus.doTransaction // cancel transaction
	rtn

////////////////////////////////////////
// page C
origin $300

// read nibble from CIC into [B]
// increments Bl, returns with skip on overflow
CicReadNibble: // 0C:00
	lax 15
	exc 0

	trs CicReadBit
	tc
	rm 3

	trs CicReadBit
	tc
	rm 2

	trs CicReadBit
	tc
	rm 1

	trs CicReadBit
	tc
	rm 0

	rc
	tr CicNibbleEnd

// write nibble at [B] to CIC
// increments Bl, returns with skip on overflow
CicWriteNibble: // 0C:10
	lax CIC_WRITE_1
	tm 3
	lax CIC_WRITE_0
	trs CicWriteBit

	lax CIC_WRITE_1
	tm 2
	lax CIC_WRITE_0
	trs CicWriteBit

	lax CIC_WRITE_1
	tm 1
	lax CIC_WRITE_0
	trs CicWriteBit

	lax CIC_WRITE_1
	tm 0
	lax CIC_WRITE_0
	trs CicWriteBit

CicNibbleEnd: // 0C:20
	incb
	rtn
	rtns


JoybusResetLoop: // 0C:23
// acknowledge error
	lblx joyErrorPort
	lax 0
	out // P4 <- 0

// reset joybus channel selected by PA
JoybusResetChannel: // 0C:26
	lblx joyStatusPort
	tpb joyStatusPort.busy // test P3.3
	tr JoybusResetLoop

	lblx 2
	lax 3
	out // P2 <- 3
	lax 1
	out // P2 <- 1

	lblx joyStatusPort
-;	tpb joyStatusPort.busy // test P3.3
	tr -
	rtn


// copy split byte at [B..B^16] to SB
ReadSplitByte: // 0C:32
	lda 1
	ex
	exbm // SBm <- [B]
	ex

	lda 1
	ex
	exbl // SBl <- [B^16]
	ex
	rtn

////////////////////////////////////////
// page D
origin $340

// This routine runs the challenge/response protocol used by CIC-6105.
// It sends the challenge data starting at PIF-RAM $30 to the CIC,
// and writes the response back to PIF-RAM $30.
// No. of bytes to send to CIC is specified at PIF-RAM $2E,
// bytes to receive specified at $2F.
CicChallenge: // 0D:00
// send challenge command to CIC
	lax CIC_WRITE_1
	trs CicWriteBit
	lax CIC_WRITE_0
	trs CicWriteBit

// read response timer value from CIC (how long to wait before reading response)
	lbx cicResponseTimerHi
	trs TRS_CicReadNibble
	trs TRS_CicReadNibble

// send challenge data to CIC
	lbx cicChallengeCount
	call CicChallengeTransfer

// decrement the response timer to zero
	lbx cicResponseTimerLo
-;	trs TRS_IncrementByte
	tr -
	trs CicReadBit // acknowledge timer reaching zero

// receive response from CIC
	lbx cicResponseCount
	call CicChallengeTransfer

// halt so RCP can read challenge response
// (we're still handling an interrupt!)
	lbmx {high(pifStatus)}
	call HaltCpu
	trs TRS_SetSB // SB = saveA
	tl MainLoopStart // return to main loop and re-enable interrupts


CicChallengeTransfer: // 0D:1B
	ex
	lbx cicChallengeData // $30 in PIF-RAM
CicChallengeLoop: // 0D:1E
	ex

// decrement count, return on underflow
	lda 0
	adx 15
	rtn
	exc 0

// check which counter Bl points to
	lax {low(cicChallengeCount)}
	tabl
	tr CicResponse

// Bl points to cicChallengeCount, send data to CIC
	ex
	trs TRS_CicWriteNibble
	trs TRS_CicWriteNibble
	tr CicChallengeLoop
	tr CicChallengeIncBm // increment Bm if Bl overflows

// Bl points to cicResponseCount, receive data from CIC
CicResponse: // 0D:2B
	ex
	trs TRS_CicReadNibble
	trs TRS_CicReadNibble
	tr CicChallengeLoop

// increment Bm from 14 to 15
// assumes Bm is 14 (no problem, challenge data can't exceed 16 bytes)
CicChallengeIncBm: // 0D:2F
	lbmx 15
	tr CicChallengeLoop


// set SB = $56, for handling interrupts
SetSB: // 0D:31
	lbx saveA
	ex
	rtn

// increment B and wrap to $80 on overflow
IncrementPtr: // 0D:35
	incb
	rtn

	exbm
	adx 1
	tr +
	lax 8
+;	exbm
	rtn

////////////////////////////////////////
// page E
origin $380

CicInit: // 0E:00
// set reset bit, copied to VR4300 on next reset
	lbx osInfo
	sm osInfo.reset

// generate random seed for compare protocol
	call CicGenerateRandomSeed

// signal CIC to send its IPL3 checksum
	lblx cicPort
	lax CIC_WRITE_1
	out // P5 <- 3
	trs TRS_LongDelay
	lax CIC_WRITE_OFF
	out // P5 <- 1

// read 8-byte checksum from CIC
	lbx cicChecksum
-;	trs TRS_CicReadNibble
	tr -

// do 4 rounds of descrambling on the CIC checksum
	trs TRS_CicDescramble
	trs TRS_CicDescramble
	trs TRS_CicDescramble
	trs TRS_CicDescramble

// initialize CIC compare data with contents of PAT table
	tl CicInitCompare

// descramble data received from CIC
// loop until end of memory page
CicDescramble: // 0E:15
	lax 15
-;	coma
	add
	exci 0
	tr -
	rtn

// Bm = high(cicCompareLo) or high(cicCompareHi)
CicCompareRound: // 0E:1B
// read last nibble from compare array, determines how many rounds to run
	lblx {low(cicCompareLoEnd)}
	lda 0

CicCompareRoundLoop: // 0E:1D
	atx

// A += random + 1
	sc
	lblx {low(cicRandomLo)}
	adc
	sc

// random <- A
	exc 0
	lda 0

// A += compare[0] + 1; A ^= $f
	incb
	adc
	sc
	coma

// compare[0] <- A; A <- old compare[0]
	exci 0

// A += compare[1] + 1; if no overflow, compare[1] <- A
	adc
	exci 0

// compare[1/2] <- A += compare[1/2]; A <- compare[1/2]
	add
	exc 0
	lda 0

// compare[2/3] <- A += compare[2/3]; A <- old compare[2/3]
	incb
	add
	exci 0

// A += 8; if no overflow, A += compare[3/4]
	adx 8
	add

// compare[3/4] <- A
	exci 0

// Bl is either 4 or 5
// A is old value of compare[3/4]
CicCompareAdd:
// A += 1 + compare[Bl]
	adx 1
	nop
	add

// compare[Bl] <- A
	exc 0
	lda 0

	incb
	tr CicCompareAdd

// decrement X, do another round if not zero
	exax
	adx 15
	rtn
	tr CicCompareRoundLoop

////////////////////////////////////////
// page F
origin $3C0

CicInitCompare: // 0F:00
// reset PAT pointer to zero
	lbx patPointer
	lax 0
	exc 0

	ex
	lbx cicCompareLo
CicInitCompareLoop: // 0F:07
	ex
	lax 4
	atx // X <- 4

// load then increment patPointer
	lda 0
	adx 1
	exc 0
	ex

// load byte from PAT and write it to cicCompare
	pat // XA <- PAT[XA]
	nop
	exc 1 // cicCompareLo[Bl] <- low(PAT[XA])
	exax
	exci 1 // cicCompareHi[Bl] <- high(PAT[XA])
	tr CicInitCompareLoop

// write random seed generated earlier to CIC
	lblx {low(cicRandomLo)}
	trs TRS_CicWriteNibble
	lbx cicRandomHi
	trs TRS_CicWriteNibble
	tl SetSB // SB = saveA
	// rtn (tail call)

CicGenerateRandomSeed: // 0F:1B
	lbx cicRngTimerLo // Bl <- rngPort
	lax 1
	out // P9 <- 1

// increment random seed byte until RNG bit goes high
-;	call IncrementByte
	nop
	lblx {low(cicRngTimerLo)} // prevent overflowing to next byte
	tpb rngPort.output // test P9.3
	tr -

// halt RNG
	lax 0
	out // P9 <- 0

// write each nibble of the random byte to the first nibble of CIC compare arrays
	excd 0 // read cicRngTimerLo (and set it to 0)
	exax
	exc 0 // read cicRngTimerHi (and set it to X)
	lblx {low(cicRandomLo)}
	exc 1 // write cicRngTimerHi to cicRandomLo
	exax
	exc 1 // write cicRngTimerLo to cicRandomHi
	rtn


// save SB, X and C
SaveRegs: // 0F:2F
	lbx saveBm

// [saveBm] <- Bm
	ex
	exbm
	ex
	exc 1

// [saveBl] <- Bl
	ex
	exbl
	ex
	exci 1

// [saveX] <- X
	exax
	exci 0

// [saveC] <- C
	sm 0
	tc
	rm 0

	rc
	rtn

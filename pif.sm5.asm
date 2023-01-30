architecture n64.pif

define low(value) = {value} & $0f
define high(value) = ({value} & $f0) >> 4

// RAM variables

// low 4 bits of PIF status byte (from VR4300 perspective)
constant pifStatusLo = $ff
constant pifStatusLo.joybus = 0    // run joybus protocol
constant pifStatusLo.challenge = 1 // send challenge to CIC
constant pifStatusLo.unknown = 2   // unknown
constant pifStatusLo.terminateBoot = 3 // terminate boot process
// high 4 bits of PIF status byte
constant pifStatusHi = $fe
constant pifStatusHi.lockRom = 0   // lock out PIF-ROM
constant pifStatusHi.verify = 1    // verify CIC checksum
constant pifStatusHi.clearRam = 2  // clear all of PIF-RAM
constant pifStatusHi.response = 3  // CIC checksum verification is complete

// status nibble
constant pifState = $5e
constant pifState.challenge = 1
constant pifState.terminateRecv = 3

// 6-nibble boot timer
constant bootTimer = $4f // $4e, $4d, $4c, $4b, $4a
// 6-nibble CIC seed
constant cicSeed = $1a // $1b, $1c, $1d, $1e, $1f

// pointer to data for each channel's joybus transaction
constant joybusDataPtrLo = $00 // $01, $02, $03, $04, $05
constant joybusDataPtrHi = $10 // $11, $12, $13, $14, $15

// status for each channel
constant joybusChStatus = $40 // $41, $42, $43, $44, $45
constant joybusChStatus.doTransaction = 3 // if bit clear, do transaction
constant joybusChStatus.reset = 0         // if bit set, reset channel

// saved registers
constant saveA = $56
constant saveBm = $57
constant saveBl = $47
constant saveX = $58
constant saveC = $59

// I/O variables

// CIC data port
constant cicPort = 5
constant cicPort.data = 0     // bidirectional data pin (CIC_15 on cart)
constant cicPort.clock = 1    // data clock (CIC_14 on cart)
constant cicPort.response = 3 // response from CIC

constant CIC_WRITE_0 = 1<<cicPort.clock
constant CIC_WRITE_1 = 1<<cicPort.data | 1<<cicPort.clock
constant CIC_WRITE_OFF = 1<<cicPort.data


// page 0 (reset vector)
origin $000

	lax CIC_WRITE_OFF
	lblx cicPort
	out   // P5 <- 1
	lblx 14
	out   // RE <- 1 (enable interrupt A)
	trs TRS_SetSB // SB = $56
	lbx $34
	trs ClearMemPage // [$34..$3f] = 0

// read CIC type to [$5e]
	lbx pifState
	trs TRS_CicReadNibble
	decb

// if type == 1, cart
	lax 1
	tam
	tr + // not cart
	lax 4
	tr WriteCicType

// if type == 9, 64DD
+;	lax 9
	tam
	trs TRS_SignalError // not 64DD
	lax 12

WriteCicType:
	exc 0

// clear all of PIF-RAM
	lbx $80
	tr +
ResetClearRam:
	exbm
+;	trs ClearMemPage
	exbm
	adx 1
	tr ResetClearRam

// write CIC seed to [$1a..$1f]
	lbx cicSeed
-;	trs TRS_CicReadNibble // loop until Bl overflows
	tr -

// decode seed (2 rounds)
	lblx {low(cicSeed)}
	trs TRS_CicDecodeSeed
	lblx {low(cicSeed)}
	trs TRS_CicDecodeSeed

// copy CIC type to [$1b]
// this will eventually be written to [$cb] (PIF-RAM $24 bits 0-3)
	lbx pifState
	lda 0
	rm 1
	rm 3
	lbx $1b
	excd 0
	rc

L00_30:
	lbx pifStatusHi
	sm pifStatusHi.response
	ie
	tl L05_00

// page 1 (TRS vectors)
origin $040

TRS_IncrementByte:
	tl IncrementByte
TRS_SwapMem:
	tl SwapMem
TRS_CicWriteNibble:
	tl CicWriteNibble
TRS_LongDelay:
	tl LongDelay
TRS08:
	tl L0E_1B
TRS_CicDecodeSeed:
	tl CicDecodeSeed
TRS_SignalError:
	tl SignalError
TRS_CicReadNibble:
	tl CicReadNibble
TRS_SetSB:
	tl SetSB

// write $fb to [B] and zero rest of segment
L01_12:
	lax 15
	exci 0
	lax 11
	exci 0

// zero memory from B to end of segment
ClearMemPage:
	lax 0
	exci 0
	tr ClearMemPage
	rtn

// fill [$40..$45] with 8
ResetJoybusTransactions:
	lbx joybusChStatus + 5
-;	lax 1<<joybusChStatus.doTransaction
	excd 0
	tr -
	rtn

CicWriteBit:
	exax
	lax cicPort
	exbl
	exax  // save old Bl in X

	out   // P5 <- A

	lax 11 // short delay
-;	adx 1
	tr -

	tr CicEndIo
	nop

CicReadBit:
	lax cicPort
	exbl
	exax  // save old Bl in X

	lax CIC_WRITE_1
	out   // P5 <- 3

	lax 11 // short delay
-;	adx 1
	tr -

// set carry to P5.3 (CIC reply)
	sc
	tpb cicPort.response // test P5.3
	rc

CicEndIo:
	lax CIC_WRITE_OFF
	out   // P5 <- 1

// another delay
	lax 12
-;	adx 1
	tr -

// restore Bl from X
	exax
	exbl
	rtn

// page 2 (interrupt vectors)
origin $080

InterruptA:
	ex     // B = $56
	exci 0 // [saveA] <- A
	tr L02_0E
	nop

InterruptB:
	ex     // B = $56
	exc 0  // [saveA] <- A

	lblx {low(pifState)}
	rm pifState.terminateRecv

	lax 1
	out   // RE <- 1 (enable interrupt A)

	lblx 8
	lax 2
	out   // R8 <- 2
	tr L02_3B

L02_0E:
	tpb 3 // test P7.3
	tr L02_1D
	tpb 2 // test P7.2
	tr L02_39

	lbx pifStatusLo
	tm pifStatusLo.challenge
	tl JoybusDoTransactions

	lbx pifState
	tm pifState.terminateRecv
	tr L02_39
	tl InterruptAltExit

L02_1D:
	call HaltCpu

// if bit 0 of PIF status byte is set, run joybus protocol
	lbx pifStatusLo
	tm pifStatusLo.joybus
	tr SkipJoybus

	rm 0
	call SaveRegs
	lbx joybusDataPtrHi
	ex
	trs ResetJoybusTransactions // [$40..$45] = 8
	call PrepareJoybusTransactions

L02_2C:
	lbx $59
	sc
	tm 0
	rc
	decb
	excd 0
	exax
	call ReadSplitByte
	tr L02_3B

SkipJoybus:
	lbmx 5
	tr L02_3B
L02_39:
	call HaltCpu
L02_3B:
	lblx 6
	exc 0
	ex
	rtni

// page 3 (standby exit vector)
origin $0C0

StandbyExit:
	nop
	tm 3
	rtn
	lax 5
	out   // RE <- 5 (enable interrupt A and B)
	rtn

HaltCpu:
	lblx 14
	lax 1
	out   // RE <- 1 (enable interrupt A)
	halt
	tr StandbyExit

CicLoopStart:
	ie

CicLoop:
	lbx pifState
	tm pifState.terminateRecv
	tl CicReset
	tm pifState.challenge
	tr CicCompare
	rm pifState.challenge
	tl CicChallenge

CicCompare:
	lax CIC_WRITE_0
	trs CicWriteBit
	lax CIC_WRITE_0
	trs CicWriteBit
	lbmx 6 // B = $6e
	trs TRS08 // L0E_1B
	trs TRS08 // L0E_1B
	trs TRS08 // L0E_1B
	lbmx 7 // B = $7e
	trs TRS08 // L0E_1B
	trs TRS08 // L0E_1B
	trs TRS08 // L0E_1B

	lbx $77
	lda 0
	adx 15
	lax 0
	adx 1
	exbl
L03_29:
	lbmx 6
	lax 3
	tm 0
	lax CIC_WRITE_0
	trs CicWriteBit
	lbmx 7
	trs CicReadBit // return with B = $75
	tc
	tr L03_37
	tm 0
	tr SignalError
L03_34:
	incb
	tr L03_29
	tr CicLoop
L03_37:
	tm 0
	tr L03_34

// infinite loop, strobe R8
SignalError:
	id
	lblx 8
-;	out   // R8 <- A
	coma
	tr -

// page 4 (PAT data)
origin $100

	db $19
	db $4a
	db $f1
	db $88
	db $b5
	db $5a
	db $71
	db $c3
	db $de
	db $61
	db $10
	db $ed
	db $9e
	db $8c

// swap internal and external memory
// [$1b..$1f] <-> [$cb..$cf]
// [$34..$3f] <-> [$e4..$ef]
SwapMem:
	lbx $cb
	call L04_14
	lbx $e4
L04_14:
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
	tr L04_14
	lbx pifStatusHi
	rtn

L04_23:
	lblx 2
	lax 0
	out   // P2 <- 0
	lax 1
	out   // P2 <- 1
	lblx 10
	in    // A <- RA
	exbl
	lbmx 1
	call ReadSplitByte
	lblx 4
	tpb 3 // test P4.3
	tr L04_33
	lax 8
	tr L04_34
L04_33:
	lax 4
L04_34:
	ex
	incb
	call IncrementPtr
	add
	exc 0
	ex
	lax 0
	out   // P4 <- 0
	tl JoybusNextChannel

// page 5
origin $140

L05_00:
	id
	trs TRS_SwapMem  // return with B = $fe
	trs ClearMemPage // clear PIF status byte
	lblx 14
	ie

// wait for rom lockout bit of PIF status byte to become set
-;	tm 0
	tr -

	id
	lax 1
	lblx 6
	out   // P6 <- 1
	lblx 2
	lax 1
	out   // P2 <- 1
	trs ResetJoybusTransactions

	lbx pifStatusHi
	ie

// wait for checksum verification bit
-;	tm pifStatusHi.verify
	tr -

	id
	trs TRS_SwapMem // return with B = $fe
	sm pifStatusHi.response // set checksum response bit
	ie

// wait for clear PIF ram bit
-;	tm pifStatusHi.clearRam
	tr -

	id
	lax 0
	exc 0 // clear pifStatusHi
	tc    // TODO: where does carry flag come from?
	call L0E_00
	lbx $34

// compare checksum
L05_22:
	lax 0
	exc 1
	tam
	trs TRS_SignalError
	lbmx 3
	incb
	tr L05_22

	lbx $4a
	trs L01_12
	ie
WaitTerminateBit:
// must be set by CPU within 5 seconds after booting, else system locks up
	lbx pifStatusLo
	tm pifStatusLo.terminateBoot
	tl IncrementBootTimer

	id
	lblx {low(pifStatusHi)}
	trs ClearMemPage // clear PIF status byte
	lbx pifState // Bl <- 14
	lax 5
	out   // RE <- 5 (enable interrupt A and B)
	sm pifState.terminateRecv
	tb    // clear interrupt B flag
	nop
	tl CicLoopStart

// page 6
origin $180

CicReset:
	lax CIC_WRITE_1
	trs CicWriteBit
	lax CIC_WRITE_1
	trs CicWriteBit

	lblx cicPort
	lax CIC_WRITE_1
	out   // P5 <- 3
	lbx $0c
	trs ClearMemPage

L06_0A:
	lbx pifStatusHi
	sm pifStatusHi.response

	lbx $05
	tpb cicPort.response // test P5.3
	tr L06_18
	lblx 15
	trs TRS_IncrementByte
	tr L06_0A
	trs TRS_IncrementByte
	tr L06_0A
	trs TRS_SignalError
	lblx 5
L06_18:
	lax CIC_WRITE_OFF
	out   // P5 <- 1

	lblx 8
	lax 0
-;	tpb 3 // test R8.3
	tr -

	id
	lblx 6
	out   // P6 <- 0
	lax 9
	lblx 8
	out   // R8 <- 9
	lax 8
	out   // R8 <- 8
	sc
	tl L00_30

// increment two nibbles at [B] and [B-1]
// on overflow, decrement Bl twice and return skip
IncrementByte:
	exc 0
	adx 1
	tr L06_32

	excd 0
	exc 0
	adx 1
	tr L06_34

	excd 0
	rtns   // overflow, return and skip

L06_32:
	exc 0
	rtn

L06_34:
	exci 0
	rtn

// page 7
origin $1C0

// increment 6-nibble boot timer at [$4f..$4a]
// if all 6 nibbles overflow, lock up
IncrementBootTimer:
	lbmx {high(bootTimer)}
// B = $4f
	trs TRS_IncrementByte
	tr +
	trs TRS_IncrementByte
	tr +
	trs TRS_IncrementByte
+;	tl WaitTerminateBit
	trs TRS_SignalError

InterruptAltExit:
	lbx pifStatusLo
	rm pifStatusLo.challenge
	lbx pifState
	sm pifState.challenge

	lblx {low(saveA)}
	exc 0
	ex
	rtn

JoybusDoTransactions:
	call SaveRegs
	lblx 10
	lax 4   // number of channels
	tr JoybusCheckChannel

L07_18:
	lblx 2
	lax 1
	out   // P2 <- 1

JoybusNextChannel:
	lblx 10
	in        // A <- RA (read back selected channel)
	adx 15    // decrement, go to next channel
	tr JoybusEndTransfers // on underflow, end protocol

JoybusCheckChannel:
	out   // RA <- A (select channel)
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

JoybusChannelTransaction:
	lbmx {high(joybusDataPtrHi)}
	call ReadSplitByte // SB = joybusDataPtr[Bl]
	lbx $22
	call L09_16
	tr +
	tr JoybusNextChannel

+;	call L09_1F
	lblx 3
	tl L08_18

JoybusEndTransfers:
	lbmx 5
	call HaltCpu
	tl L02_2C

// page 8
origin $200

L08_00:
	excd 0
	exc 0
	adx 15
	tr L08_1D
	exci 0
L08_05:
	tpb 2 // test ??.2
	tl L04_23
	tpb 3 // test ??.3
	tr L08_05

	ex
	lda 0
	incb
	outl  // P0 <- A
	lda 0
	outl  // P0 <- A
	incb
	tr L08_17
	exbm
	adx 1
	tr L08_16
	lax 8
L08_16:
	exbm
L08_17:
	ex
L08_18:
	exc 0
	adx 15
	tr L08_00
	exc 0
	tr L08_05

L08_1D:
	call L09_00
	lbx $33
	tr L08_38

L08_22:
	excd 0
	exc 0
	adx 15
	tl L07_18
	exci 0
L08_28:
	tpb 2 // test ??.2
	tl L04_23
	tpb 3 // test ??.3
	tr L08_28

	inl   // A <- P3
	ex
	exci 0
	inl   // A <- P3
	exci 0
	tr L08_37
	exbm
	lbmx 8
	adx 1
	exbm
L08_37:
	ex
L08_38:
	exc 0
	adx 15
	tr L08_22
	exc 0
	tr L08_28

L08_3D:
	nop
	nop
	tr L08_3D

// page 9
origin $240

L09_00:
	lbx pifStatusLo
	tm pifStatusLo.unknown
	tr L09_09
	tm pifStatusLo.terminateBoot
	call L09_09
	trs TRS_LongDelay
	rtn

L09_09:
	lblx 2
	lax 2
	out   // P2 <- 2
	rtn

// loop until XA overflows
LongDelay:
	lax 0
	atx
LongDelayLoop:
	exax
-;	adx 1
	tr -
	exax
	adx 1
	tr LongDelayLoop
	rtn

L09_16:
	ex   // B = joybusDataPtr[SB], SB = $22
	tm 3 // skip if pointer is inside PIF-RAM
	tr +
	rtns

+;	tm 2 // reset channel if bit 2 is set, TODO why?
	tr L09_22
	call JoybusResetChannel
	rtns

L09_1F:
	ex
	rm 3
	rm 2
L09_22:
// copy [B] to [SB], increment B and SB
	lda 0
	ex
	exci 0
	ex
	incb

	lda 0
	call IncrementPtr
	ex
	excd 1
	rtn

// This routine prepares all six channels for a Joybus transaction.
// joybusDataPtr[0..5]  = pointer to joybus transaction data in PIF-RAM
// joybusChStatus[0..5] = mark channel as ready for transfer, or reset it
PrepareJoybusTransactions:
	lbx $80
JoybusLoop:
	lax 15
	tam   // are bits 7-4 of TX count = $f?
	tl JoybusCommand // if not, run normal joybus command

	incb
	lda 0
	adx 1 // jump if bits 3-0 != $f
	tl JoybusSpecialCommand

// TX = $ff, command is a NOP. go to next byte
	incb
	tr JoybusLoop
	exbm
	adx 1
	tl L0A_00
	rtn   // end protocol if B overflows (end of PIF-RAM)

// page A
origin $280

L0A_00:
	exbm
	tl JoybusLoop

JoybusSpecialCommand:
	adx 1
	tr +
	rtn   // end protocol if TX = $fe

+;	adx 1
	tr JoybusHandleTxDecb // if not $fd, handle as normal command

// TX = $fd, mark joybus channel for reset
	ex
	lbmx {high(joybusChStatus)}
	sm joybusChStatus.reset
	lbmx {high(joybusDataPtrHi)}
	ex
	tr L0A_14

JoybusCommand:
// check if TX = $00
	lax 0
	tam
	tr JoybusHandleTx
	incb
	tam
	tr JoybusHandleTxDecb

// TX = $00, skip this channel
L0A_14:
// increment PIF-RAM pointer
	incb
	tr L0A_1B
	exbm
	adx 1
	tr +
	rtn   // end protocol if B overflows (end of PIF-RAM)
+;	exbm
L0A_1B:
	ex
	incb  // skip channel
	tl JoybusNextCmd

JoybusHandleTxDecb:
	decb
JoybusHandleTx:

// B = PIF-RAM pointer
// SB = joybusDataHi
// write high nibble of transaction data addr to joybusDataPtrHi
	exbm
	atx    // X <- Bm
	exbm
	exax
	ex
	exc 1  // joybusDataPtrHi[SBl] <- X (Bm)
	ex

// SB = joybusDataLo
// write low nibble of transaction data addr to joybusDataPtrHi
	exbl
	atx    // X <- Bl
	exbl
	exax
	ex
	exci 1 // joybusDataPtrLo[SBl] <- X (Bl)
	ex

// joybusDataLo/Hi now contains pointer to data for this channel's transaction

// copy TX count bits 7-4 to X, mask upper 2 bits of X
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
	tl L0B_00

// page B
origin $2C0

L0B_00:
// now comes the lengthy process of jumping to the next command in PIF-RAM
// temporarily write no of bytes in this transaction to next channel's data ptr

// initialize with TX byte count...
	exc 1
	exax
	exc 1
// next, add number of RX bytes

// go to next PIF-RAM byte (RX count)
	ex
	incb
	tr JoybusHandleRx
	exbm
	adx 1
	tr JoybusHandleRxExbm
	tr CancelJoybusCmdEx // end protocol if B overflows (end of PIF-RAM)

JoybusHandleRxExbm:
	exbm
JoybusHandleRx:
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
	exc 1  // write transaction byte count lo

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

	sc   // add 1
	adc  // add transaction nibble count lo
	nop
	lbmx {high(joybusDataPtrHi)}

	ex
	exbl // Bl <- next joybus command lo
	exbm // A <- high nibble of curr PIF-RAM address
	ex

	adc  // add transaction nibble count hi + carry
	tr +
	tr CancelJoybusCmd // if address overflows, cancel this channel's transaction and end protocol
+;	ex
	exbm // Bm <- next joybus command hi
	ex

// B now contains address of next command in PIF-RAM
// finally, go to next channel

JoybusNextCmd:
	lax 5
	tabl
	tr +
	rtn   // if Bl = 5, last channel was just processed. end protocol
+;	ex
	tl JoybusLoop // else, process next channel

CancelJoybusCmdEx:
	ex
CancelJoybusCmd:
	decb
	lbmx {high(joybusChStatus)}
	rm joybusChStatus.reset         // do not reset this channel
	sm joybusChStatus.doTransaction // cancel transaction
	rtn

// page C
origin $300

// read nibble from CIC into [B]
// increments Bl, returns with skip on carry
CicReadNibble:
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

CicWriteNibble:
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

CicNibbleEnd:
	incb
	rtn
	rtns

JoybusResetLoop:
	lblx 4
	lax 0
	out   // P4 <- 0

// reset joybus channel selected by RA
JoybusResetChannel:
	lblx 3
	tpb 3 // test P3.3
	tr JoybusResetLoop

	lblx 2
	lax 3
	out   // P2 <- 3
	lax 1
	out   // P2 <- 1

	lblx 3
-;	tpb 3 // test P3.3
	tr -
	rtn

// copy split byte at [B..B^16] to SB
ReadSplitByte:
	lda 1
	ex
	exbm // SBm <- [B]
	ex

	lda 1
	ex
	exbl // SBl <- [B^16]
	ex
	rtn

// page D
origin $340

CicChallenge:
	lax CIC_WRITE_1
	trs CicWriteBit
	lax CIC_WRITE_0
	trs CicWriteBit
	lbx $0a
	trs TRS_CicReadNibble
	trs TRS_CicReadNibble
	lbx $dd
	call L0D_1B
	lbx $0b

-;	trs TRS_IncrementByte
	tr -

	trs CicReadBit
	lbx $df
	call L0D_1B
	lbmx 5
	call HaltCpu
	trs TRS_SetSB // SB = $56
	tl CicLoopStart

L0D_1B:
	ex
	lbx $e0
L0D_1E:
	ex
	lda 0
	adx 15
	rtn

L0D_22:
	exc 0
	lax 13
	tabl
	tr L0D_2B
	ex
	trs TRS_CicWriteNibble
	trs TRS_CicWriteNibble
	tr L0D_1E
	tr L0D_2F

L0D_2B:
	ex
	trs TRS_CicReadNibble
	trs TRS_CicReadNibble
	tr L0D_1E
L0D_2F:
	lbmx 15
	tr L0D_1E

// set SB = $56
SetSB:
	lbx $56
	ex
	rtn

// increment B and wrap to $80 on overflow
IncrementPtr:
	incb
	rtn

	exbm
	adx 1
	tr +
	lax 8
+;	exbm
	rtn

// page E
origin $380

L0E_00:
	lbx $1b
	sm 1
	call L0F_1B
	lblx 5
	lax 3
	out   // R3 <- 3 (set P3 pins 0 and 1 to analog)
	trs TRS_LongDelay
	lax 1
	out   // R3 <- 1 (set P3 pin 0 to analog)
	lbx $20
L0E_0D:
	trs TRS_CicReadNibble
	tr L0E_0D

// 4 rounds of decoding
	trs TRS_CicDecodeSeed
	trs TRS_CicDecodeSeed
	trs TRS_CicDecodeSeed
	trs TRS_CicDecodeSeed
	tl L0F_00

// decode CIC seed or checksum (one round)
CicDecodeSeed:
	lax 15
-;	coma
	add
	exci 0
	tr -
	rtn

L0E_1B:
	lblx 15
	lda 0
L0E_1D:
	atx
	sc
	lblx 1
	adc
	sc
	exc 0
	lda 0
	incb
	adc
	sc
	coma
	exci 0
	adc
	exci 0
	add
	exc 0
	lda 0
	incb
	add
	exci 0
	adx 8
	add
	exci 0

L0E_34:
	adx 1
	nop
	add
	exc 0
	lda 0
	incb
	tr L0E_34

	exax
	adx 15
	rtn
	tr L0E_1D

// page F
origin $3C0

L0F_00:
	lbx $60
	lax 0
	exc 0
	ex
	lbx $62
L0F_07:
	ex
	lax 4
	atx
	lda 0
	adx 1
	exc 0
	ex
	pat
	nop
	exc 1
	exax
	exci 1
	tr L0F_07
	lblx 1
	trs TRS_CicWriteNibble
	lbx $71
	trs TRS_CicWriteNibble
	tl SetSB // SB = $56

L0F_1B:
	lbx $69
	lax 1
	out   // R9 <- 1

L0F_1F:
	call IncrementByte
	nop
	lblx 9
	tpb 3 // test R9.3
	tr L0F_1F

	lax 0
	out   // R9 <- 0
	excd 0
	exax
	exc 0
	lblx 1
	exc 1
	exax
	exc 1
	rtn

// save SB, X and C
SaveRegs:
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

//------------------------------------------------------------------------------
//	Copyright (c) 2003, Niels Sascha Reedijk
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.


enum registers {
	IDR0 = 0x0 ,
	Command = 0x37 ,
	TxConfig = 0x40 ,
	RxConfig = 0x44 ,
	Config1 = 0x52 , 
	BMCR = 0x62 , 		//BAsic Mode Configuration Register
	BMSR = 0x64 ,
	RBSTART = 0x30 ,
	IMR = 0x3C ,		//Interrupt mask registers
	ISR = 0x3E ,
	_9346CR = 0x50 , 	// 9346 Configuration register
	Config4 = 0x5a ,
	TSD0 = 0x10 ,
	TSD1 = 0x14 ,
	MULINT = 0x5c ,		//Multiple interrupt 
	TSD2 = 0x18 ,
	TSD3 = 0x1C ,
	ESRS = 0x36 ,
	TSAD0 = 0x20 ,
	TSAD1 = 0x24 ,
	TSAD2 = 0x28 ,
	TSAD3 = 0x2C ,
	TSAD = 0x60 ,		//Transmit Status of All Descriptors
	CAPR = 0x38 ,		//Tail pointer for buffer thingie
	CBR = 0x3A ,		//Offset in rxbuffer of next packet
	MPC = 0x4C ,
	MAR0 = 0x8			//Multicast register
	
};


enum Command_actions {
	Reset = 0x10 ,				// 5th bit 
	EnableReceive = 0x08 ,		// 4th bit
	EnableTransmit = 0x04 ,		// 3rd bit
	BUFE = 0x01
};

enum Transmitter_actions {
	MXDMA_2 = 0x400 ,			// 11th bit
	MXDMA_1 = 0x200 ,			// 10th bit
	MXDMA_0 = 0x100	,			// 9th bit
	IFG_1 = 0x2000000 ,			// 26th bit
	IFG_0 = 0x1000000 			// 25th bit
};

enum Receiver_actions {
	RXFTH2 = 0x8000, 			// 16th bit
	RXFTH1 = 0x4000, 			// 15th bit
	RXFTH0 = 0x2000 ,           // 14th bit
	RBLEN_1 = 0x1000 ,			// 13rd bit
	RBLEN_0 = 0x800	,			// 12th bit
	// MXDMA equal to transmitter
	WRAP = 0x100 ,				// 8th bit
	AB = 0x8 ,					// 4rd bit
	AM = 0x4 , 					// 3rd bit
	APM = 0x2					// 2nd bit
};

enum TransmitDescription {
	OWN = 0x2000 ,
	TUN = 0x4000 ,
	TOK = 0x8000 ,
	OWC = 0x20000000 ,
	TABT = 0x40000000 ,
	CRS = 0x80000000
};
	

enum InterruptStatusBits {
	ReceiveOk = 0x01 ,
	ReceiveError = 0x02 ,
	TransmitOk = 0x04 ,
	TransmitError = 0x08 ,
	ReceiveOverflow = 0x10 ,
	ReceiveUnderflow = 0x20 ,
	ReceiveFIFOOverrun = 0x40 ,
	TimeOut = 0x4000 ,
	SystemError = 0x8000
};

enum BMCR_Commands {
	ANE = 0x2000 , 				// Enable auto config
	Duplex_Mode = 0x100 ,
	Restart_Auto_Negotiation = 0x400
};

typedef enum chiptype { RTL_8139_C , RTL_8139_D , RTL_8101_L } chiptype;


#include "includes.h"
#include "pci.h"
#include "ether.h"
#include "nic.h"




#define	XCVR_MAGIC	(0x5A00)
/** any single transmission fails after 16 collisions or other errors
 ** this is the number of times to retry the transmission -- this should
 ** be plenty
 **/
#define	XMIT_RETRIES	250

#undef	virt_to_bus
#define	virt_to_bus(x)	(FP_OFF(x) + FP_SEG(x) * 16)

/*** Register definitions for the 3c905 ***/
enum Registers
    {
    regPowerMgmtCtrl_w = 0x7c,        /** 905B Revision Only                 **/
    regUpMaxBurst_w = 0x7a,           /** 905B Revision Only                 **/
    regDnMaxBurst_w = 0x78,           /** 905B Revision Only                 **/
    regDebugControl_w = 0x74,         /** 905B Revision Only                 **/
    regDebugData_l = 0x70,            /** 905B Revision Only                 **/
    regRealTimeCnt_l = 0x40,          /** Universal                          **/
    regUpBurstThresh_b = 0x3e,        /** 905B Revision Only                 **/
    regUpPoll_b = 0x3d,               /** 905B Revision Only                 **/
    regUpPriorityThresh_b = 0x3c,     /** 905B Revision Only                 **/
    regUpListPtr_l = 0x38,            /** Universal                          **/
    regCountdown_w = 0x36,            /** Universal                          **/
    regFreeTimer_w = 0x34,            /** Universal                          **/
    regUpPktStatus_l = 0x30,          /** Universal with Exception, pg 130   **/
    regTxFreeThresh_b = 0x2f,         /** 90X Revision Only                  **/
    regDnPoll_b = 0x2d,               /** 905B Revision Only                 **/
    regDnPriorityThresh_b = 0x2c,     /** 905B Revision Only                 **/
    regDnBurstThresh_b = 0x2a,        /** 905B Revision Only                 **/
    regDnListPtr_l = 0x24,            /** Universal with Exception, pg 107   **/
    regDmaCtrl_l = 0x20,              /** Universal with Exception, pg 106   **/
				      /**                                    **/
    regIntStatusAuto_w = 0x1e,        /** 905B Revision Only                 **/
    regTxStatus_b = 0x1b,             /** Universal with Exception, pg 113   **/
    regTimer_b = 0x1a,                /** Universal                          **/
    regTxPktId_b = 0x18,              /** 905B Revision Only                 **/
    regCommandIntStatus_w = 0x0e,     /** Universal (Command Variations)     **/
    };

/** following are windowed registers **/
enum Registers7
    {
    regPowerMgmtEvent_7_w = 0x0c,     /** 905B Revision Only                 **/
    regVlanEtherType_7_w = 0x04,      /** 905B Revision Only                 **/
    regVlanMask_7_w = 0x00,           /** 905B Revision Only                 **/
    };

enum Registers6
    {
    regBytesXmittedOk_6_w = 0x0c,     /** Universal                          **/
    regBytesRcvdOk_6_w = 0x0a,        /** Universal                          **/
    regUpperFramesOk_6_b = 0x09,      /** Universal                          **/
    regFramesDeferred_6_b = 0x08,     /** Universal                          **/
    regFramesRecdOk_6_b = 0x07,       /** Universal with Exceptions, pg 142  **/
    regFramesXmittedOk_6_b = 0x06,    /** Universal                          **/
    regRxOverruns_6_b = 0x05,         /** Universal                          **/
    regLateCollisions_6_b = 0x04,     /** Universal                          **/
    regSingleCollisions_6_b = 0x03,   /** Universal                          **/
    regMultipleCollisions_6_b = 0x02, /** Universal                          **/
    regSqeErrors_6_b = 0x01,          /** Universal                          **/
    regCarrierLost_6_b = 0x00,        /** Universal                          **/
    };

enum Registers5
    {
    regIndicationEnable_5_w = 0x0c,   /** Universal                          **/
    regInterruptEnable_5_w = 0x0a,    /** Universal                          **/
    regTxReclaimThresh_5_b = 0x09,    /** 905B Revision Only                 **/
    regRxFilter_5_b = 0x08,           /** Universal                          **/
    regRxEarlyThresh_5_w = 0x06,      /** Universal                          **/
    regTxStartThresh_5_w = 0x00,      /** Universal                          **/
    };

enum Registers4
    {
    regUpperBytesOk_4_b = 0x0d,       /** Universal                          **/
    regBadSSD_4_b = 0x0c,             /** Universal                          **/
    regMediaStatus_4_w = 0x0a,        /** Universal with Exceptions, pg 201  **/
    regPhysicalMgmt_4_w = 0x08,       /** Universal                          **/
    regNetworkDiagnostic_4_w = 0x06,  /** Universal with Exceptions, pg 203  **/
    regFifoDiagnostic_4_w = 0x04,     /** Universal with Exceptions, pg 196  **/
    regVcoDiagnostic_4_w = 0x02,      /** Undocumented?                      **/
    };

enum Registers3
    {
    regTxFree_3_w = 0x0c,             /** Universal                          **/
    regRxFree_3_w = 0x0a,             /** Universal with Exceptions, pg 125  **/
    regResetMediaOptions_3_w = 0x08,  /** Media Options on B Revision,       **/
                                      /** Reset Options on Non-B Revision    **/
    regMacControl_3_w = 0x06,         /** Universal with Exceptions, pg 199  **/
    regMaxPktSize_3_w = 0x04,         /** 905B Revision Only                 **/
    regInternalConfig_3_l = 0x00,     /** Universal, different bit           **/
                                      /** definitions, pg 59                 **/
    };

enum Registers2
    {
    regResetOptions_2_w = 0x0c,       /** 905B Revision Only                 **/
    regStationMask_2_3w = 0x06,       /** Universal with Exceptions, pg 127  **/
    regStationAddress_2_3w = 0x00,    /** Universal with Exceptions, pg 127  **/
    };

enum Registers1
    {
    regRxStatus_1_w = 0x0a,           /** 90X Revision Only, Pg 126          **/
    };

enum Registers0
    {
    regEepromData_0_w = 0x0c,         /** Universal                          **/
    regEepromCommand_0_w = 0x0a,      /** Universal                          **/
    regBiosRomData_0_b = 0x08,        /** 905B Revision Only                 **/
    regBiosRomAddr_0_l = 0x04,        /** 905B Revision Only                 **/
    };


/*** The names for the eight register windows ***/
enum Windows
    {
    winPowerVlan7 = 0x07,
    winStatistics6 = 0x06,
    winTxRxControl5 = 0x05,
    winDiagnostics4 = 0x04,
    winTxRxOptions3 = 0x03,
    winAddressing2 = 0x02,
    winUnused1 = 0x01,
    winEepromBios0 = 0x00,
    };


/*** Command definitions for the 3c90X ***/
enum Commands
    {
    cmdGlobalReset = 0x00,             /** Universal with Exceptions, pg 151 **/
    cmdSelectRegisterWindow = 0x01,    /** Universal                         **/
    cmdEnableDcConverter = 0x02,       /**                                   **/
    cmdRxDisable = 0x03,               /**                                   **/
    cmdRxEnable = 0x04,                /** Universal                         **/
    cmdRxReset = 0x05,                 /** Universal                         **/
    cmdStallCtl = 0x06,                /** Universal                         **/
    cmdTxEnable = 0x09,                /** Universal                         **/
    cmdTxDisable = 0x0A,               /**                                   **/
    cmdTxReset = 0x0B,                 /** Universal                         **/
    cmdRequestInterrupt = 0x0C,        /**                                   **/
    cmdAcknowledgeInterrupt = 0x0D,    /** Universal                         **/
    cmdSetInterruptEnable = 0x0E,      /** Universal                         **/
    cmdSetIndicationEnable = 0x0F,     /** Universal                         **/
    cmdSetRxFilter = 0x10,             /** Universal                         **/
    cmdSetRxEarlyThresh = 0x11,        /**                                   **/
    cmdSetTxStartThresh = 0x13,        /**                                   **/
    cmdStatisticsEnable = 0x15,        /**                                   **/
    cmdStatisticsDisable = 0x16,       /**                                   **/
    cmdDisableDcConverter = 0x17,      /**                                   **/
    cmdSetTxReclaimThresh = 0x18,      /**                                   **/
    cmdSetHashFilterBit = 0x19,        /**                                   **/
    };


/*** Values for int status register bitmask **/
#define	INT_INTERRUPTLATCH	(1<<0)
#define INT_HOSTERROR		(1<<1)
#define INT_TXCOMPLETE		(1<<2)
#define INT_RXCOMPLETE		(1<<4)
#define INT_RXEARLY		(1<<5)
#define INT_INTREQUESTED	(1<<6)
#define INT_UPDATESTATS		(1<<7)
#define INT_LINKEVENT		(1<<8)
#define INT_DNCOMPLETE		(1<<9)
#define INT_UPCOMPLETE		(1<<10)
#define INT_CMDINPROGRESS	(1<<12)
#define INT_WINDOWNUMBER	(7<<13)


/*** TX descriptor ***/
typedef struct
    {
    unsigned long	DnNextPtr;
    unsigned long	FrameStartHeader;
    unsigned long	HdrAddr;
    unsigned long	HdrLength;
    unsigned long	DataAddr;
    unsigned long	DataLength;
    }
    TXD;

/*** RX descriptor ***/
typedef struct
    {
    unsigned long	UpNextPtr;
    unsigned long	UpPktStatus;
    unsigned long	DataAddr;
    unsigned long	DataLength;
    }
    RXD;

/*** Global variables ***/
static struct
    {
    unsigned char	isBrev;
    unsigned char	CurrentWindow;
    unsigned long	IOAddr;
    unsigned char	HWAddr[ETH_ALEN];
    TXD			TransmitDPD;
    RXD			ReceiveUPD;
    }
    INF_3C90X;


/*** a3c90x_internal_IssueCommand: sends a command to the 3c90x card
 ***/
static int
a3c90x_internal_IssueCommand(unsigned short ioaddr, int cmd, int param)
    {
    unsigned long val;

	/** Build the cmd. **/
	val = cmd;
	val <<= 11;
	val |= param;

	/** Send the cmd to the cmd register **/
	NETCARD_OUTW(val, ioaddr + regCommandIntStatus_w);

	/** Wait for the cmd to complete, if necessary **/
	while (NETCARD_INW(ioaddr + regCommandIntStatus_w) & INT_CMDINPROGRESS);

    return 0;
    }


/*** a3c90x_internal_SetWindow: selects a register window set.
 ***/
static int
a3c90x_internal_SetWindow(unsigned short ioaddr, int window)
    {

	/** Window already as set? **/
	if (INF_3C90X.CurrentWindow == window) return 0;

	/** Issue the window command. **/
	a3c90x_internal_IssueCommand(ioaddr, cmdSelectRegisterWindow, window);
	INF_3C90X.CurrentWindow = window;

    return 0;
    }


/*** a3c90x_internal_ReadEeprom - read data from the serial eeprom.
 ***/
static unsigned short
a3c90x_internal_ReadEeprom(unsigned short ioaddr, int address)
    {
    unsigned short val;

	/** Select correct window **/
        a3c90x_internal_SetWindow(INF_3C90X.IOAddr, winEepromBios0);

	/** Make sure the eeprom isn't busy **/
	while((1<<15) & NETCARD_INW(ioaddr + regEepromCommand_0_w));

	/** Read the value. **/
	NETCARD_OUTW(address + ((0x02)<<6), ioaddr + regEepromCommand_0_w);
	while((1<<15) & NETCARD_INW(ioaddr + regEepromCommand_0_w));
	val = NETCARD_INW(ioaddr + regEepromData_0_w);

    return val;
    }


/*** a3c90x_internal_WriteEepromWord - write a physical word of
 *** data to the onboard serial eeprom (not the BIOS prom, but the
 *** nvram in the card that stores, among other things, the MAC
 *** address).
 ***/
static int
a3c90x_internal_WriteEepromWord(unsigned short ioaddr, int address, unsigned short value)
    {
	/** Select register window **/
	a3c90x_internal_SetWindow(ioaddr, winEepromBios0);

	/** Verify Eeprom not busy **/
	while((1<<15) & NETCARD_INW(ioaddr + regEepromCommand_0_w));

	/** Issue WriteEnable, and wait for completion. **/
	NETCARD_OUTW(0x30, ioaddr + regEepromCommand_0_w);
	while((1<<15) & NETCARD_INW(ioaddr + regEepromCommand_0_w));

	/** Issue EraseRegister, and wait for completion. **/
	NETCARD_OUTW(address + ((0x03)<<6), ioaddr + regEepromCommand_0_w);
	while((1<<15) & NETCARD_INW(ioaddr + regEepromCommand_0_w));

	/** Send the new data to the eeprom, and wait for completion. **/
	NETCARD_OUTW(value, ioaddr + regEepromData_0_w);
	NETCARD_OUTW(0x30, ioaddr + regEepromCommand_0_w);
	while((1<<15) & NETCARD_INW(ioaddr + regEepromCommand_0_w));

	/** Burn the new data into the eeprom, and wait for completion. **/
	NETCARD_OUTW(address + ((0x01)<<6), ioaddr + regEepromCommand_0_w);
	while((1<<15) & NETCARD_INW(ioaddr + regEepromCommand_0_w));

    return 0;
    }


/*** a3c90x_internal_WriteEeprom - write data to the serial eeprom,
 *** and re-compute the eeprom checksum.
 ***/
static int
a3c90x_internal_WriteEeprom(unsigned short ioaddr, int address, unsigned short value)
    {
    int cksum = 0,v;
    int i;
    int maxAddress, cksumAddress;

	if (INF_3C90X.isBrev)
	    {
	    maxAddress=0x1f;
	    cksumAddress=0x20;
	    }
	else
	    {
	    maxAddress=0x16;
	    cksumAddress=0x17;
	    }

	/** Write the value. **/
	if (a3c90x_internal_WriteEepromWord(ioaddr, address, value) == -1)
	    return -1;

	/** Recompute the checksum. **/
	for(i=0;i<=maxAddress;i++)
	    {
	    v = a3c90x_internal_ReadEeprom(ioaddr, i);
	    cksum ^= (v & 0xFF);
	    cksum ^= ((v>>8) & 0xFF);
	    }
	/** Write the checksum to the location in the eeprom **/
	if (a3c90x_internal_WriteEepromWord(ioaddr, cksumAddress, cksum) == -1)
	    return -1;

    return 0;
    }

#if 0


/*** a3c90x_reset: exported function that resets the card to its default
 *** state.  This is so the Linux driver can re-set the card up the way
 *** it wants to.  If CFG_3C90X_PRESERVE_XCVR is defined, then the reset will
 *** not alter the selected transceiver that we used to download the boot
 *** image.
 ***/
static void
a3c90x_reset(struct nic *nic)
    {
    int cfg;

#ifdef	CFG_3C90X_PRESERVE_XCVR
    /** Read the current InternalConfig value. **/
    a3c90x_internal_SetWindow(INF_3C90X.IOAddr, winTxRxOptions3);
    cfg = NETCARD_IND(INF_3C90X.IOAddr + regInternalConfig_3_l);
#endif

    /** Send the reset command to the card **/
    debug_print("Issuing RESET:\n");
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdGlobalReset, 0);

    /** wait for reset command to complete **/
    while (NETCARD_INW(INF_3C90X.IOAddr + regCommandIntStatus_w) & INT_CMDINPROGRESS);

    /** global reset command resets station mask, non-B revision cards
     ** require explicit reset of values
     **/
    a3c90x_internal_SetWindow(INF_3C90X.IOAddr, winAddressing2);
    NETCARD_OUTW(0, INF_3C90X.IOAddr + regStationMask_2_3w+0);
    NETCARD_OUTW(0, INF_3C90X.IOAddr + regStationMask_2_3w+2);
    NETCARD_OUTW(0, INF_3C90X.IOAddr + regStationMask_2_3w+4);

#ifdef	CFG_3C90X_PRESERVE_XCVR
    /** Re-set the original InternalConfig value from before reset **/
    a3c90x_internal_SetWindow(INF_3C90X.IOAddr, winTxRxOptions3);
    NETCARD_OUTD(cfg, INF_3C90X.IOAddr + regInternalConfig_3_l);

    /** enable DC converter for 10-Base-T **/
    if ((cfg&0x0300) == 0x0300)
	{
	a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdEnableDcConverter, 0);
	}
#endif

    /** Issue transmit reset, wait for command completion **/
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdTxReset, 0);
    while (NETCARD_INW(INF_3C90X.IOAddr + regCommandIntStatus_w) & INT_CMDINPROGRESS)
	;
    if (! INF_3C90X.isBrev)
	NETCARD_OUTB(0x01, INF_3C90X.IOAddr + regTxFreeThresh_b);
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdTxEnable, 0);

    /**
     ** reset of the receiver on B-revision cards re-negotiates the link
     ** takes several seconds (a computer eternity)
     **/
    if (INF_3C90X.isBrev)
	a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdRxReset, 0x04);
    else
	a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdRxReset, 0x00);
    while (NETCARD_INW(INF_3C90X.IOAddr + regCommandIntStatus_w) & INT_CMDINPROGRESS);
	;
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdRxEnable, 0);

    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr,
                                 cmdSetInterruptEnable, 0);
    /** enable rxComplete and txComplete **/
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr,
                                 cmdSetIndicationEnable, 0x0014);
    /** acknowledge any pending status flags **/
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr,
                                 cmdAcknowledgeInterrupt, 0x661);

    return;
    }



/*** a3c90x_transmit: exported function that transmits a packet.  Does not
 *** return any particular status.  Parameters are:
 *** d[6] - destination address, ethernet;
 *** t - protocol type (ARP, IP, etc);
 *** s - size of the non-header part of the packet that needs transmitted;
 *** p - the pointer to the packet data itself.
 ***/
static void
a3c90x_transmit(struct nic *nic, const char *d, unsigned long t,
		unsigned long s, const char *p)
    {

    struct eth_hdr
	{
	unsigned char dst_addr[ETH_ALEN];
	unsigned char src_addr[ETH_ALEN];
	unsigned short type;
	} hdr;

    unsigned char status;
    unsigned i, retries;

    for (retries=0; retries < XMIT_RETRIES ; retries++)
	{
	/** Stall the download engine **/
	a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdStallCtl, 2);

	/** Make sure the card is not waiting on us **/
	NETCARD_INW(INF_3C90X.IOAddr + regCommandIntStatus_w);
	NETCARD_INW(INF_3C90X.IOAddr + regCommandIntStatus_w);

	while (NETCARD_INW(INF_3C90X.IOAddr+regCommandIntStatus_w) &
	       INT_CMDINPROGRESS)
	    ;

	/** Set the ethernet packet type **/
	hdr.type = htons(t);

	/** Copy the destination address **/
	memcpy(hdr.dst_addr, d, ETH_ALEN);

	/** Copy our MAC address **/
	memcpy(hdr.src_addr, INF_3C90X.HWAddr, ETH_ALEN);

	/** Setup the DPD (download descriptor) **/
	INF_3C90X.TransmitDPD.DnNextPtr = 0;
	/** set notification for transmission completion (bit 15) **/
	INF_3C90X.TransmitDPD.FrameStartHeader = (s + sizeof(hdr)) | 0x8000;
	INF_3C90X.TransmitDPD.HdrAddr = virt_to_bus(&hdr);
	INF_3C90X.TransmitDPD.HdrLength = sizeof(hdr);
	INF_3C90X.TransmitDPD.DataAddr = virt_to_bus(p);
	INF_3C90X.TransmitDPD.DataLength = s + (1<<31);

	/** Send the packet **/
	NETCARD_OUTD(virt_to_bus(&(INF_3C90X.TransmitDPD)),
	     INF_3C90X.IOAddr + regDnListPtr_l);

	/** End Stall and Wait for upload to complete. **/
	a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdStallCtl, 3);
	while(NETCARD_IND(INF_3C90X.IOAddr + regDnListPtr_l) != 0)
	    ;

	/** Wait for NIC Transmit to Complete **/
	load_timer2(10*TICKS_PER_MS);	/* Give it 10 ms */
	while (!(NETCARD_INW(INF_3C90X.IOAddr + regCommandIntStatus_w)&0x0004) &&
		timer2_running())
		;

	if (!(NETCARD_INW(INF_3C90X.IOAddr + regCommandIntStatus_w)&0x0004))
	    {
	    debug_print("3C90X: Tx Timeout\n");
	    continue;
	    }

	status = NETCARD_INB(INF_3C90X.IOAddr + regTxStatus_b);

	/** acknowledge transmit interrupt by writing status **/
	NETCARD_OUTB(0x00, INF_3C90X.IOAddr + regTxStatus_b);

	/** successful completion (sans "interrupt Requested" bit) **/
	if ((status & 0xbf) == 0x80)
	    return;

	   debug_print("3C90X: Status (%hhX)\n", status);
	/** check error codes **/
	if (status & 0x02)
	    {
	    debug_print("3C90X: Tx Reclaim Error (%hhX)\n", status);
	    a3c90x_reset(NULL);
	    }
	else if (status & 0x04)
	    {
	    debug_print("3C90X: Tx Status Overflow (%hhX)\n", status);
	    for (i=0; i<32; i++)
		NETCARD_OUTB(0x00, INF_3C90X.IOAddr + regTxStatus_b);
	    /** must re-enable after max collisions before re-issuing tx **/
	    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdTxEnable, 0);
	    }
	else if (status & 0x08)
	    {
	    debug_print("3C90X: Tx Max Collisions (%hhX)\n", status);
	    /** must re-enable after max collisions before re-issuing tx **/
	    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdTxEnable, 0);
	    }
	else if (status & 0x10)
	    {
	    debug_print("3C90X: Tx Underrun (%hhX)\n", status);
	    a3c90x_reset(NULL);
	    }
	else if (status & 0x20)
	    {
	    debug_print("3C90X: Tx Jabber (%hhX)\n", status);
	    a3c90x_reset(NULL);
	    }
	else if ((status & 0x80) != 0x80)
	    {
	    debug_print("3C90X: Internal Error - Incomplete Transmission (%hhX)\n",
	           status);
	    a3c90x_reset(NULL);
	    }
	}

    /** failed after RETRY attempts **/
    debug_print("Failed to send after %d retries\n", retries);
    return;

    }

#endif



/*** a3c90x_poll: exported routine that waits for a certain length of time
 *** for a packet, and if it sees none, returns 0.  This routine should
 *** copy the packet to nic->packet if it gets a packet and set the size
 *** in nic->packetlen.  Return 1 if a packet was found.
 ***/
static int
a3c90x_poll(struct nic *nic,unsigned char *buf,int *buflen)
    {
    unsigned long i;
    int  errcode;

    if (!(NETCARD_INW(INF_3C90X.IOAddr + regCommandIntStatus_w)&0x0010))
    {
	return 0;
    }

    /** we don't need to acknowledge rxComplete -- the upload engine
     ** does it for us.
     **/
    printf("step 1 \n");

    /** Build the up-load descriptor **/
    INF_3C90X.ReceiveUPD.UpNextPtr = 0;
    INF_3C90X.ReceiveUPD.UpPktStatus = 0;
    INF_3C90X.ReceiveUPD.DataAddr = virt_to_bus(buf);
    INF_3C90X.ReceiveUPD.DataLength = 1536 + (1<<31);

    printf("step 2 . \n");
    /** Submit the upload descriptor to the NIC **/
    NETCARD_OUTD(virt_to_bus(&(INF_3C90X.ReceiveUPD)),
	 INF_3C90X.IOAddr + regUpListPtr_l);

    printf("step . \n");

    /** Wait for upload completion (upComplete(15) or upError (14)) **/
    for(i=0;i<40000;i++);
    while((INF_3C90X.ReceiveUPD.UpPktStatus & ((1<<14) | (1<<15))) == 0)
	for(i=0;i<40000;i++);

	printf("step 4 \n");
    /** Check for Error (else we have good packet) **/
    if (INF_3C90X.ReceiveUPD.UpPktStatus & (1<<14))
	{
	errcode = INF_3C90X.ReceiveUPD.UpPktStatus;
	if (errcode & (1<<16))
	    debug_print("3C90X: Rx Overrun (%hX)\n",errcode>>16);
	else if (errcode & (1<<17))
	    debug_print("3C90X: Runt Frame (%hX)\n",errcode>>16);
	else if (errcode & (1<<18))
	    debug_print("3C90X: Alignment Error (%hX)\n",errcode>>16);
	else if (errcode & (1<<19))
	    debug_print("3C90X: CRC Error (%hX)\n",errcode>>16);
	else if (errcode & (1<<20))
	    debug_print("3C90X: Oversized Frame (%hX)\n",errcode>>16);
	else
	    debug_print("3C90X: Packet error (%hX)\n",errcode>>16);
	return 0;
	}

    /** Ok, got packet.  Set length in nic->packetlen. **/
    *buflen = (INF_3C90X.ReceiveUPD.UpPktStatus & 0x1FFF);

    return 1;
    }


/*** a3c90x_disable: exported routine to disable the card.  What's this for?
 *** the eepro100.c driver didn't have one, so I just left this one empty too.
 *** Ideas anyone?
 *** Must turn off receiver at least so stray packets will not corrupt memory
 *** [Ken]
 ***/
static void
a3c90x_disable(struct nic *nic)
    {
	/* Disable the receiver and transmitter. */
	NETCARD_OUTW(cmdRxDisable, INF_3C90X.IOAddr + regCommandIntStatus_w);
	NETCARD_OUTW(cmdTxDisable, INF_3C90X.IOAddr + regCommandIntStatus_w);
    }


/*** a3c90x_probe: exported routine to probe for the 3c905 card and perform
 *** initialization.  If this routine is called, the pci functions did find the
 *** card.  We just have to init it here.
 ***/
int
a3c90x_probe(struct nic *nic)
    {
    int i, c;
    struct pci_device *pci = nic->pci_data;
    unsigned short eeprom[0x21];
    unsigned long cfg;
    unsigned long mopt;
    unsigned short linktype;

    unsigned short pci_command;
    unsigned short new_command;
    unsigned char pci_latency;

    if (nic->ioaddr == 0)
	  return -1;
    if (pci == NULL)
	  return -1;

    /* Make certain the card is properly set up as a bus master. */
    pcibios_read_config_word(pci->bus, pci->devfn, PCI_COMMAND, &pci_command);
    new_command = pci_command | PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY | PCI_COMMAND_IO;
    if (pci_command != new_command) {
	    debug_print("\nThe PCI BIOS has not enabled this device!\nUpdating PCI command %hX->%hX. pci_bus %hhX pci_device_fn %hhX\n",
		    pci_command, new_command, pci->bus, pci->devfn);
	    pcibios_write_config_word(pci->bus, pci->devfn, PCI_COMMAND, new_command);
    }
    pcibios_read_config_byte(pci->bus, pci->devfn, PCI_LATENCY_TIMER, &pci_latency);
    if (pci_latency < 32) {
	    debug_print("\nPCI latency timer (CFLT) is unreasonably low at %d. Setting to 32 clocks.\n", pci_latency);
	    pcibios_write_config_byte(pci->bus, pci->devfn, PCI_LATENCY_TIMER, 32);
    }

    INF_3C90X.IOAddr = nic->ioaddr;
    INF_3C90X.CurrentWindow = 255;
    switch (a3c90x_internal_ReadEeprom(INF_3C90X.IOAddr, 0x03))
	{
	case 0x9000: /** 10 Base TPO             **/
	case 0x9001: /** 10/100 T4               **/
	case 0x9050: /** 10/100 TPO              **/
	case 0x9051: /** 10 Base Combo           **/
		INF_3C90X.isBrev = 0;
		break;

	case 0x9004: /** 10 Base TPO             **/
	case 0x9005: /** 10 Base Combo           **/
	case 0x9006: /** 10 Base TPO and Base2   **/
	case 0x900A: /** 10 Base FL              **/
	case 0x9055: /** 10/100 TPO              **/
	case 0x9056: /** 10/100 T4               **/
	case 0x905A: /** 10 Base FX              **/
	default:
		INF_3C90X.isBrev = 1;
		break;
	}

    /** Load the EEPROM contents **/
    if (INF_3C90X.isBrev)
	{
	for(i=0;i<=0x20;i++)
	    {
	    eeprom[i] = a3c90x_internal_ReadEeprom(INF_3C90X.IOAddr, i);
	    }

#ifdef	CFG_3C90X_BOOTROM_FIX
	/** Set xcvrSelect in InternalConfig in eeprom. **/
	/* only necessary for 3c905b revision cards with boot PROM bug!!! */
	a3c90x_internal_WriteEeprom(INF_3C90X.IOAddr, 0x13, 0x0160);
#endif

#ifdef	CFG_3C90X_XCVR
	if (CFG_3C90X_XCVR == 255)
	    {
	    /** Clear the LanWorks register **/
	    a3c90x_internal_WriteEeprom(INF_3C90X.IOAddr, 0x16, 0);
	    }
	else
	    {
	    /** Set the selected permanent-xcvrSelect in the
	     ** LanWorks register
	     **/
	    a3c90x_internal_WriteEeprom(INF_3C90X.IOAddr, 0x16,
	                    XCVR_MAGIC + ((CFG_3C90X_XCVR) & 0x000F));
	    }
#endif
	}
    else
	{
	for(i=0;i<=0x17;i++)
	    {
	    eeprom[i] = a3c90x_internal_ReadEeprom(INF_3C90X.IOAddr, i);
	    }
	}

    /** Print identification message **/
    debug_print("\n\n3C90X Driver 2.00 "
	   "Copyright 1999 LightSys Technology Services, Inc.\n"
	   "Portions Copyright 1999 Steve Smith\n");
    debug_print("Provided with ABSOLUTELY NO WARRANTY.\n");
    debug_print("-------------------------------------------------------"
           "------------------------\n");

    /** Retrieve the Hardware address and print it on the screen. **/
    INF_3C90X.HWAddr[0] = eeprom[0]>>8;
    INF_3C90X.HWAddr[1] = eeprom[0]&0xFF;
    INF_3C90X.HWAddr[2] = eeprom[1]>>8;
    INF_3C90X.HWAddr[3] = eeprom[1]&0xFF;
    INF_3C90X.HWAddr[4] = eeprom[2]>>8;
    INF_3C90X.HWAddr[5] = eeprom[2]&0xFF;
    debug_print_buf("MAC Address:", INF_3C90X.HWAddr,ETH_ALEN);

    /** Program the MAC address into the station address registers **/
    a3c90x_internal_SetWindow(INF_3C90X.IOAddr, winAddressing2);
    NETCARD_OUTW(htons(eeprom[0]), INF_3C90X.IOAddr + regStationAddress_2_3w);
    NETCARD_OUTW(htons(eeprom[1]), INF_3C90X.IOAddr + regStationAddress_2_3w+2);
    NETCARD_OUTW(htons(eeprom[2]), INF_3C90X.IOAddr + regStationAddress_2_3w+4);
    NETCARD_OUTW(0, INF_3C90X.IOAddr + regStationMask_2_3w+0);
    NETCARD_OUTW(0, INF_3C90X.IOAddr + regStationMask_2_3w+2);
    NETCARD_OUTW(0, INF_3C90X.IOAddr + regStationMask_2_3w+4);

    /** Fill in our entry in the etherboot arp table **/
    for(i=0;i<ETH_ALEN;i++)
	nic->node_addr[i] = (eeprom[i/2] >> (8*((i&1)^1))) & 0xff;

    /** Read the media options register, print a message and set default
     ** xcvr.
     **
     ** Uses Media Option command on B revision, Reset Option on non-B
     ** revision cards -- same register address
     **/
    a3c90x_internal_SetWindow(INF_3C90X.IOAddr, winTxRxOptions3);
    mopt = NETCARD_INW(INF_3C90X.IOAddr + regResetMediaOptions_3_w);

    /** mask out VCO bit that is defined as 10baseFL bit on B-rev cards **/
    if (! INF_3C90X.isBrev)
	{
	mopt &= 0x7F;
	}

    debug_print("Connectors present: ");
    c = 0;
    linktype = 0x0008;
    if (mopt & 0x01)
	{
	debug_print("%s100Base-T4",(c++)?", ":"");
	linktype = 0x0006;
	}
    if (mopt & 0x04)
	{
	debug_print("%s100Base-FX",(c++)?", ":"");
	linktype = 0x0005;
	}
    if (mopt & 0x10)
	{
	debug_print("%s10Base-2",(c++)?", ":"");
	linktype = 0x0003;
	}
    if (mopt & 0x20)
	{
	debug_print("%sAUI",(c++)?", ":"");
	linktype = 0x0001;
	}
    if (mopt & 0x40)
	{
	debug_print("%sMII",(c++)?", ":"");
	linktype = 0x0006;
	}
    if ((mopt & 0xA) == 0xA)
	{
	debug_print("%s10Base-T / 100Base-TX",(c++)?", ":"");
	linktype = 0x0008;
	}
    else if ((mopt & 0xA) == 0x2)
	{
	debug_print("%s100Base-TX",(c++)?", ":"");
	linktype = 0x0008;
	}
    else if ((mopt & 0xA) == 0x8)
	{
	debug_print("%s10Base-T",(c++)?", ":"");
	linktype = 0x0008;
	}
    debug_print(".\n");

    /** Determine transceiver type to use, depending on value stored in
     ** eeprom 0x16
     **/
    if (INF_3C90X.isBrev)
	{
	if ((eeprom[0x16] & 0xFF00) == XCVR_MAGIC)
	    {
	    /** User-defined **/
	    linktype = eeprom[0x16] & 0x000F;
	    }
	}
    else
	{
#ifdef	CFG_3C90X_XCVR
	    if (CFG_3C90X_XCVR != 255)
		linktype = CFG_3C90X_XCVR;
#endif	/* CFG_3C90X_XCVR */

	    /** I don't know what MII MAC only mode is!!! **/
	    if (linktype == 0x0009)
		{
		if (INF_3C90X.isBrev)
			debug_print("WARNING: MII External MAC Mode only supported on B-revision "
			       "cards!!!!\nFalling Back to MII Mode\n");
		linktype = 0x0006;
		}
	}

    /** enable DC converter for 10-Base-T **/
    if (linktype == 0x0003)
	{
	a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdEnableDcConverter, 0);
	}

    /** Set the link to the type we just determined. **/
    a3c90x_internal_SetWindow(INF_3C90X.IOAddr, winTxRxOptions3);
    cfg = NETCARD_IND(INF_3C90X.IOAddr + regInternalConfig_3_l);
    cfg &= ~(0xF<<20);
    cfg |= (linktype<<20);
    NETCARD_OUTD(cfg, INF_3C90X.IOAddr + regInternalConfig_3_l);

    /** Now that we set the xcvr type, reset the Tx and Rx, re-enable. **/
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdTxReset, 0x00);
    while (NETCARD_INW(INF_3C90X.IOAddr + regCommandIntStatus_w) & INT_CMDINPROGRESS)
	;

    if (!INF_3C90X.isBrev)
	NETCARD_OUTB(0x01, INF_3C90X.IOAddr + regTxFreeThresh_b);

    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdTxEnable, 0);

    /**
     ** reset of the receiver on B-revision cards re-negotiates the link
     ** takes several seconds (a computer eternity)
     **/
    if (INF_3C90X.isBrev)
	a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdRxReset, 0x04);
    else
	a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdRxReset, 0x00);
    while (NETCARD_INW(INF_3C90X.IOAddr + regCommandIntStatus_w) & INT_CMDINPROGRESS)
	;

    /** Set the RX filter = receive only individual pkts & bcast. **/
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdSetRxFilter, 0x01 + 0x04);
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdRxEnable, 0);


    /**
     ** set Indication and Interrupt flags , acknowledge any IRQ's
     **/
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr, cmdSetInterruptEnable, 0);
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr,
				 cmdSetIndicationEnable, 0x0014);
    a3c90x_internal_IssueCommand(INF_3C90X.IOAddr,
				 cmdAcknowledgeInterrupt, 0x661);

    nic->disable = a3c90x_disable;
    nic->poll = a3c90x_poll;
    return 0;
    }



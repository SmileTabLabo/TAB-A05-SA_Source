/*
 * HDMI support
 *
 * Copyright (C) 2013 ITE Tech. Inc.
 * Author: Hermes Wu <hermes.wu@ite.com.tw>
 *
 * HDMI TX driver for IT66121
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "hdmitx.h"
#include "hdmitx_drv.h"
#include "sha1.h"

#ifdef SUPPORT_HDCP
static unsigned char countbit(unsigned char b);
#endif

#ifdef SUPPORT_SHA
unsigned char SHABuff[64];
unsigned char V[20];
unsigned char KSVList[32];
unsigned char Vr[20];
unsigned char M0[8];
#endif

#ifdef SUPPORT_HDCP

bool HDMITX_EnableHDCP(unsigned char bEnable)
{
#ifdef SUPPORT_HDCP
	if (bEnable) {
		if (hdmitx_hdcp_Authenticate() == ER_FAIL) {
			/* IT66121_LOG("ER_FAIL == hdmitx_hdcp_Authenticate\n");
			 */
			hdmitx_hdcp_ResetAuth();
			return FALSE;
		}
	} else {
		hdmiTxDev[0].bAuthenticated = FALSE;
		hdmitx_hdcp_ResetAuth();
	}
#endif
	return TRUE;
}

bool getHDMITX_AuthenticationDone(void)
{
	/* HDCP_DEBUG_PRINTF((" getHDMITX_AuthenticationDone() =
	 * %s\n",hdmiTxDev[0].bAuthenticated?"TRUE":"FALSE" ));
	 */
	return hdmiTxDev[0].bAuthenticated;
}

/* */
/* Authentication */
/* */
void hdmitx_hdcp_ClearAuthInterrupt(void)
{
	/* unsigned char uc ; */
	/* uc = HDMITX_ReadI2C_Byte(REG_TX_INT_MASK2) &*/
	/* (~(B_TX_KSVLISTCHK_MASK|B_TX_AUTH_DONE_MASK|B_TX_AUTH_FAIL_MASK)); */
	Switch_HDMITX_Bank(0);
	HDMITX_SetI2C_Byte(REG_TX_INT_MASK2,
			   B_TX_KSVLISTCHK_MASK | B_TX_AUTH_DONE_MASK |
				   B_TX_AUTH_FAIL_MASK,
			   0);
	HDMITX_WriteI2C_Byte(REG_TX_INT_CLR0, B_TX_CLR_AUTH_FAIL |
						      B_TX_CLR_AUTH_DONE |
						      B_TX_CLR_KSVLISTCHK);
	HDMITX_WriteI2C_Byte(REG_TX_INT_CLR1, 0);
	HDMITX_WriteI2C_Byte(REG_TX_SYS_STATUS, B_TX_INTACTDONE);
}

void hdmitx_hdcp_ResetAuth(void)
{
	Switch_HDMITX_Bank(0);
	HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL, 0);
	HDMITX_WriteI2C_Byte(REG_TX_HDCP_DESIRE, 0);
	HDMITX_OrReg_Byte(REG_TX_SW_RST, B_TX_HDCP_RST_HDMITX);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,
			     B_TX_MASTERDDC | B_TX_MASTERHOST);
	hdmitx_hdcp_ClearAuthInterrupt();
	hdmitx_AbortDDC();
}

/* //////////////////////////////////////////////////////////////////// */
/* Function: hdmitx_hdcp_Auth_Fire() */
/* Parameter: N/A */
/* Return: N/A */
/* Remark: write anything to reg21 to enable HDCP authentication by HW */
/* Side-Effect: N/A */
/* //////////////////////////////////////////////////////////////////// */

void hdmitx_hdcp_Auth_Fire(void)
{
	/* HDCP_DEBUG_PRINTF(("hdmitx_hdcp_Auth_Fire():\n")); */
	HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,
			     B_TX_MASTERDDC | B_TX_MASTERHDCP);
	/* MASTERHDCP,no need command but fire. */
	HDMITX_WriteI2C_Byte(REG_TX_AUTHFIRE, 1);
}

/* //////////////////////////////////////////////////////////////////// */
/* Function: hdmitx_hdcp_StartAnCipher */
/* Parameter: N/A */
/* Return: N/A */
/* Remark: Start the Cipher to free run for random number. When stop,An is */
/* ready in Reg30. */
/* Side-Effect: N/A */
/* //////////////////////////////////////////////////////////////////// */

void hdmitx_hdcp_StartAnCipher(void)
{
	HDMITX_WriteI2C_Byte(REG_TX_AN_GENERATE, B_TX_START_CIPHER_GEN);
	delay1ms(1); /* delay 1 ms */
}

/* //////////////////////////////////////////////////////////////////// */
/* Function: hdmitx_hdcp_StopAnCipher */
/* Parameter: N/A */
/* Return: N/A */
/* Remark: Stop the Cipher,and An is ready in Reg30. */
/* Side-Effect: N/A */
/* //////////////////////////////////////////////////////////////////// */

void hdmitx_hdcp_StopAnCipher(void)
{
	HDMITX_WriteI2C_Byte(REG_TX_AN_GENERATE, B_TX_STOP_CIPHER_GEN);
}

/* //////////////////////////////////////////////////////////////////// */
/* Function: hdmitx_hdcp_GenerateAn */
/* Parameter: N/A */
/* Return: N/A */
/* Remark: start An ciper random run at first,then stop it. Software can get */
/* an in reg30~reg38,the write to reg28~2F */
/* Side-Effect: */
/* //////////////////////////////////////////////////////////////////// */

void hdmitx_hdcp_GenerateAn(void)
{
	unsigned char Data[8];
	unsigned char i = 0;
#if 1
	hdmitx_hdcp_StartAnCipher();
	/* HDMITX_WriteI2C_Byte(REG_TX_AN_GENERATE,B_TX_START_CIPHER_GEN); */
	/* delay1ms(1); // delay 1 ms */
	/* HDMITX_WriteI2C_Byte(REG_TX_AN_GENERATE,B_TX_STOP_CIPHER_GEN); */

	hdmitx_hdcp_StopAnCipher();

	Switch_HDMITX_Bank(0);
	/* new An is ready in reg30 */
	HDMITX_ReadI2C_ByteN(REG_TX_AN_GEN, Data, 8);
#else
	Data[0] = 0;
	Data[1] = 0;
	Data[2] = 0;
	Data[3] = 0;
	Data[4] = 0;
	Data[5] = 0;
	Data[6] = 0;
	Data[7] = 0;
#endif
	HDCP_DEBUG_PRINTF(
		"An={0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}\n",
		Data[0], Data[1], Data[2], Data[3],
		Data[4], Data[5], Data[6], Data[7]);
	for (i = 0; i < 8; i++)
		HDMITX_WriteI2C_Byte(REG_TX_AN + i, Data[i]);
	/* HDMITX_WriteI2C_ByteN(REG_TX_AN,Data,8); */
}

/* //////////////////////////////////////////////////////////////////// */
/* Function: hdmitx_hdcp_GetBCaps */
/* Parameter: pBCaps - pointer of byte to get BCaps. */
/* pBStatus - pointer of two bytes to get BStatus */
/* Return: ER_SUCCESS if successfully got BCaps and BStatus. */
/* Remark: get B status and capability from HDCP receiver via DDC bus. */
/* Side-Effect: */
/* //////////////////////////////////////////////////////////////////// */

SYS_STATUS hdmitx_hdcp_GetBCaps(unsigned char *pBCaps, unsigned short *pBStatus)
{
	unsigned char ucdata;
	unsigned char TimeOut;

	Switch_HDMITX_Bank(0);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,
			     B_TX_MASTERDDC | B_TX_MASTERHOST);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_HEADER, DDC_HDCP_ADDRESS);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_REQOFF, 0x40); /* BCaps offset */
	HDMITX_WriteI2C_Byte(REG_TX_DDC_REQCOUNT, 3);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD, CMD_DDC_SEQ_BURSTREAD);

	for (TimeOut = 200; TimeOut > 0; TimeOut--) {
		delay1ms(1);

		ucdata = HDMITX_ReadI2C_Byte(REG_TX_DDC_STATUS);

		if (ucdata & B_TX_DDC_DONE) {
			/* HDCP_DEBUG_PRINTF(("hdmitx_hdcp_GetBCaps(): DDC
			 * Done.\n"));
			 */
			break;
		}
		if (ucdata & B_TX_DDC_ERROR) {
			/* HDCP_DEBUG_PRINTF(("hdmitx_hdcp_GetBCaps(): DDC fail
			 * by reg16=%02X.\n",ucdata));
			 */
			return ER_FAIL;
		}
	}
	if (TimeOut == 0)
		return ER_FAIL;
#if 1
	ucdata = HDMITX_ReadI2C_Byte(REG_TX_BSTAT + 1);

	*pBStatus = (unsigned short)ucdata;
	*pBStatus <<= 8;
	ucdata = HDMITX_ReadI2C_Byte(REG_TX_BSTAT);
	*pBStatus |= ((unsigned short)ucdata & 0xFF);
	*pBCaps = HDMITX_ReadI2C_Byte(REG_TX_BCAP);
#else
	*pBCaps = HDMITX_ReadI2C_Byte(0x17);
	*pBStatus = HDMITX_ReadI2C_Byte(0x17) & 0xFF;
	*pBStatus |= (int)(HDMITX_ReadI2C_Byte(0x17) & 0xFF) << 8;
	HDCP_DEBUG_PRINTF("%s(): ucdata = %02X\n", __func__,
			  (int)HDMITX_ReadI2C_Byte(0x16));
#endif
	return ER_SUCCESS;
}

/* //////////////////////////////////////////////////////////////////// */
/* Function: hdmitx_hdcp_GetBKSV */
/* Parameter: pBKSV - pointer of 5 bytes buffer for getting BKSV */
/* Return: ER_SUCCESS if successfully got BKSV from Rx. */
/* Remark: Get BKSV from HDCP receiver. */
/* Side-Effect: N/A */
/* //////////////////////////////////////////////////////////////////// */

SYS_STATUS hdmitx_hdcp_GetBKSV(unsigned char *pBKSV)
{
	unsigned char ucdata;
	unsigned char TimeOut;

	Switch_HDMITX_Bank(0);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,
			     B_TX_MASTERDDC | B_TX_MASTERHOST);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_HEADER, DDC_HDCP_ADDRESS);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_REQOFF, 0x00); /* BKSV offset */
	HDMITX_WriteI2C_Byte(REG_TX_DDC_REQCOUNT, 5);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD, CMD_DDC_SEQ_BURSTREAD);

	for (TimeOut = 200; TimeOut > 0; TimeOut--) {
		delay1ms(1);

		ucdata = HDMITX_ReadI2C_Byte(REG_TX_DDC_STATUS);
		if (ucdata & B_TX_DDC_DONE) {
			HDCP_DEBUG_PRINTF(
				"hdmitx_hdcp_GetBKSV(): DDC Done.\n");
			break;
		}
		if (ucdata & B_TX_DDC_ERROR) {
			HDCP_DEBUG_PRINTF(
				"hdmitx_hdcp_GetBKSV(): DDC No ack or arbilose,%x,maybe cable did not connected. Fail.\n",
				ucdata);
			return ER_FAIL;
		}
	}
	if (TimeOut == 0)
		return ER_FAIL;
	HDMITX_ReadI2C_ByteN(REG_TX_BKSV, (unsigned char *)pBKSV, 5);

	return ER_SUCCESS;
}

/* //////////////////////////////////////////////////////////////////// */
/* Function:hdmitx_hdcp_Authenticate */
/* Parameter: N/A */
/* Return: ER_SUCCESS if Authenticated without error. */
/* Remark: do Authentication with Rx */
/* Side-Effect: */
/* 1. hdmiTxDev[0].bAuthenticated global variable will be TRUE when
 * authenticated.
 */
/* 2. Auth_done interrupt and AUTH_FAIL interrupt will be enabled. */
/* //////////////////////////////////////////////////////////////////// */
static unsigned char countbit(unsigned char b)
{
	unsigned char i, count;

	for (i = 0, count = 0; i < 8; i++) {
		if (b & (1 << i))
			count++;
	}
	return count;
}

void hdmitx_hdcp_Reset(void)
{
	unsigned char uc;

	Switch_HDMITX_Bank(0);
	uc = HDMITX_ReadI2C_Byte(REG_TX_SW_RST) | B_TX_HDCP_RST_HDMITX;
	HDMITX_WriteI2C_Byte(REG_TX_SW_RST, uc);
	HDMITX_WriteI2C_Byte(REG_TX_HDCP_DESIRE, 0);
	HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL, 0);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL, B_TX_MASTERHOST);
	hdmitx_ClearDDCFIFO();
	hdmitx_AbortDDC();
}

SYS_STATUS hdmitx_hdcp_Authenticate(void)
{
	unsigned char ucdata;
	unsigned char BCaps;
	unsigned short BStatus;
	unsigned short TimeOut;

	/* unsigned char revoked = FALSE ; */
	unsigned char BKSV[5];

	hdmiTxDev[0].bAuthenticated = FALSE;
	if (0 == (B_TXVIDSTABLE & HDMITX_ReadI2C_Byte(REG_TX_SYS_STATUS)))
		return ER_FAIL;

	/* Authenticate should be called after AFE setup up. */

	HDCP_DEBUG_PRINTF("%s():\n", __func__);
	hdmitx_hdcp_Reset();

	Switch_HDMITX_Bank(0);

	for (TimeOut = 0; TimeOut < 80; TimeOut++) {
		delay1ms(15);

		if (hdmitx_hdcp_GetBCaps(&BCaps, &BStatus) != ER_SUCCESS) {
			HDCP_DEBUG_PRINTF("hdmitx_hdcp_GetBCaps fail.\n");
			return ER_FAIL;
		}
		/* HDCP_DEBUG_PRINTF(("(%d)Reg16 =
		 * %02X\n",idx++,(int)HDMITX_ReadI2C_Byte(0x16)));
		 */

		if (B_TX_HDMI_MODE ==
		    (HDMITX_ReadI2C_Byte(REG_TX_HDMI_MODE) & B_TX_HDMI_MODE)) {
			if ((BStatus & B_TX_CAP_HDMI_MODE) ==
			    B_TX_CAP_HDMI_MODE)
				break;
		} else {
			if ((BStatus & B_TX_CAP_HDMI_MODE) !=
			    B_TX_CAP_HDMI_MODE)
				break;
		}
	}
	/*   if((BStatus & M_TX_DOWNSTREAM_COUNT)> 6)*/
	/*   {*/
	/*   HDCP_DEBUG_PRINTF(("Down Stream Count %d is over maximum supported
	 * number 6,fail.\n",
	 */
	/*	(int)(BStatus & M_TX_DOWNSTREAM_COUNT)));*/
	/*   return ER_FAIL ;*/
	/*   }*/

	HDCP_DEBUG_PRINTF("BCAPS = %02X BSTATUS = %04X\n", (int)BCaps, BStatus);
	hdmitx_hdcp_GetBKSV(BKSV);
	HDCP_DEBUG_PRINTF("BKSV %02X %02X %02X %02X %02X\n", (int)BKSV[0],
			  (int)BKSV[1], (int)BKSV[2], (int)BKSV[3],
			  (int)BKSV[4]);

	for (TimeOut = 0, ucdata = 0; TimeOut < 5; TimeOut++)
		ucdata += countbit(BKSV[TimeOut]);

	if (ucdata != 20) {
		HDCP_DEBUG_PRINTF("countbit error\n");
		return ER_FAIL;
	}
	Switch_HDMITX_Bank(0);
	/* switch bank action should start on direct register writing of each
	 * function.
	 */

	HDMITX_AndReg_Byte(REG_TX_SW_RST, ~(B_TX_HDCP_RST_HDMITX));

	HDMITX_WriteI2C_Byte(REG_TX_HDCP_DESIRE, B_TX_CPDESIRE);
	hdmitx_hdcp_ClearAuthInterrupt();

	hdmitx_hdcp_GenerateAn();
	HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL, 0);
	hdmiTxDev[0].bAuthenticated = FALSE;

	hdmitx_ClearDDCFIFO();

	if ((BCaps & B_TX_CAP_HDMI_REPEATER) == 0) {
		hdmitx_hdcp_Auth_Fire();
		/* wait for status ; */

		for (TimeOut = 250; TimeOut > 0; TimeOut--) {
			delay1ms(5); /* delay 1ms */
			ucdata = HDMITX_ReadI2C_Byte(REG_TX_AUTH_STAT);
			/* HDCP_DEBUG_PRINTF("reg46 = %02x reg16 = %02x\n",*/
			/* (int)ucdata,(int)HDMITX_ReadI2C_Byte(0x16)); */

			if (ucdata & B_TX_AUTH_DONE) {
				hdmiTxDev[0].bAuthenticated = TRUE;
				break;
			}
			ucdata = HDMITX_ReadI2C_Byte(REG_TX_INT_STAT2);
			if (ucdata & B_TX_INT_AUTH_FAIL) {

				HDMITX_WriteI2C_Byte(REG_TX_INT_CLR0,
						     B_TX_CLR_AUTH_FAIL);
				HDMITX_WriteI2C_Byte(REG_TX_INT_CLR1, 0);
				HDMITX_WriteI2C_Byte(REG_TX_SYS_STATUS,
						     B_TX_INTACTDONE);
				HDMITX_WriteI2C_Byte(REG_TX_SYS_STATUS, 0);

				HDCP_DEBUG_PRINTF(
					"%s()-receiver: Authenticate fail\n",
					__func__);
				hdmiTxDev[0].bAuthenticated = FALSE;
				return ER_FAIL;
			}
		}
		if (TimeOut == 0) {
			HDCP_DEBUG_PRINTF(
				"%s()-receiver: Time out. return fail\n",
				__func__);
			hdmiTxDev[0].bAuthenticated = FALSE;
			return ER_FAIL;
		}
		return ER_SUCCESS;
	}
	return hdmitx_hdcp_Authenticate_Repeater();
}

/* //////////////////////////////////////////////////////////////////// */
/* Function: hdmitx_hdcp_VerifyIntegration */
/* Parameter: N/A */
/* Return: ER_SUCCESS if success,if AUTH_FAIL interrupt status,return fail. */
/* Remark: no used now. */
/* Side-Effect: */
/* //////////////////////////////////////////////////////////////////// */

SYS_STATUS hdmitx_hdcp_VerifyIntegration(void)
{
	/* if any interrupt issued a Auth fail,returned the Verify Integration
	 * fail.
	 */

	if (HDMITX_ReadI2C_Byte(REG_TX_INT_STAT1) & B_TX_INT_AUTH_FAIL) {
		hdmitx_hdcp_ClearAuthInterrupt();
		hdmiTxDev[0].bAuthenticated = FALSE;
		return ER_FAIL;
	}
	if (hdmiTxDev[0].bAuthenticated == TRUE)
		return ER_SUCCESS;

	return ER_FAIL;
}

/* //////////////////////////////////////////////////////////////////// */
/* Function: hdmitx_hdcp_Authenticate_Repeater */
/* Parameter: BCaps and BStatus */
/* Return: ER_SUCCESS if success,if AUTH_FAIL interrupt status,return fail. */
/* Remark: */
/* Side-Effect: as Authentication */
/* //////////////////////////////////////////////////////////////////// */

void hdmitx_hdcp_CancelRepeaterAuthenticate(void)
{
	HDCP_DEBUG_PRINTF("hdmitx_hdcp_CancelRepeaterAuthenticate");
	HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,
			     B_TX_MASTERDDC | B_TX_MASTERHOST);
	hdmitx_AbortDDC();
	HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL, B_TX_LISTFAIL | B_TX_LISTDONE);
	hdmitx_hdcp_ClearAuthInterrupt();
}

void hdmitx_hdcp_ResumeRepeaterAuthenticate(void)
{
	HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL, B_TX_LISTDONE);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL, B_TX_MASTERHDCP);
}

#if 0  /* def SUPPORT_SHA */
/* #define SHA_BUFF_COUNT 17 */
/* ULONG w[SHA_BUFF_COUNT]; */
/*  */
/* ULONG sha[5] ; */
/*  */
/* #define rol(x,y) (((x) << (y)) | (((ULONG)x) >> (32-y))) */
/*  */
/* void SHATransform(ULONG * h) */
/* { */
/* int t,i; */
/* ULONG tmp ; */
/*  */
/* h[0] = 0x67452301 ; */
/* h[1] = 0xefcdab89; */
/* h[2] = 0x98badcfe; */
/* h[3] = 0x10325476; */
/* h[4] = 0xc3d2e1f0; */
/* for( t = 0 ; t < 80 ; t++ ) */
/* { */
/* if((t>=16)&&(t<80)) { */
/* i=(t+SHA_BUFF_COUNT-3)%SHA_BUFF_COUNT; */
/* tmp = w[i]; */
/* i=(t+SHA_BUFF_COUNT-8)%SHA_BUFF_COUNT; */
/* tmp ^= w[i]; */
/* i=(t+SHA_BUFF_COUNT-14)%SHA_BUFF_COUNT; */
/* tmp ^= w[i]; */
/* i=(t+SHA_BUFF_COUNT-16)%SHA_BUFF_COUNT; */
/* tmp ^= w[i]; */
/* w[t%SHA_BUFF_COUNT] = rol(tmp,1); */
/* //HDCP_DEBUG_PRINTF(("w[%2d] = %08lX\n",t,w[t%SHA_BUFF_COUNT])); */
/* } */
/*  */
/* if((t>=0)&&(t<20)) { */
/* tmp = rol(h[0],5) + ((h[1] & h[2]) | (h[3] & ~h[1])) + h[4] +
 * w[t%SHA_BUFF_COUNT] + 0x5a827999;
 */
/* HDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",
 * h[0],h[1],h[2],h[3],h[4]));
 */
/* h[4] = h[3]; */
/* h[3] = h[2]; */
/* h[2] = rol(h[1],30); */
/* h[1] = h[0]; */
/* h[0] = tmp; */
/*  */
/* } */
/* if((t>=20)&&(t<40)) { */
/* tmp = rol(h[0],5) + (h[1] ^ h[2] ^ h[3]) + h[4] +
 * w[t%SHA_BUFF_COUNT] + 0x6ed9eba1;
 */
/* HDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",
 * h[0],h[1],h[2],h[3],h[4]));
 */
/* h[4] = h[3]; */
/* h[3] = h[2]; */
/* h[2] = rol(h[1],30); */
/* h[1] = h[0]; */
/* h[0] = tmp; */
/* } */
/* if((t>=40)&&(t<60)) { */
/* tmp = rol(h[0], 5) + ((h[1] & h[2]) |
 * (h[1] & h[3]) | (h[2] & h[3])) + h[4] +
 * w[t%SHA_BUFF_COUNT] + 0x8f1bbcdc;
 */
/* //HDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",
 * h[0],h[1],h[2],h[3],h[4]));
 */
/* h[4] = h[3]; */
/* h[3] = h[2]; */
/* h[2] = rol(h[1],30); */
/* h[1] = h[0]; */
/* h[0] = tmp; */
/* } */
/* if((t>=60)&&(t<80)) { */
/* tmp = rol(h[0],5) + (h[1] ^ h[2] ^ h[3]) + h[4] +
 * w[t%SHA_BUFF_COUNT] + 0xca62c1d6;
 */
/* HDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",
 * h[0],h[1],h[2],h[3],h[4]));
 */
/* h[4] = h[3]; */
/* h[3] = h[2]; */
/* h[2] = rol(h[1],30); */
/* h[1] = h[0]; */
/* h[0] = tmp; */
/* } */
/* } */
/* HDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",
 * h[0],h[1],h[2],h[3],h[4]));
 */

/* h[0] += 0x67452301 ; */
/* h[1] += 0xefcdab89; */
/* h[2] += 0x98badcfe; */
/* h[3] += 0x10325476; */
/* h[4] += 0xc3d2e1f0; */
/* //    HDCP_DEBUG_PRINTF(("%08lX %08lX %08lX %08lX %08lX\n",
 * h[0],h[1],h[2],h[3],h[4]));
 */
/* } */
/*  */
/*  ---------------------------------------------------------------------- */
/* * Outer SHA algorithm: take an arbitrary length byte string, */
/* * convert it into 16-word blocks with the prescribed padding at */
/* * the end,and pass those blocks to the core SHA algorithm. */
/* */ */
/*  */
/* void SHA_Simple(void *p,LONG len,unsigned char *output) */
/* { */
/* // SHA_State s; */
/* int i, t ; */
/* ULONG c ; */
/* char *pBuff = p ; */
/*  */
/* for(i=0;i < len;i+=4) */
/* { */
/*  */
/* t=i/4; */
/* w[t]=0; */
/* *((char *)&c)= pBuff[i]; */
/* *((char *)&c+1)= pBuff[i+1]; */
/* *((char *)&c+2)= pBuff[i+2]; */
/* *((char *)&c+3)= pBuff[i+3]; */
/* w[t]=c; */
/* } */
/*  */
/* c=0x80; */
/* c<<=((3-len%4)*8); */
/* w[t] |= c; */
/*  */
/*  */
/* for( i = 0 ; i < len ; i++ ) */
/* { */
/* t = i/4 ; */
/* if( i%4 == 0 ) */
/* { */
/* w[t] = 0 ; */
/* } */
/* c = pBuff[i] ; */
/* c &= 0xFF ; */
/* c <<= (3-(i%4))*8 ; */
/* w[t] |= c ; */
/* //        HDCP_DEBUG_PRINTF(("pBuff[%d] = %02x, c = %08lX,
 * w[%d] = %08lX\n",i,pBuff[i],c,t,w[t]));
 */
/* } */
/*  */
/* t = i/4 ; */
/* if( i%4 == 0 ) */
/* { */
/* w[t] = 0 ; */
/* } */
/* c = 0x80; */
/* c <<= ((3-i%4)*8); */
/* w[t]|= c ; */
/* */ */
/* t++ ; */
/* for( ; t < 15 ; t++ ) */
/* { */
/* w[t] = 0 ; */
/* } */
/* w[15] = len*8  ; */
/*  */
/* for( t = 0 ; t< 16 ; t++ ) */
/* { */
/* HDCP_DEBUG_PRINTF(("w[%2d] = %08lX\n",t,w[t])); */
/* } */
/*  */
/* SHATransform(sha); */
/*  */
/* for( i = 0 ; i < 5 ; i++ ) */
/* { */
/* output[i*4] = (unsigned char)(sha[i]&0xFF); */
/* output[i*4+1] = (unsigned char)((sha[i]>>8)&0xFF); */
/* output[i*4+2] = (unsigned char)((sha[i]>>16)&0xFF); */
/* output[i*4+3]   = (unsigned char)((sha[i]>>24)&0xFF); */
/* } */
/* } */
#endif /* 0 */

#ifdef SUPPORT_SHA

SYS_STATUS hdmitx_hdcp_CheckSHA(unsigned char pM0[], unsigned short BStatus,
				unsigned char pKSVList[], int cDownStream,
				unsigned char Vr[])
{
	int i, n;

	for (i = 0; i < cDownStream * 5; i++)
		SHABuff[i] = pKSVList[i];

	SHABuff[i++] = BStatus & 0xFF;
	SHABuff[i++] = (BStatus >> 8) & 0xFF;
	for (n = 0; n < 8; n++, i++)
		SHABuff[i] = pM0[n];

	n = i;
	/* SHABuff[i++] = 0x80 ; // end mask */
	for (; i < 64; i++)
		SHABuff[i] = 0;

	/* n = cDownStream * 5 + 2  + 8  ; */
	/* n *= 8 ; */
	/* SHABuff[62] = (n>>8) & 0xff ; */
	/* SHABuff[63] = (n>>8) & 0xff ; */

	/*  for(i = 0 ; i < 64 ; i++)*/
	/*  {*/
	/*	if(i % 16 == 0)*/
	/*	{*/
	/*	    HDCP_DEBUG_PRINTF(("SHA[]: "));*/
	/*	}*/
	/*	HDCP_DEBUG_PRINTF((" %02X",SHABuff[i]));*/
	/*	if((i%16)==15)*/
	/*	{*/
	/*	    HDCP_DEBUG_PRINTF(("\n"));*/
	/*	}*/
	/*    }*/

	SHA_Simple(SHABuff, n, V);
	for (i = 0; i < 20; i++) {
		if (V[i] != Vr[i]) {
			/*
			HDCP_DEBUG_PRINTF("V[] =");
			for (i = 0; i < 20; i++)
				HDCP_DEBUG_PRINTF(" %02X", (int)V[i]);

			HDCP_DEBUG_PRINTF("\nVr[] =");
			for (i = 0; i < 20; i++)
				HDCP_DEBUG_PRINTF(" %02X", (int)Vr[i]);
			*/
			HDCP_DEBUG_PRINTF(
				"V ={0x%02x,0x%02x,0x%02x,0x%02x,\n",
				V[0], V[1], V[2], V[3]);
			HDCP_DEBUG_PRINTF(
				"    0x%02x,0x%02x,0x%02x,0x%02x,\n",
				V[4], V[5], V[6], V[7]);
			HDCP_DEBUG_PRINTF(
				"    0x%02x,0x%02x,0x%02x,0x%02x,\n",
				V[8], V[9], V[10], V[11]);
			HDCP_DEBUG_PRINTF(
				"    0x%02x,0x%02x,0x%02x,0x%02x,\n",
				V[12], V[13], V[14], V[15]);
			HDCP_DEBUG_PRINTF(
				"    0x%02x,0x%02x,0x%02x,0x%02x}\n",
				V[16], V[17], V[18], V[19]);

			return ER_FAIL;
		}
	}
	return ER_SUCCESS;
}

#endif /* SUPPORT_SHA */

SYS_STATUS hdmitx_hdcp_GetKSVList(unsigned char *pKSVList,
				  unsigned char cDownStream)
{
	unsigned char TimeOut = 100;
	unsigned char ucdata;

	if (cDownStream == 0)
		return ER_SUCCESS;

	if (/* cDownStream == 0 || */ pKSVList == NULL)
		return ER_FAIL;

	HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL, B_TX_MASTERHOST);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_HEADER, 0x74);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_REQOFF, 0x43);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_REQCOUNT, cDownStream * 5);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD, CMD_DDC_SEQ_BURSTREAD);

	for (TimeOut = 200; TimeOut > 0; TimeOut--) {

		ucdata = HDMITX_ReadI2C_Byte(REG_TX_DDC_STATUS);
		if (ucdata & B_TX_DDC_DONE) {
			HDCP_DEBUG_PRINTF(
				"%s(): DDC Done.\n", __func__);
			break;
		}
		if (ucdata & B_TX_DDC_ERROR) {
			HDCP_DEBUG_PRINTF(
				"%s(): DDC Fail by REG_TX_DDC_STATUS = %x.\n",
				__func__, ucdata);
			return ER_FAIL;
		}
		delay1ms(5);
	}
	if (TimeOut == 0)
		return ER_FAIL;

//	HDCP_DEBUG_PRINTF("%s(): KSV", __func__);
	for (TimeOut = 0; TimeOut < cDownStream * 5; TimeOut++) {
		pKSVList[TimeOut] = HDMITX_ReadI2C_Byte(REG_TX_DDC_READFIFO);
//		HDCP_DEBUG_PRINTF(" %02X", (int)pKSVList[TimeOut]);
	}
//	HDCP_DEBUG_PRINTF("\n");

	for (TimeOut = 0; TimeOut < cDownStream; TimeOut++) {
		HDCP_DEBUG_PRINTF(
			"KSVList[%d]={0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}\n",
			TimeOut,
			pKSVList[TimeOut * 5 + 0],
			pKSVList[TimeOut * 5 + 1],
			pKSVList[TimeOut * 5 + 2],
			pKSVList[TimeOut * 5 + 3],
			pKSVList[TimeOut * 5 + 4]);
	}
	return ER_SUCCESS;
}

SYS_STATUS hdmitx_hdcp_GetVr(unsigned char *pVr)
{
	unsigned char TimeOut;
	unsigned char ucdata;

	if (pVr == NULL)
		return ER_FAIL;

	HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL, B_TX_MASTERHOST);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_HEADER, 0x74);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_REQOFF, 0x20);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_REQCOUNT, 20);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD, CMD_DDC_SEQ_BURSTREAD);

	for (TimeOut = 200; TimeOut > 0; TimeOut--) {
		ucdata = HDMITX_ReadI2C_Byte(REG_TX_DDC_STATUS);
		if (ucdata & B_TX_DDC_DONE) {
			HDCP_DEBUG_PRINTF("%s(): DDC Done.\n", __func__);
			break;
		}
		if (ucdata & B_TX_DDC_ERROR) {
			HDCP_DEBUG_PRINTF(
				"%s(): DDC fail by REG_TX_DDC_STATUS = %x.\n",
				__func__, (int)ucdata);
			return ER_FAIL;
		}
		delay1ms(5);
	}
	if (TimeOut == 0) {
		HDCP_DEBUG_PRINTF(
			"%s(): DDC fail by timeout.\n", __func__);
		return ER_FAIL;
	}
	Switch_HDMITX_Bank(0);

	for (TimeOut = 0; TimeOut < 5; TimeOut++) {
		HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL, TimeOut);
		pVr[TimeOut * 4] =
			(ULONG)HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE1);
		pVr[TimeOut * 4 + 1] =
			(ULONG)HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE2);
		pVr[TimeOut * 4 + 2] =
			(ULONG)HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE3);
		pVr[TimeOut * 4 + 3] =
			(ULONG)HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE4);
		/* HDCP_DEBUG_PRINTF(("V' = %02X %02X %02X %02X\n",*/
		/* (int)pVr[TimeOut*4],(int)pVr[TimeOut*4+1],
		 * (int)pVr[TimeOut*4+2],(int)pVr[TimeOut*4+3]));
		 */
	}

	HDCP_DEBUG_PRINTF(
		"V'={0x%02x,0x%02x,0x%02x,0x%02x,\n",
		pVr[0], pVr[1], pVr[2], pVr[3]);
	HDCP_DEBUG_PRINTF(
		"    0x%02x,0x%02x,0x%02x,0x%02x,\n",
		pVr[4], pVr[5], pVr[6], pVr[7]);
	HDCP_DEBUG_PRINTF(
		"    0x%02x,0x%02x,0x%02x,0x%02x,\n",
		pVr[8], pVr[9], pVr[10], pVr[11]);
	HDCP_DEBUG_PRINTF(
		"    0x%02x,0x%02x,0x%02x,0x%02x,\n",
		pVr[12], pVr[13], pVr[14], pVr[15]);
	HDCP_DEBUG_PRINTF(
		"    0x%02x,0x%02x,0x%02x,0x%02x}\n",
		pVr[16], pVr[17], pVr[18], pVr[19]);

		return ER_SUCCESS;
}

SYS_STATUS hdmitx_hdcp_GetM0(unsigned char *pM0)
{
//	int i;

	if (!pM0)
		return ER_FAIL;

	HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL,
			     5); /* read m0[31:0] from reg51~reg54 */
	pM0[0] = HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE1);
	pM0[1] = HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE2);
	pM0[2] = HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE3);
	pM0[3] = HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE4);
	HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL, 0); /* read m0[39:32] from reg55 */
	pM0[4] = HDMITX_ReadI2C_Byte(REG_TX_AKSV_RD_BYTE5);
	HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL, 1); /* read m0[47:40] from reg55 */
	pM0[5] = HDMITX_ReadI2C_Byte(REG_TX_AKSV_RD_BYTE5);
	HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL, 2); /* read m0[55:48] from reg55 */
	pM0[6] = HDMITX_ReadI2C_Byte(REG_TX_AKSV_RD_BYTE5);
	HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL, 3); /* read m0[63:56] from reg55 */
	pM0[7] = HDMITX_ReadI2C_Byte(REG_TX_AKSV_RD_BYTE5);

	/*
	HDCP_DEBUG_PRINTF("M[] =");
	for (i = 0; i < 8; i++)
		HDCP_DEBUG_PRINTF("0x%02x,", (int)pM0[i]);

	HDCP_DEBUG_PRINTF("\n");
	*/
	HDCP_DEBUG_PRINTF(
		"M0={0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}\n",
		pM0[0], pM0[1], pM0[2], pM0[3], pM0[4], pM0[5], pM0[6], pM0[7]);

	return ER_SUCCESS;
}

SYS_STATUS hdmitx_hdcp_Authenticate_Repeater(void)
{
	unsigned char uc, ii;
	/* unsigned char revoked ; */
	/* int i ; */
	unsigned char cDownStream;

	unsigned char BCaps;
	unsigned short BStatus;
	unsigned short TimeOut;

	HDCP_DEBUG_PRINTF("Authentication for repeater\n");
	/* emily add for test,abort HDCP */
	/* 2007/10/01 marked by jj_tseng@chipadvanced.com */
	/* HDMITX_WriteI2C_Byte(0x20,0x00); */
	/* HDMITX_WriteI2C_Byte(0x04,0x01); */
	/* HDMITX_WriteI2C_Byte(0x10,0x01); */
	/* HDMITX_WriteI2C_Byte(0x15,0x0F); */
	/* delay1ms(100); */
	/* HDMITX_WriteI2C_Byte(0x04,0x00); */
	/* HDMITX_WriteI2C_Byte(0x10,0x00); */
	/* HDMITX_WriteI2C_Byte(0x20,0x01); */
	/* delay1ms(100); */
	/* test07 = HDMITX_ReadI2C_Byte(0x7); */
	/* test06 = HDMITX_ReadI2C_Byte(0x6); */
	/* test08 = HDMITX_ReadI2C_Byte(0x8); */
	/* ~jj_tseng@chipadvanced.com */
	/* end emily add for test */
	/* //////////////////////////////////// */
	/* Authenticate Fired */
	/* //////////////////////////////////// */

	hdmitx_hdcp_GetBCaps(&BCaps, &BStatus);
	delay1ms(2);
	if ((B_TX_INT_HPD_PLUG | B_TX_INT_RX_SENSE) &
	    HDMITX_ReadI2C_Byte(REG_TX_INT_STAT1)) {
		HDCP_DEBUG_PRINTF("HPD Before Fire Auth\n");
		goto hdmitx_hdcp_Repeater_Fail;
	}
	hdmitx_hdcp_Auth_Fire();
	/* delay1ms(550); // emily add for test */
	for (ii = 0; ii < 55; ii++) {
		/* delay1ms(550); // emily add for test */
		if ((B_TX_INT_HPD_PLUG | B_TX_INT_RX_SENSE) &
		    HDMITX_ReadI2C_Byte(REG_TX_INT_STAT1))
			goto hdmitx_hdcp_Repeater_Fail;

		delay1ms(10);
	}
	for (TimeOut = /*250*6 */ 10; TimeOut > 0; TimeOut--) {
		HDCP_DEBUG_PRINTF("TimeOut = %d wait part 1\n", TimeOut);
		if ((B_TX_INT_HPD_PLUG | B_TX_INT_RX_SENSE) &
		    HDMITX_ReadI2C_Byte(REG_TX_INT_STAT1)) {
			HDCP_DEBUG_PRINTF("HPD at wait part 1\n");
			goto hdmitx_hdcp_Repeater_Fail;
		}
		uc = HDMITX_ReadI2C_Byte(REG_TX_INT_STAT1);
		if (uc & B_TX_INT_DDC_BUS_HANG) {
			HDCP_DEBUG_PRINTF("DDC Bus hang\n");
			goto hdmitx_hdcp_Repeater_Fail;
		}
		uc = HDMITX_ReadI2C_Byte(REG_TX_INT_STAT2);

		if (uc & B_TX_INT_AUTH_FAIL) {
			/* HDMITX_WriteI2C_Byte(REG_TX_INT_CLR0,
			 * B_TX_CLR_AUTH_FAIL);
			 */
			/* HDMITX_WriteI2C_Byte(REG_TX_INT_CLR1,0);*/
			/* HDMITX_WriteI2C_Byte(REG_TX_SYS_STATUS,
			 * B_TX_INTACTDONE);
			 */
			/* HDMITX_WriteI2C_Byte(REG_TX_SYS_STATUS,0);*/

			HDCP_DEBUG_PRINTF(
				"%s(): B_TX_INT_AUTH_FAIL.\n", __func__);
			goto hdmitx_hdcp_Repeater_Fail;
		}
		/* emily add for test */
		/* test =(HDMITX_ReadI2C_Byte(0x7)&0x4)>>2 ; */
		if (uc & B_TX_INT_KSVLIST_CHK) {
			HDMITX_WriteI2C_Byte(REG_TX_INT_CLR0,
				B_TX_CLR_KSVLISTCHK);
			HDMITX_WriteI2C_Byte(REG_TX_INT_CLR1, 0);
			HDMITX_WriteI2C_Byte(REG_TX_SYS_STATUS,
				B_TX_INTACTDONE);
			HDMITX_WriteI2C_Byte(REG_TX_SYS_STATUS, 0);
			HDCP_DEBUG_PRINTF("B_TX_INT_KSVLIST_CHK\n");
			break;
		}
		delay1ms(5);
	}
	if (TimeOut == 0) {
		HDCP_DEBUG_PRINTF(
			"Time out for wait KSV List checking interrupt\n");
		goto hdmitx_hdcp_Repeater_Fail;
	}
	/* ///////////////////////////////////// */
	/* clear KSVList check interrupt. */
	/* ///////////////////////////////////// */

	for (TimeOut = 500; TimeOut > 0; TimeOut--) {
		HDCP_DEBUG_PRINTF("TimeOut=%d at wait FIFO ready\n", TimeOut);
		if ((B_TX_INT_HPD_PLUG | B_TX_INT_RX_SENSE) &
		    HDMITX_ReadI2C_Byte(REG_TX_INT_STAT1)) {
			HDCP_DEBUG_PRINTF("HPD at wait FIFO ready\n");
			goto hdmitx_hdcp_Repeater_Fail;
		}
		if (hdmitx_hdcp_GetBCaps(&BCaps, &BStatus) == ER_FAIL) {
			HDCP_DEBUG_PRINTF("Get BCaps fail\n");
			goto hdmitx_hdcp_Repeater_Fail;
		}
		if (BCaps & B_TX_CAP_KSV_FIFO_RDY) {
			HDCP_DEBUG_PRINTF("FIFO Ready\n");
			break;
		}
		delay1ms(5);
	}
	if (TimeOut == 0) {
		HDCP_DEBUG_PRINTF("Get KSV FIFO ready TimeOut\n");
		goto hdmitx_hdcp_Repeater_Fail;
	}
	HDCP_DEBUG_PRINTF("Wait timeout = %d\n", TimeOut);

	hdmitx_ClearDDCFIFO();
	hdmitx_GenerateDDCSCLK();
	cDownStream = (BStatus & M_TX_DOWNSTREAM_COUNT);

	if (/*cDownStream == 0 || */ cDownStream > 6 ||
	    BStatus & (B_TX_MAX_CASCADE_EXCEEDED | B_TX_DOWNSTREAM_OVER)) {
		HDCP_DEBUG_PRINTF("Invalid Down stream count,fail\n");
		goto hdmitx_hdcp_Repeater_Fail;
	}
#ifdef SUPPORT_SHA
	if (hdmitx_hdcp_GetKSVList(KSVList, cDownStream) == ER_FAIL)
		goto hdmitx_hdcp_Repeater_Fail;

#if 0
	for (i = 0; i < cDownStream; i++) {
		revoked = FALSE;
		uc = 0;
		for (TimeOut = 0; TimeOut < 5; TimeOut++) {
			/* check bit count */
			uc += countbit(KSVList[i * 5 + TimeOut]);
		}
		if (uc != 20)
			revoked = TRUE;

		if (revoked) {
/* HDCP_DEBUG_PRINTF(("KSVFIFO[%d] = %02X %02X %02X %02X %02X is revoked\n",*/
/*i,(int)KSVList[i*5],(int)KSVList[i*5+1],(int)KSVList[i*5+2],*/
/*(int)KSVList[i*5+3],(int)KSVList[i*5+4])); */
			goto hdmitx_hdcp_Repeater_Fail;
		}
	}
#endif

	if (hdmitx_hdcp_GetVr(Vr) == ER_FAIL)
		goto hdmitx_hdcp_Repeater_Fail;

	if (hdmitx_hdcp_GetM0(M0) == ER_FAIL)
		goto hdmitx_hdcp_Repeater_Fail;

	/* do check SHA */
	if (hdmitx_hdcp_CheckSHA(M0, BStatus, KSVList, cDownStream, Vr) ==
	    ER_FAIL)
		goto hdmitx_hdcp_Repeater_Fail;

	if ((B_TX_INT_HPD_PLUG | B_TX_INT_RX_SENSE) &
	    HDMITX_ReadI2C_Byte(REG_TX_INT_STAT1)) {
		HDCP_DEBUG_PRINTF("HPD at Final\n");
		goto hdmitx_hdcp_Repeater_Fail;
	}
#endif /* SUPPORT_SHA */

	hdmitx_hdcp_ResumeRepeaterAuthenticate();
	hdmiTxDev[0].bAuthenticated = TRUE;
	return ER_SUCCESS;

hdmitx_hdcp_Repeater_Fail:
	hdmitx_hdcp_CancelRepeaterAuthenticate();
	return ER_FAIL;
}

/* //////////////////////////////////////////////////////////////////// */
/* Function: hdmitx_hdcp_ResumeAuthentication */
/* Parameter: N/A */
/* Return: N/A */
/* Remark: called by interrupt handler to restart Authentication and Encryption.
 */
/* Side-Effect: as Authentication and Encryption. */
/* //////////////////////////////////////////////////////////////////// */

void hdmitx_hdcp_ResumeAuthentication(void)
{
	setHDMITX_AVMute(TRUE);
	if (hdmitx_hdcp_Authenticate() == ER_SUCCESS)
		;

	setHDMITX_AVMute(FALSE);
}

#endif /* SUPPORT_HDCP */

////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) SANYO Techno Solutions Tottori Co., Ltd.
//
////////////////////////////////////////////////////////////////////////////////

#ifdef SUPPORT_HDCP_STS

typedef enum {
	HDCP_STATE_A0,
	HDCP_STATE_A1_A2_A3,
	HDCP_STATE_A4,
	HDCP_STATE_A5,
	HDCP_STATE_A6,
	HDCP_STATE_A8,
	HDCP_STATE_A9,
} HDCP_STATE;

static HDCP_STATE hdmitx_hdcp_state = HDCP_STATE_A0;
static unsigned int hdmitx_hdcp_a8_start = 0;
static int hdmitx_hdcp_hdmi_mode = -1;

static int hdmitx_hdcp_auth_state_a0(u8 int_status);
static int hdmitx_hdcp_auth_state_a1_a2_a3(u8 int_status);
static int hdmitx_hdcp_auth_state_a4(u8 int_status);
static int hdmitx_hdcp_auth_state_a6(u8 int_status);
static int hdmitx_hdcp_auth_state_a8(u8 int_status);
static int hdmitx_hdcp_auth_state_a9(u8 int_status);

void hdmitx_hdcp_auth_reset(void)
{
//	pr_info("%s\n", __FUNCTION__);

	if (hdmitx_hdcp_state >= HDCP_STATE_A8) {
		hdmitx_hdcp_CancelRepeaterAuthenticate();
	}
	hdmitx_hdcp_ResetAuth();
	hdmitx_hdcp_ClearAuthInterrupt();
	hdmitx_hdcp_state = HDCP_STATE_A0;
	hdmiTxDev[0].bAuthenticated = FALSE;
	HDCP_DEBUG_PRINTF("HDCP state: A0\n");
}

void hdmitx_hdcp_auth_start(int hdmi_mode)
{
//	pr_info("%s (hdmi_mode=%d)\n", __FUNCTION__, hdmi_mode);

	hdmitx_hdcp_auth_reset();
	hdmitx_hdcp_hdmi_mode = hdmi_mode;
//	if (!hdmi_mode) {
//		hdmitx_hdcp_state = HDCP_STATE_A4;
//	}
	hdmitx_hdcp_auth_continue();
}

void hdmitx_hdcp_auth_continue(void)
{
	int ret;
	u8 int_status;

//	pr_info("%s\n", __FUNCTION__);

	Switch_HDMITX_Bank(0);

	int_status = HDMITX_ReadI2C_Byte(REG_TX_INT_STAT1);
	if (int_status & (B_TX_INT_HPD_PLUG | B_TX_INT_RX_SENSE)) {
		pr_err("HPD or RXSENSE changed (state=%d)\n",
			hdmitx_hdcp_state);
		hdmitx_hdcp_auth_reset();
		return;
	}
	int_status = HDMITX_ReadI2C_Byte(REG_TX_INT_STAT2);
	if (int_status & B_TX_INT_AUTH_FAIL) {
		pr_err("authentication failed (state=%d)\n",
			hdmitx_hdcp_state);
		hdmitx_hdcp_auth_reset();
		return;
	}

	switch (hdmitx_hdcp_state)
	{
	case HDCP_STATE_A0:
		ret = hdmitx_hdcp_auth_state_a0(int_status);
		break;

	case HDCP_STATE_A1_A2_A3:
		ret = hdmitx_hdcp_auth_state_a1_a2_a3(int_status);
		break;

	case HDCP_STATE_A4:
		ret = hdmitx_hdcp_auth_state_a4(int_status);
		break;

	case HDCP_STATE_A6:
		ret = hdmitx_hdcp_auth_state_a6(int_status);
		break;

	case HDCP_STATE_A8:
		ret = hdmitx_hdcp_auth_state_a8(int_status);
		break;

	case HDCP_STATE_A9:
		ret = hdmitx_hdcp_auth_state_a9(int_status);
		break;

	default:
		ret = -1;
		break;
	}

	if (ret != 0) {
		pr_err("authentication failed (state=%d)\n", hdmitx_hdcp_state);
		hdmitx_hdcp_auth_reset();
	}
}

//##############################################################################
// A0 - Wait for active RX
//##############################################################################
static int hdmitx_hdcp_auth_state_a0(u8 int_status)
{
	u8 sys_status;
	u8 bcaps;
	u16 bstatus;
	u8 bksv[5];
	u8 cnt;
	u8 value;
	int i;
	int timeout;

	Switch_HDMITX_Bank(0);
	sys_status = HDMITX_ReadI2C_Byte(REG_TX_SYS_STATUS);
	if (!(sys_status & B_TX_HPDETECT)) {
		return -1;
	}
	if (!(sys_status & B_TXVIDSTABLE)) {
		return -1;
	}

	hdmitx_hdcp_Reset();

	setHDMITX_AVMute(TRUE);

	for (timeout = 80; timeout > 0; timeout--) {
		delay1ms(15);
		if (ER_SUCCESS != hdmitx_hdcp_GetBCaps(&bcaps, &bstatus)) {
			pr_err("failed to get BCaps and BStatus @ A0\n");
			return -1;
		}
		if ((bstatus & B_TX_CAP_HDMI_MODE) == B_TX_CAP_HDMI_MODE) {
			if (hdmitx_hdcp_hdmi_mode == 1) {
				break;
			}
		}
		else {
			if (hdmitx_hdcp_hdmi_mode == 0) {
				break;
			}
		}
	}
	HDCP_DEBUG_PRINTF("BCaps=0x%02x, BStatus=0x%04x\n", bcaps, bstatus);

	if (ER_SUCCESS != hdmitx_hdcp_GetBKSV(bksv)) {
		pr_err("failed to get BKSV\n");
		return -1;
	}
	HDCP_DEBUG_PRINTF("BKSV={0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}\n",
		bksv[0], bksv[1], bksv[2], bksv[3], bksv[4]);
	cnt = 0;
	for (i=0; i<5; i++) {
		cnt += countbit(bksv[i]);
	}
	if (cnt != 20) {
		pr_err("failed to check BKSV\n");
		return -1;
	}

	Switch_HDMITX_Bank(0);
	HDMITX_AndReg_Byte(REG_TX_SW_RST, ~(B_TX_HDCP_RST_HDMITX));

	hdmitx_hdcp_GenerateAn();

	Switch_HDMITX_Bank(0);
	value = B_TX_CPDESIRE;
	if (bcaps & B_CAP_HDCP_1p1) {
		value |= B_TX_ENABLE_HDCP11;
	}
	HDMITX_WriteI2C_Byte(REG_TX_HDCP_DESIRE, value);

	hdmitx_hdcp_ClearAuthInterrupt();

	hdmitx_hdcp_Auth_Fire();

	HDCP_DEBUG_PRINTF("HDCP state: A0 -> A1/A2/A3\n");
	hdmitx_hdcp_state = HDCP_STATE_A1_A2_A3;

	for (timeout = 250; timeout > 0; timeout--) {
		int state = hdmitx_hdcp_state;
		delay1ms(5);
		sys_status = HDMITX_ReadI2C_Byte(REG_TX_SYS_STATUS);
		if (!(sys_status & B_TX_HPDETECT)) {
			pr_err("HPD is low");
			return -1;
		}
		int_status = HDMITX_ReadI2C_Byte(REG_TX_INT_STAT1);
		if (int_status & (B_TX_INT_HPD_PLUG | B_TX_INT_RX_SENSE)) {
			pr_err("HPD or RXSENSE changed (state=%d)\n",
				hdmitx_hdcp_state);
			return -1;
		}
		if (state == HDCP_STATE_A8) {
			if ((timeout % 20) != 0) {
				continue;
			}
		}
		hdmitx_hdcp_auth_continue();
		if (hdmitx_hdcp_state == HDCP_STATE_A0) {
			return -1;
		}
		if (hdmitx_hdcp_state == HDCP_STATE_A4) {
			break;
		}
		if (hdmitx_hdcp_state == HDCP_STATE_A8) {
			if (state != HDCP_STATE_A8) {
				// KSV FIFO ready timeout: 5 sec (+ alpha)
				timeout = 1000 + 20;
			}
		}
	}
	if (timeout <= 0) {
		pr_err("authentication failed (timed out)\n");
		return -1;
	}

	return 0;
}

//##############################################################################
// A1 - Exchange KSVs
// A2 - Computations
// A3 - Validate Receiver
//##############################################################################
static int hdmitx_hdcp_auth_state_a1_a2_a3(u8 int_status)
{
	if (int_status & (B_TX_INT_AUTH_DONE | B_TX_INT_KSVLIST_CHK)) {
		HDCP_DEBUG_PRINTF("HDCP state: A1/A2/A3 -> A6\n");
		hdmitx_hdcp_state = HDCP_STATE_A6;
	}
	return 0;
}

//##############################################################################
// A4 - Authenticated
//##############################################################################
static int hdmitx_hdcp_auth_state_a4(u8 int_status)
{
	if (!hdmiTxDev[0].bAuthenticated) {
		HDCP_DEBUG_PRINTF("HDCP state: A4\n");
		pr_info("HDCP authentication completed.\n");
		hdmiTxDev[0].bAuthenticated = TRUE;
		setHDMITX_AVMute(FALSE);
		if (int_status & B_TX_INT_AUTH_DONE) {
			hdmitx_hdcp_ClearAuthInterrupt();
			HDMITX_OrReg_Byte(REG_TX_INT_MASK2, B_TX_AUTH_DONE_MASK);
		}
	}
	return 0;
}

//##############################################################################
// A6 - Test For Repeater
//##############################################################################
static int hdmitx_hdcp_auth_state_a6(u8 int_status)
{
	if (int_status & B_TX_INT_KSVLIST_CHK) {
		HDCP_DEBUG_PRINTF("HDCP state: A6 -> A8\n");
		hdmitx_hdcp_state = HDCP_STATE_A8;
		hdmitx_hdcp_a8_start = jiffies_to_msecs(jiffies);
		return 0;
	}
	if (int_status & B_TX_INT_AUTH_DONE) {
		HDCP_DEBUG_PRINTF("HDCP state: A6 -> A4\n");
		hdmitx_hdcp_state = HDCP_STATE_A4;
		return 0;
	}
	return 0;
}

//##############################################################################
// A8 - Wait for Ready
//##############################################################################
static int hdmitx_hdcp_auth_state_a8(u8 int_status)
{
	u8 bcaps;
	u16 bstatus;
	unsigned int msec = jiffies_to_msecs(jiffies);

	(void)int_status;

	if (ER_SUCCESS != hdmitx_hdcp_GetBCaps(&bcaps, &bstatus)) {
		pr_err("failed to get BCaps and BStatus @ A8\n");
		return -1;
	}
	if (bcaps & B_TX_CAP_KSV_FIFO_RDY) {
		if (500 < abs((int)(msec - hdmitx_hdcp_a8_start))) {
			HDCP_DEBUG_PRINTF("HDCP state: A8 -> A9\n");
			hdmitx_hdcp_state = HDCP_STATE_A9;
		}
		return 0;
	}
	// KSV FIFO ready timeout: 5 sec (+ alpha)
	if ((5000 + 100) < abs((int)(msec - hdmitx_hdcp_a8_start))) {
		pr_err("timed out @ A8\n");
		return -1;
	}

	return 0;
}

//##############################################################################
// A9 - Read KSV List
//##############################################################################
static int hdmitx_hdcp_auth_state_a9(u8 int_status)
{
	u8 bcaps;
	u16 bstatus;
	u8 count;

	(void)int_status;

	if (ER_SUCCESS != hdmitx_hdcp_GetBCaps(&bcaps, &bstatus)) {
		pr_err("failed to get BCaps and BStatus @ A9\n");
		return -1;
	}
	hdmitx_ClearDDCFIFO();
	hdmitx_GenerateDDCSCLK();

	count = bstatus & M_TX_DOWNSTREAM_COUNT;
	if ((count == 0) || (count > 6) ||
		(bstatus & (B_TX_MAX_CASCADE_EXCEEDED | B_TX_DOWNSTREAM_OVER))) {
		pr_err("BStatus is invalid (0x%04x)\n", bstatus);
		return -1;
	}
	if (ER_FAIL == hdmitx_hdcp_GetKSVList(KSVList, count)) {
		pr_err("failed to get KSV List\n");
		return -1;
	}
	if (ER_FAIL == hdmitx_hdcp_GetM0(M0)) {
		pr_err("failed to get M0\n");
		return -1;
	}
	if (ER_FAIL == hdmitx_hdcp_GetVr(Vr)) {
		pr_err("failed to get Vr\n");
		return -1;
	}
	if (ER_FAIL == hdmitx_hdcp_CheckSHA(M0, bstatus, KSVList, count, Vr)) {
		pr_err("failed to check SHA\n");
		return -1;
	}

	hdmitx_hdcp_ResumeRepeaterAuthenticate();

	HDCP_DEBUG_PRINTF("HDCP state: A9 -> A4\n");
	hdmitx_hdcp_state = HDCP_STATE_A4;

	return 0;
}

#endif /* SUPPORT_HDCP_STS */

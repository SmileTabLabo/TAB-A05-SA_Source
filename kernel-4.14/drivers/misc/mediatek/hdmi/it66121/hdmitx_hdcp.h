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
#ifndef _HDMITX_HDCP_H_
#define _HDMITX_HDCP_H_

#include "itx_typedef.h"

#define REG_TX_HDCP_DESIRE 0x20
#define B_TX_ENABLE_HDCP11 (1 << 1)
#define B_TX_CPDESIRE (1 << 0)

#define REG_TX_AUTHFIRE 0x21
#define REG_TX_LISTCTRL 0x22
#define B_TX_LISTFAIL (1 << 1)
#define B_TX_LISTDONE (1 << 0)

#define REG_TX_AKSV 0x23
#define REG_TX_AKSV0 0x23
#define REG_TX_AKSV1 0x24
#define REG_TX_AKSV2 0x25
#define REG_TX_AKSV3 0x26
#define REG_TX_AKSV4 0x27

#define REG_TX_AN 0x28
#define REG_TX_AN_GEN 0x30
#define REG_TX_ARI 0x38
#define REG_TX_ARI0 0x38
#define REG_TX_ARI1 0x39
#define REG_TX_APJ 0x3A

#define REG_TX_BKSV 0x3B
#define REG_TX_BRI 0x40
#define REG_TX_BRI0 0x40
#define REG_TX_BRI1 0x41
#define REG_TX_BPJ 0x42
#define REG_TX_BCAP 0x43
#define B_TX_CAP_HDMI_REPEATER (1 << 6)
#define B_TX_CAP_KSV_FIFO_RDY (1 << 5)
#define B_TX_CAP_HDMI_FAST_MODE (1 << 4)
#define B_CAP_HDCP_1p1 (1 << 1)
#define B_TX_CAP_FAST_REAUTH (1 << 0)
#define REG_TX_BSTAT 0x44
#define REG_TX_BSTAT0 0x44
#define REG_TX_BSTAT1 0x45
#define B_TX_CAP_HDMI_MODE (1 << 12)
#define B_TX_CAP_DVI_MODE (0 << 12)
#define B_TX_MAX_CASCADE_EXCEEDED (1 << 11)
#define M_TX_REPEATER_DEPTH (0x7 << 8)
#define O_TX_REPEATER_DEPTH 8
#define B_TX_DOWNSTREAM_OVER (1 << 7)
#define M_TX_DOWNSTREAM_COUNT 0x7F

#define REG_TX_AUTH_STAT 0x46
#define B_TX_AUTH_DONE (1 << 7)
/* ////////////////////////////////////////////////// */
/* Function Prototype */
/* ////////////////////////////////////////////////// */
extern HDMITXDEV hdmiTxDev[1];

bool getHDMITX_AuthenticationDone(void);
void hdmitx_hdcp_ClearAuthInterrupt(void);
void hdmitx_hdcp_ResetAuth(void);
void hdmitx_hdcp_Auth_Fire(void);
void hdmitx_hdcp_StartAnCipher(void);
void hdmitx_hdcp_StopAnCipher(void);
void hdmitx_hdcp_GenerateAn(void);
SYS_STATUS hdmitx_hdcp_GetBCaps(unsigned char *pBCaps,
				unsigned short *pBStatus);
SYS_STATUS hdmitx_hdcp_GetBKSV(unsigned char *pBKSV);

void hdmitx_hdcp_Reset(void);
SYS_STATUS hdmitx_hdcp_Authenticate(void);
SYS_STATUS hdmitx_hdcp_VerifyIntegration(void);
void hdmitx_hdcp_CancelRepeaterAuthenticate(void);
void hdmitx_hdcp_ResumeRepeaterAuthenticate(void);
SYS_STATUS hdmitx_hdcp_CheckSHA(unsigned char pM0[], unsigned short BStatus,
				unsigned char pKSVList[], int cDownStream,
				unsigned char Vr[]);
SYS_STATUS hdmitx_hdcp_GetKSVList(unsigned char *pKSVList,
				  unsigned char cDownStream);
SYS_STATUS hdmitx_hdcp_GetVr(unsigned char *pVr);
SYS_STATUS hdmitx_hdcp_GetM0(unsigned char *pM0);
SYS_STATUS hdmitx_hdcp_Authenticate_Repeater(void);
void hdmitx_hdcp_ResumeAuthentication(void);

////////////////////////////////////////////////////////////////////////////////

extern void hdmitx_hdcp_auth_start(int hdmi_mode);
extern void hdmitx_hdcp_auth_continue(void);

#endif /* _HDMITX_HDCP_H_ */

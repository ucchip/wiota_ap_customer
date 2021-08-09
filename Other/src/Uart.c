#define	__UART_C_

#include "Uart.h"
#include "pulpino.h"
#include "Function.h"
#include "string_lib.h"

void InitUart(IN ENUM_UART_BSP eBsp)
{
	SetBsp(eBsp);
	g_tQueUartTx.sRead = 0;
	g_tQueUartTx.sWrite = 0;
	
	g_tQueUartRx.sRead = 0;
	g_tQueUartRx.sWrite = 0;
	
	g_hSemUartTx = rt_sem_create("tx", 0, 0);
	g_hSemUartRx = rt_sem_create("rx", 0, 0);
	g_hSemReset = rt_sem_create("rst", 0, 0);
	
	REG_UART_IER = (REG_UART_IER & 0xF0) | 0x01; // set IER (interrupt enable register) on UART
//	IER |= (1 << MASK_BIT_UART0);		// enable CCE interrupt for PULPino event handler
}

void SetBsp(IN ENUM_UART_BSP eBsp)
{
	U16 usClkDiv;
	const U32 nClk = 1625000;	// clk / 16, clk = 26MHz
	U32 nClkFrac, nClkInt;
	F32 fSysClk;
	
	nClkInt = REG_CLK_PLL_INT & 0x1F;
	nClkFrac = (REG_CLK_PLL_FRAC >> 8) & 0x7FFFFF;
	fSysClk = nClk * (nClkInt + nClkFrac / 8388608.0f);
	usClkDiv = (U16)(fSysClk / eBsp) - 1;		// clk_div = sysclk / bsp / 16 - 1, auto set bsp
	
	CGREG |= (1 << CGUART);		// don't clock gate UART
	REG_UART_LCR = 0x83;		// sets 8N1 and set DLAB to 1
	REG_UART_DLM = (usClkDiv >> 8) & 0xFF;
	REG_UART_DLL = usClkDiv & 0xFF;
	REG_UART_FCR = 0xA7;		// enables 16byte FIFO and clear FIFOs
	REG_UART_LCR = 0x03;		// sets 8N1 and set DLAB to 0
	m_eBsp = eBsp;
}

U08 hextoint(IN U08 ucData)
{
	if(ucData >= '0' && ucData <= '9')
	{
		return ucData - 0x30;
	}
	else if(ucData >= 'a' && ucData <= 'f')
	{
		return ucData - 0x57;
	}
	else if(ucData >= 'A' && ucData <= 'F')
	{
		return ucData - 0x37;
	}
	else
		return 0xFF;
}

void Resp(IN U08 ucId, IN U08 ucState)
{
	S08 cBuf[128] = {'$'};
	S16 sLen;
	S32 iTmp;
	
	switch(g_eModePvt)
	{
	case PVT_MODE_GPS:
		cBuf[1] = 'G';
		cBuf[2] = 'P';
		break;
	case PVT_MODE_BDS:
		cBuf[1] = 'B';
		cBuf[2] = 'D';
		break;
	default:
		cBuf[1] = '-';
		cBuf[2] = '-';
		break;
	}
	
	switch(ucId)
	{
	case 0:
		sLen = sprintf((char *)(cBuf + 3), "FBK,,%d", ucState);
		break;
	case 1:
		sLen = sprintf((char *)(cBuf + 3), "FBK,RST,%d", ucState);
		break;
	case 2:
		sLen = sprintf((char *)(cBuf + 3), "FBK,MOW,%d", ucState);
		break;
	case 3:
		sLen = sprintf((char *)(cBuf + 3), "FBK,BOU,%d", ucState);
		break;
	case 4:
		sLen = sprintf((char *)(cBuf + 3), "FBK,SOS,%d", ucState);
		break;
	case 5:
		switch(ucState)
		{
		case 0:
			sLen = sprintf((char *)(cBuf + 3), "MOW,%d", g_eModePvt);
			break;
		case 1:
			switch(m_eBsp)
			{
			case UART_BSP_115200:
				iTmp = 1;
				break;
			case UART_BSP_19200:
				iTmp = 2;
				break;
			case UART_BSP_9600:
				iTmp = 3;
				break;
			default:
				iTmp = m_eBsp;
				break;
			}
			
			sLen = sprintf((char *)(cBuf + 3), "BOU,%d", iTmp);
			break;
		case 2:
			sLen = sprintf((char *)(cBuf + 3), "SOS,%d", g_eScene);
			break;
		default:
			return;
		}
		
		break;
	case 6:
		sLen = sprintf((char *)(cBuf + 3), "FBK,TST,%d", ucState);
		break;
	default:
		return;
	}
	
	sLen += sprintf((char *)(cBuf + sLen + 3), "*%02X\r\n", UartSum((U08 *)(cBuf + 1), sLen + 2));
	UartSend((U08 *)cBuf, sLen + 3);
}

void UartSend(IN const U08 *pData, IN U16 usLen)
{
	U16 usLenTmp;
	rt_base_t level;
	
	usLenTmp = (g_tQueUartTx.sWrite >= g_tQueUartTx.sRead) ? (g_tQueUartTx.sWrite - g_tQueUartTx.sRead) : (UART_BUF_LEN + g_tQueUartTx.sWrite - g_tQueUartTx.sRead);
	
	if((usLenTmp + usLen) > UART_BUF_LEN)
	{
		printf("uart print over flow\r\n");
		return;
	}
	
	level = rt_hw_interrupt_disable();
	
	if((g_tQueUartTx.sWrite + usLen) <= UART_BUF_LEN)
	{
		MemoryCopy(g_tQueUartTx.ucBuf + g_tQueUartTx.sWrite, pData, usLen);
	}
	else
	{
		register U16 usTmp;
		
		usTmp = UART_BUF_LEN - g_tQueUartTx.sWrite;
		MemoryCopy(g_tQueUartTx.ucBuf + g_tQueUartTx.sWrite, pData, usTmp);
		MemoryCopy(g_tQueUartTx.ucBuf, pData + usTmp, usLen - usTmp);
	}
	
	g_tQueUartTx.sWrite = (g_tQueUartTx.sWrite + usLen) % UART_BUF_LEN;
	rt_hw_interrupt_enable(level);
	rt_sem_release(g_hSemUartTx);
}

void ISR_UART0()
{
	switch(REG_UART_IIR & 0xF)
	{
	case UART_INT_ERR:	// receiver line status
		break;
	case UART_INT_DAT:	// received data available
	{
		register S16 sLen, sRxLen, i;
		register U08 *pBuf = g_tQueUartRx.ucBuf + g_tQueUartRx.sWrite;
		
		sRxLen = UART_RX_LVL;
		sLen = UART_BUF_LEN - g_tQueUartRx.sWrite;
		
		if(sLen >= sRxLen)
		{
			for(i = 0; i < sRxLen; i++)
				*pBuf++ = REG_UART_RBR;
		}
		else
		{
			for(i = 0; i < sLen; i++)
				*pBuf++ = REG_UART_RBR;
			
			pBuf = g_tQueUartRx.ucBuf;
			
			for(i = sLen; i < sRxLen; i++)
				*pBuf++ = REG_UART_RBR;
		}
		
		g_tQueUartRx.sWrite = (g_tQueUartRx.sWrite + sRxLen) % UART_BUF_LEN;
		rt_sem_release(g_hSemUartRx);
		break;
	}
	case UART_INT_TMT:	// character timeout
		while(REG_UART_LSR & 0x1)
		{
			g_tQueUartRx.ucBuf[g_tQueUartRx.sWrite++] = REG_UART_RBR;
			g_tQueUartRx.sWrite = g_tQueUartRx.sWrite % UART_BUF_LEN;
		}
		
		rt_sem_release(g_hSemUartRx);
		break;
	case UART_INT_TSP:	// transmitter holding register empty
		rt_sem_release(g_hSemUartTx);
		break;
	case UART_INT_MDM:	// modem status
		break;
	default:
		break;
	}
	
	REG_INT_CLEAR |= (1 << MASK_BIT_UART0);
}

void TaskUartTx(void *pvParameters)
{
	while(1)
	{
		if(rt_sem_take(g_hSemUartTx, RT_WAITING_FOREVER) == RT_EOK)
		{
			while(g_tQueUartTx.sWrite != g_tQueUartTx.sRead)
			{
				register U08 *pBuf;
				
				if((REG_UART_LSR & 0x20) == 0x20)
				{
					pBuf = g_tQueUartTx.ucBuf + g_tQueUartTx.sRead;
					REG_UART_THR = *pBuf;
					g_tQueUartTx.sRead = (g_tQueUartTx.sRead + 1) % UART_BUF_LEN;
				}
			}
		}
	}
}

void TaskUartRx(void *pvParameters)
{
	REG_INT_ENABLE |= (1 << MASK_BIT_UART0);	// enable CCE interrupt for PULPino event handler
	
	while(1)
	{
		if(rt_sem_take(g_hSemUartRx, 1000) == RT_EOK)
		{
			while(g_tQueUartRx.sWrite != g_tQueUartRx.sRead)
			{
				U08 *pBuf;
				U08 ucBuf[640];
				U16 usHead, usTail;
				S16 i, sLen, sIdx, sDataLen;
				
				sLen = (g_tQueUartRx.sWrite >= g_tQueUartRx.sRead) ? (g_tQueUartRx.sWrite - g_tQueUartRx.sRead) : (g_tQueUartRx.sWrite + UART_BUF_LEN - g_tQueUartRx.sRead);
				
				if(sLen < 4)
					break;
				
				sIdx = g_tQueUartRx.sRead;
				
				for(i = 0; i < 4; i++)
				{
					pBuf = g_tQueUartRx.ucBuf + g_tQueUartRx.sRead;
					ucBuf[i] = *pBuf;
					g_tQueUartRx.sRead = (g_tQueUartRx.sRead + 1) % UART_BUF_LEN;
				}
				
				usHead = (ucBuf[1] << 8) | ucBuf[0];
				sDataLen = ((ucBuf[3] << 8) | ucBuf[2]) + 6;
				
				if(0xAAAA == usHead && sDataLen < 640)
				{
					if(sLen >= sDataLen)
					{
						for(i = 2; i < sDataLen - 2; i++)
						{
							pBuf = g_tQueUartRx.ucBuf + g_tQueUartRx.sRead;
							ucBuf[i] = *pBuf;
							g_tQueUartRx.sRead = (g_tQueUartRx.sRead + 1) % UART_BUF_LEN;
						}
						
						usTail = (ucBuf[sDataLen - 3] << 8) | ucBuf[sDataLen - 4];
						
						if(0xBBBB == usTail)
						{
							switch(sDataLen)
							{
							case 600:
								g_nLoopCfg |= LoopPara(ucBuf + 2);
								break;
							default:
								break;
							}
						}
						else
							g_tQueUartRx.sRead = (sIdx + 1) % UART_BUF_LEN;
					}
					else
						g_tQueUartRx.sRead = sIdx;
				}
				else if(ucBuf[0] == '$')
				{
					BOOL bTrue = FALSE;
					
					sDataLen = 1;
					g_tQueUartRx.sRead = sIdx + 1;
					
					for(i = 1; i < sLen; i++)
					{
						switch(g_tQueUartRx.ucBuf[g_tQueUartRx.sRead])
						{
						case '$':
							sIdx = g_tQueUartRx.sRead;
							sDataLen = 0;
							break;
						default:
							break;
						}
						
						ucBuf[sDataLen++] = g_tQueUartRx.ucBuf[g_tQueUartRx.sRead++];
						g_tQueUartRx.sRead = g_tQueUartRx.sRead % UART_BUF_LEN;
						
						if(sDataLen > 5 && ucBuf[sDataLen - 2] == '\r' && ucBuf[sDataLen - 1] == '\n')
						{
							bTrue = TRUE;
							break;
						}
					}
					
					if(FALSE == bTrue)
						g_tQueUartRx.sRead = sIdx;
					else
					{
						U08 ucCrc, ucCrcIn;
						
						ucCrc = UartSum(ucBuf + 1, sDataLen - 6);
						ucCrcIn = (hextoint(ucBuf[sDataLen - 4]) << 4) | hextoint(ucBuf[sDataLen - 3]);
						
						if(ucCrc != ucCrcIn)
						{
							Resp(0, 3);
							continue;
						}
						
						switch((ucBuf[3] << 16) | (ucBuf[4] << 8) | ucBuf[5])
						{
						case RST:
							switch(ucBuf[7])
							{
							case '1':
								g_eModeRst = RST_MODE_COLD;
								break;
							case '2':
								g_eModeRst = RST_MODE_WARM;
								break;
							case '3':
								g_eModeRst = RST_MODE_HOT;
								break;
							case '5':
								g_eModeRst = RST_READ_POS_REC;
								break;
							default:
								Resp(1, 5);
								continue;
							}
							
							g_eWork = WORK_MODE_NORMAL;
							rt_sem_release(g_hSemReset);
							break;
						case QRY:
							switch((ucBuf[7] << 16) | (ucBuf[8] << 8) | ucBuf[9])
							{
							case MOW:
								Resp(5, 0);
								break;
							case BOU:
								Resp(5, 1);
								break;
							case SOS:
								Resp(5, 2);
								break;
							default:
								UnpackData(ucBuf, sDataLen);
								break;
							}
							
							break;
						case MOW:
							switch(ucBuf[7])
							{
							case '0':
								if(PVT_MODE_GPS != g_eModePvt)
								{
									g_eWork = WORK_MODE_NORMAL;
									g_eModePvt = PVT_MODE_GPS;
									rt_sem_release(g_hSemReset);
								}
								
								break;
							case '1':
								if(PVT_MODE_BDS != g_eModePvt)
								{
									g_eWork = WORK_MODE_NORMAL;
									g_eModePvt = PVT_MODE_BDS;
									rt_sem_release(g_hSemReset);
								}
								
								break;
							default:
								Resp(2, 5);
								break;
							}
							
							break;
						case BOU:
							switch(ucBuf[7])
							{
							case '1':
								Resp(3, 0);
								
								if(UART_BSP_115200 != m_eBsp)
								{
									rt_thread_mdelay(1);
									SetBsp(UART_BSP_115200);
								}
								
								break;
							case '2':
								Resp(3, 0);
								
								if(UART_BSP_19200 != m_eBsp)
								{
									rt_thread_mdelay(1);
									SetBsp(UART_BSP_19200);
								}
								
								break;
							case '3':
								Resp(3, 0);
								
								if(UART_BSP_9600 != m_eBsp)
								{
									rt_thread_mdelay(1);
									SetBsp(UART_BSP_9600);
								}
								
								break;
							default:
								Resp(3, 5);
								break;
							}
							
							break;
						case SOS:
							switch(ucBuf[7] - 0x30)
							{
							case SCENE_STATIONARY:	// stationary
							case SCENE_PEDESTRIAN:	// pedestrian
							case SCENE_PORTABLE:	// portable
							case SCENE_AUTOMOTIVE:	// automotive
							case SCENE_WRIST:		// wrist
							case SCENE_SEA:			// sea
							case SCENE_AIRBORNE_1G:	// airborne < 1g
							case SCENE_AIRBORNE_2G:	// airborne < 2g
							case SCENE_AIRBORNE_4G:	// airborne < 4g
							case SCENE_CUSTOMIZED:
								g_eScene = ucBuf[7] - 0x30;
								g_eWork = WORK_MODE_NORMAL;
								Resp(4, 0);
								rt_sem_release(g_hSemReset);
								break;
							default:
								Resp(4, 5);
								break;
							}
							
							break;
						case TST:
							switch(ucBuf[7] - 0x30)
							{
							case WORK_MODE_TST_GAIN:
							case WORK_MODE_TST_AGC:
							case WORK_MODE_NORMAL:
								g_eWork = ucBuf[7] - 0x30;
								Resp(6, 0);
								rt_sem_release(g_hSemReset);
								break;
							default:
								Resp(6, 5);
								break;;
							}
							break;
						default:
							UnpackData(ucBuf, sDataLen);
							break;
						}
					}
				}
				else
					g_tQueUartRx.sRead = (sIdx + 1) % UART_BUF_LEN;
				
				rt_thread_mdelay(1);
			}
		}
	}
}

U08 UartSum(IN register const U08 *pData, IN S16 sLen)
{
	register S16 i;
	register U08 ucRes = 0x00;
	
	for(i = 0; i < sLen; i++)
		ucRes ^= *pData++;
	
	return ucRes;
}

U08 LoopPara(IN const U08 *pBuf)
{
	U32 nData;
	U16 usData;
	S16 j;
	U08 ucSnrLvl, ucCodeOrder;
	
	ucCodeOrder = *pBuf++;
	ucSnrLvl = *pBuf++;
//	REG_CCE_LOOP_ORDER = (4 << 20) | (64 << 12) | (100 << 3) | (ucCodeOrder & 0x3);

	REG_CCE_LOOP_ORDER = (8 << 20) | (64 << 12) | (100 << 3) | (ucCodeOrder & 0x3);
	
	for(j = 0; j < 10; j++)
	{
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_CN0_TH(j) = nData;
	}
	
	usData = *pBuf++;
	usData = (usData << 8) | *pBuf++;
	MEM_GPS_PLI_SMOOTH(0) = usData << 16;
	usData = *pBuf++;
	usData = (usData << 8) | *pBuf++;
	MEM_GPS_PLI_SMOOTH(1) = usData << 16;
	usData = *pBuf++;
	usData = (usData << 8) | *pBuf++;
	MEM_GPS_FLI_SMOOTH(0) = usData << 16;
	usData = *pBuf++;
	usData = (usData << 8) | *pBuf++;
	MEM_GPS_FLI_SMOOTH(1) = usData << 16;
	
	for(j = 0; j < 2; j++)
	{
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_FLI_PLI_TH(ucSnrLvl, j) = nData;
	}
	
	// ALL 20ms, 2*20ms, 5*20ms, 10*20ms, 15*20ms
	for(j = 0; j < 5; j++)
	{
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_ALL_CFG(j, 0) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_ALL_CFG(j, 1) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_ALL_CFG(j, 2) = nData;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_ALL_CFG(j, 3) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_ALL_CFG(j, 4) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_ALL_CFG(j, 5) = nData;
	}
	
	// PLL order-2
	// PLL 1ms, 2ms, 4ms, 10ms, 20ms
	for(j = 0; j < 5; j++)
	{
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG1(ucSnrLvl, j, 0) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG1(ucSnrLvl, j, 1) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG1(ucSnrLvl, j, 2) = nData;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG1(ucSnrLvl, j, 3) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG1(ucSnrLvl, j, 4) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG1(ucSnrLvl, j, 5) = nData;
	}
	
	// PLL 2*20ms, 5*20ms, 10*20ms, 15*20ms, 30*20ms
	for(j = 0; j < 5; j++)
	{
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG2(j, 0) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG2(j, 1) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG2(j, 2) = nData;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG2(j, 3) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG2(j, 4) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_PLL2_CFG2(j, 5) = nData;
	}
	
	// PLL order-3
	// PLL 1ms, 2ms, 4ms, 10ms, 20ms
	for(j = 0; j < 5; j++)
	{
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL3_CFG(ucSnrLvl, j, 0) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL3_CFG(ucSnrLvl, j, 1) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_PLL3_CFG(ucSnrLvl, j, 2) = nData;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL3_CFG(ucSnrLvl, j, 3) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL3_CFG(ucSnrLvl, j, 4) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_PLL3_CFG(ucSnrLvl, j, 5) = nData;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL3_CFG(ucSnrLvl, j, 6) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_PLL3_CFG(ucSnrLvl, j, 7) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_PLL3_CFG(ucSnrLvl, j, 8) = nData;
	}
	
	// FLL order-2
	// FLL 1ms, 2ms, 4ms, 10ms
	for(j = 0; j < 4; j++)
	{
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_FLL2_CFG(ucSnrLvl, j, 0) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_FLL2_CFG(ucSnrLvl, j, 1) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_FLL2_CFG(ucSnrLvl, j, 2) = nData;
	}
	
	// FLL order-3
	// FLL 1ms, 2ms, 4ms, 10ms
	for(j = 0; j < 4; j++)
	{
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_FLL3_CFG(ucSnrLvl, j, 0) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_FLL3_CFG(ucSnrLvl, j, 1) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_FLL3_CFG(ucSnrLvl, j, 2) = nData;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_FLL3_CFG(ucSnrLvl, j, 3) = usData << 16;
		usData = *pBuf++;
		usData = (usData << 8) | *pBuf++;
		MEM_GPS_FLL3_CFG(ucSnrLvl, j, 4) = usData << 16;
		nData = *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		nData = (nData << 8) | *pBuf++;
		MEM_GPS_FLL3_CFG(ucSnrLvl, j, 5) = nData;
	}
	
	// DLL 1ms
	usData = *pBuf++;
	usData = (usData << 8) | *pBuf++;
	MEM_GPS_DLL2_CFG1(ucSnrLvl, 0, 0) = usData << 16;
	usData = *pBuf++;
	usData = (usData << 8) | *pBuf++;
	MEM_GPS_DLL2_CFG1(ucSnrLvl, 0, 1) = usData << 16;
	nData = *pBuf++;
	nData = (nData << 8) | *pBuf++;
	nData = (nData << 8) | *pBuf++;
	nData = (nData << 8) | *pBuf++;
	MEM_GPS_DLL2_CFG1(ucSnrLvl, 0, 2) = nData;
	usData = *pBuf++;
	usData = (usData << 8) | *pBuf++;
	MEM_GPS_DLL2_CFG1(ucSnrLvl, 0, 3) = usData << 16;
	usData = *pBuf++;
	usData = (usData << 8) | *pBuf++;
	MEM_GPS_DLL2_CFG1(ucSnrLvl, 0, 4) = usData << 16;
	nData = *pBuf++;
	nData = (nData << 8) | *pBuf++;
	nData = (nData << 8) | *pBuf++;
	nData = (nData << 8) | *pBuf++;
	MEM_GPS_DLL2_CFG1(ucSnrLvl, 0, 5) = nData;
	
	// DLL 20ms-width, 20ms-narrow, narrow-corr, 100ms
	if(0x0 == (ucCodeOrder & 0x2))
	{
		for(j = 0; j < 2; j++)
		{
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG3(ucSnrLvl, j, 0) = usData << 16;
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG3(ucSnrLvl, j, 1) = usData << 16;
			nData = *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG3(ucSnrLvl, j, 2) = nData;
		}
		
		for(j = 0; j < 6; j++)
		{
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG4(j, 0) = usData << 16;
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG4(j, 1) = usData << 16;
			nData = *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG4(j, 2) = nData;
		}
	}
	else
	{
		for(j = 0; j < 2; j++)
		{
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG1(ucSnrLvl, j + 1, 0) = usData << 16;
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG1(ucSnrLvl, j + 1, 1) = usData << 16;
			nData = *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG1(ucSnrLvl, j + 1, 2) = nData;
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG1(ucSnrLvl, j + 1, 3) = usData << 16;
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG1(ucSnrLvl, j + 1, 4) = usData << 16;
			nData = *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG1(ucSnrLvl, j + 1, 5) = nData;
		}
		
		for(j = 0; j < 2; j++)
		{
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG2(j + 1, 0) = usData << 16;
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG2(j + 1, 1) = usData << 16;
			nData = *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG2(j + 1, 2) = nData;
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG2(j + 1, 3) = usData << 16;
			usData = *pBuf++;
			usData = (usData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG2(j + 1, 4) = usData << 16;
			nData = *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			nData = (nData << 8) | *pBuf++;
			MEM_GPS_DLL2_CFG2(j + 1, 5) = nData;
		}
	}
	
	return (1 << ucSnrLvl);
}

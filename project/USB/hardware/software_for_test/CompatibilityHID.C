
/********************************** (C) COPYRIGHT *******************************
* File Name          :CompatibilityHID.C
* Author             : WCH
* Version            : V1.2
* Date               : 2018/02/28
* Description        : CH554ģ��HID�����豸��֧���ж����´���֧�ֿ��ƶ˵����´���֧������ȫ�٣�����
*******************************************************************************/

//������û����Ӳ��spi��̫����
#define SOFT_SPI

#include "CH552.H"
#include "Debug.H"
#ifdef SOFT_SPI
#else
#include "SPI.H"
#endif
#include <stdio.h>
#include <string.h>


sbit spi_reset = P3^2;
sbit spi_cs = P1^4;
sbit spi_dc = P3^1;
sbit spi_led = P3^4;
sbit led1=P3^0;
sbit led2=P3^3;
sbit led3=P1^1;

#ifdef SOFT_SPI
sbit sclk = P1^7;
sbit mosi = P1^5;
#endif

void LCD_WR_DATA8(UINT8 dat)
{

    spi_cs = 0;
    //
    #ifdef SOFT_SPI
    {
        UINT8 i;
        for(i=0;i<8;i++)
        {
            sclk=0;
            if(dat&0x80)
            {
                mosi=1;
            }
            else
            {
                mosi=0;
            }
            sclk=1;
            dat<<=1;
        }
    }
    #else
        CH554SPIMasterWrite(dat);
    #endif
    spi_cs = 1;
}
void LCD_WR_DATA(UINT16 dat)
{
    LCD_WR_DATA8(dat>>8);
    LCD_WR_DATA8(dat);
}
void LCD_WR_REG(UINT8 dat)
{
    spi_dc = 0;
    LCD_WR_DATA8(dat);
    spi_dc = 1;
}
void LCD_Address_Set(UINT16 x1,UINT16 y1,UINT16 x2,UINT16 y2)
{
	LCD_WR_REG(0x2a);//�е�ַ����
	LCD_WR_DATA(x1);
	LCD_WR_DATA(x2);
	LCD_WR_REG(0x2b);//�е�ַ����
	LCD_WR_DATA(y1);
	LCD_WR_DATA(y2);
	LCD_WR_REG(0x2c);//������д
}

#define USE_HORIZONTAL = 0

#define Fullspeed               1

#ifdef  Fullspeed
#define THIS_ENDP0_SIZE         64
#else
#define THIS_ENDP0_SIZE         8                                                  //����USB���жϴ��䡢���ƴ�����������Ϊ8
#endif
UINT8X  Ep0Buffer[8>(THIS_ENDP0_SIZE+2)?8:(THIS_ENDP0_SIZE+2)] _at_ 0x0000;        //�˵�0 OUT&IN��������������ż��ַ
UINT8X  Ep2Buffer[128>(2*MAX_PACKET_SIZE+4)?128:(2*MAX_PACKET_SIZE+4)] _at_ 0x0044;//�˵�2 IN&OUT������,������ż��ַ
UINT8   SetupReq,SetupLen,Ready,Count,FLAG,UsbConfig;
PUINT8  pDescr;                                                                    //USB���ñ�־
USB_SETUP_REQ   SetupReqBuf;                                                       //�ݴ�Setup��
#define UsbSetupBuf     ((PUSB_SETUP_REQ)Ep0Buffer)

sbit Ep2InKey = P1^5;                                                              //K1����
#pragma  NOAREGS
/*�豸������*/
UINT8C DevDesc[18] = {0x12,0x01,0x10,0x01,0x00,0x00,0x00,THIS_ENDP0_SIZE,
                      0x33,0x23/*<--vid*/,0x34,0x24/*<--pid*/,0x00,0x00,0x00,0x00,
                      0x00,0x01
                     };
UINT8C CfgDesc[41] =
{
    0x09,0x02,0x29,0x00,0x01,0x01,0x04,0xA0,0x23,               //����������
    0x09,0x04,0x00,0x00,0x02,0x03,0x00,0x00,0x05,               //�ӿ�������
    0x09,0x21,0x00,0x01,0x00,0x01,0x22,0x22,0x00,               //HID��������
#ifdef  Fullspeed
    0x07,0x05,0x82,0x03,THIS_ENDP0_SIZE,0x00,0x01,              //�˵�������(ȫ�ټ��ʱ��ĳ�1ms)
    0x07,0x05,0x02,0x03,THIS_ENDP0_SIZE,0x00,0x01,              //�˵�������
#else
    0x07,0x05,0x82,0x03,THIS_ENDP0_SIZE,0x00,0x0A,              //�˵�������(���ټ��ʱ����С10ms)
    0x07,0x05,0x02,0x03,THIS_ENDP0_SIZE,0x00,0x0A,              //�˵�������
#endif
};
/*�ַ��������� ��*/

/*HID�౨��������*/
UINT8C HIDRepDesc[ ] =
{
    0x06, 0x00,0xff,
    0x09, 0x01,
    0xa1, 0x01,                                                   //���Ͽ�ʼ
    0x09, 0x02,                                                   //Usage Page  �÷�
    0x15, 0x00,                                                   //Logical  Minimun
    0x26, 0x00,0xff,                                              //Logical  Maximun
    0x75, 0x08,                                                   //Report Size
    0x95, THIS_ENDP0_SIZE,                                        //Report Counet
    0x81, 0x06,                                                   //Input
    0x09, 0x02,                                                   //Usage Page  �÷�
    0x15, 0x00,                                                   //Logical  Minimun
    0x26, 0x00,0xff,                                              //Logical  Maximun
    0x75, 0x08,                                                   //Report Size
    0x95, THIS_ENDP0_SIZE,                                        //Report Counet
    0x91, 0x06,                                                   //Output
    0xC0
};
// unsigned char  code LangDes[]={0x04,0x03,0x09,0x04};           //����������
// unsigned char  code SerDes[]={
//                           0x28,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                           0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                           0x00,0x00,0x00,0x00,0x00,0x49,0x00,0x43,0x00,0x42,
//                           0x00,0x43,0x00,0x31,0x00,0x00,0x00,0x00,0x00,0x00
//                           };                                   //�ַ���������

UINT8X UserEp2Buf[64];                                            //�û����ݶ���
UINT8 Endp2Busy = 0;

/*******************************************************************************
* Function Name  : USBDeviceInit()
* Description    : USB�豸ģʽ����,�豸ģʽ�������շ��˵����ã��жϿ���
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceInit()
{
	IE_USB = 0;
	USB_CTRL = 0x00;                                                           // ���趨USB�豸ģʽ
	UDEV_CTRL = bUD_PD_DIS;                                                    // ��ֹDP/DM��������
#ifndef Fullspeed
    UDEV_CTRL |= bUD_LOW_SPEED;                                                //ѡ�����1.5Mģʽ
    USB_CTRL |= bUC_LOW_SPEED;
#else
    UDEV_CTRL &= ~bUD_LOW_SPEED;                                               //ѡ��ȫ��12Mģʽ��Ĭ�Ϸ�ʽ
    USB_CTRL &= ~bUC_LOW_SPEED;
#endif
    UEP2_DMA = Ep2Buffer;                                                      //�˵�2���ݴ����ַ
    UEP2_3_MOD |= bUEP2_TX_EN | bUEP2_RX_EN;                                   //�˵�2���ͽ���ʹ��
    UEP2_3_MOD &= ~bUEP2_BUF_MOD;                                              //�˵�2�շ���64�ֽڻ�����
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;                 //�˵�2�Զ���תͬ����־λ��IN���񷵻�NAK��OUT����ACK
    UEP0_DMA = Ep0Buffer;                                                      //�˵�0���ݴ����ַ
    UEP4_1_MOD &= ~(bUEP4_RX_EN | bUEP4_TX_EN);                                //�˵�0��64�ֽ��շ�������
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;                                 //OUT���񷵻�ACK��IN���񷵻�NAK

	USB_DEV_AD = 0x00;
	USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;                     // ����USB�豸��DMA�����ж��ڼ��жϱ�־δ���ǰ�Զ�����NAK
	UDEV_CTRL |= bUD_PORT_EN;                                                  // ����USB�˿�
	USB_INT_FG = 0xFF;                                                         // ���жϱ�־
	USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
	IE_USB = 1;
}

/*******************************************************************************
* Function Name  : Enp2BlukIn()
* Description    : USB�豸ģʽ�˵�2�������ϴ�
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void Enp2BlukIn( )
{
    memcpy( Ep2Buffer+MAX_PACKET_SIZE, UserEp2Buf, sizeof(UserEp2Buf));        //�����ϴ�����
    UEP2_T_LEN = THIS_ENDP0_SIZE;                                              //�ϴ���������
    UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;                  //������ʱ�ϴ����ݲ�Ӧ��ACK
}

/*******************************************************************************
* Function Name  : DeviceInterrupt()
* Description    : CH559USB�жϴ�������
*******************************************************************************/
void    DeviceInterrupt( void ) interrupt INT_NO_USB using 1                    //USB�жϷ������,ʹ�üĴ�����1
{
    UINT8 len,i,type,count;
    if(UIF_TRANSFER)                                                            //USB������ɱ�־
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
        case UIS_TOKEN_IN | 2:                                                  //endpoint 2# �˵������ϴ�
            UEP2_T_LEN = 0;                                                    //Ԥʹ�÷��ͳ���һ��Ҫ���
//            UEP1_CTRL ^= bUEP_T_TOG;                                          //����������Զ���ת����Ҫ�ֶ���ת
            Endp2Busy = 0 ;
			UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //Ĭ��Ӧ��NAK
            break;
        case UIS_TOKEN_OUT | 2:                                                 //endpoint 2# �˵������´�
            if ( U_TOG_OK )                                                     // ��ͬ�������ݰ�������
            {
                len = USB_RX_LEN;                                               //�������ݳ��ȣ����ݴ�Ep2Buffer�׵�ַ��ʼ���
                type = 0;//�洢��������
                count = 0;//�洢ʣ���ֽ���
                /*
                    hid --> spi Э��
                    ͷ����������  ��������ֽڳ���
                        1bit     7bit
                    Ȼ����Ϻ��������
                */
                for ( i = 0; i < len; i ++ )
                {
                    if(count==0)
                    {
                        count = Ep2Buffer[i] & 0x7F;
                        type = Ep2Buffer[i] >> 7;
                        Ep2Buffer[MAX_PACKET_SIZE+i] = 0x00;
                    }
                    else
                    {
                        if(type)//1��ʾdata
                            LCD_WR_DATA8(Ep2Buffer[i]);
                        else//0��ʾ����
                            LCD_WR_REG(Ep2Buffer[i]);
                        count--;
                        Ep2Buffer[MAX_PACKET_SIZE+i] = 0xcc;
                    }
                }
                UEP2_T_LEN = len;
                UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;       // �����ϴ�
            }
            break;
        case UIS_TOKEN_SETUP | 0:                                               //SETUP����
            len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ)))
            {
                SetupLen = UsbSetupBuf->wLengthL;
                len = 0;                                                         // Ĭ��Ϊ�ɹ������ϴ�0����
                SetupReq = UsbSetupBuf->bRequest;
                if ( ( UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )/*HID������*/
                {
					switch( SetupReq )
					{
						case 0x01:                                                  //GetReport
							pDescr = UserEp2Buf;                                    //���ƶ˵��ϴ����
							if(SetupLen >= THIS_ENDP0_SIZE)                         //���ڶ˵�0��С����Ҫ���⴦��
							{
								len = THIS_ENDP0_SIZE;
							}
							else
							{
								len = SetupLen;
							}
							break;
						case 0x02:                                                   //GetIdle
							break;
						case 0x03:                                                   //GetProtocol
							break;
						case 0x09:                                                   //SetReport
							break;
						case 0x0A:                                                   //SetIdle
							break;
						case 0x0B:                                                   //SetProtocol
							break;
						default:
							len = 0xFF;  				                              /*���֧��*/
							break;
					}
					if( SetupLen > len )
					{
						SetupLen = len;    //�����ܳ���
					}
					len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;   //���δ��䳤��
					memcpy(Ep0Buffer,pDescr,len);                                     //�����ϴ�����
					SetupLen -= len;
					pDescr += len;
                }
                else                                                             //��׼����
                {
                    switch(SetupReq)                                             //������
                    {
                    case USB_GET_DESCRIPTOR:
                        switch(UsbSetupBuf->wValueH)
                        {
                        case 1:                                                  //�豸������
                            pDescr = DevDesc;                                    //���豸�������͵�Ҫ���͵Ļ�����
                            len = sizeof(DevDesc);
                            break;
                        case 2:                                                  //����������
                            pDescr = CfgDesc;                                    //���豸�������͵�Ҫ���͵Ļ�����
                            len = sizeof(CfgDesc);
                            break;
                        case 0x22:                                               //����������
                            pDescr = HIDRepDesc;                                 //����׼���ϴ�
                            len = sizeof(HIDRepDesc);
                            break;
                        default:
                            len = 0xff;                                          //��֧�ֵ�������߳���
                            break;
                        }
                        if ( SetupLen > len )
                        {
                            SetupLen = len;    //�����ܳ���
                        }
                        len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;//���δ��䳤��
                        memcpy(Ep0Buffer,pDescr,len);                            //�����ϴ�����
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;                         //�ݴ�USB�豸��ַ
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if ( SetupLen >= 1 )
                        {
                            len = 1;
                        }
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
						if(UsbConfig)
						{
							Ready = 1;                                            //set config����һ�����usbö����ɵı�־
						}
                        break;
                    case 0x0A:
                        break;
                    case USB_CLEAR_FEATURE:                                      //Clear Feature
                        if ( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )// �˵�
                        {
                            switch( UsbSetupBuf->wIndexL )
                            {
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            default:
                                len = 0xFF;                                       // ��֧�ֵĶ˵�
                                break;
                            }
                        }
                        else
                        {
                            len = 0xFF;                                           // ���Ƕ˵㲻֧��
                        }
                        break;
                    case USB_SET_FEATURE:                                         /* Set Feature */
                        if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x00 )        /* �����豸 */
                        {
                            if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x01 )
                            {
                                if( CfgDesc[ 7 ] & 0x20 )
                                {
                                    /* ���û���ʹ�ܱ�־ */
                                }
                                else
                                {
                                    len = 0xFF;                                    /* ����ʧ�� */
                                }
                            }
                            else
                            {
                                len = 0xFF;                                        /* ����ʧ�� */
                            }
                        }
                        else if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x02 )    /* ���ö˵� */
                        {
                            if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x00 )
                            {
                                switch( ( ( UINT16 )UsbSetupBuf->wIndexH << 8 ) | UsbSetupBuf->wIndexL )
                                {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* ���ö˵�2 IN STALL */
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* ���ö˵�2 OUT Stall */
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* ���ö˵�1 IN STALL */
                                    break;
                                default:
                                    len = 0xFF;                                     /* ����ʧ�� */
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFF;                                         /* ����ʧ�� */
                            }
                        }
                        else
                        {
                            len = 0xFF;                                             /* ����ʧ�� */
                        }
                        break;
                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;
                        if ( SetupLen >= 2 )
                        {
                            len = 2;
                        }
                        else
                        {
                            len = SetupLen;
                        }
                        break;
                    default:
                        len = 0xff;                                                  //����ʧ��
                        break;
                    }
                }
            }
            else
            {
                len = 0xff;                                                          //�����ȴ���
            }
            if(len == 0xff)
            {
                SetupReq = 0xFF;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;//STALL
            }
            else if(len <= THIS_ENDP0_SIZE)                                         //�ϴ����ݻ���״̬�׶η���0���Ȱ�
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//Ĭ�����ݰ���DATA1������Ӧ��ACK
            }
            else
            {
                UEP0_T_LEN = 0;  //��Ȼ��δ��״̬�׶Σ�������ǰԤ���ϴ�0�������ݰ��Է�������ǰ����״̬�׶�
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//Ĭ�����ݰ���DATA1,����Ӧ��ACK
            }
            break;
        case UIS_TOKEN_IN | 0:                                                      //endpoint0 IN
            switch(SetupReq)
            {
            case USB_GET_DESCRIPTOR:
            case HID_GET_REPORT:
                len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;     //���δ��䳤��
                memcpy( Ep0Buffer, pDescr, len );                                   //�����ϴ�����
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;                                            //ͬ����־λ��ת
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0;                                                      //״̬�׶�����жϻ�����ǿ���ϴ�0�������ݰ��������ƴ���
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0:  // endpoint0 OUT
            len = USB_RX_LEN;
            if(SetupReq == 0x09)
            {
                if(Ep0Buffer[0])
                {
                    //printf("Light on Num Lock LED!\n");
                }
                else if(Ep0Buffer[0] == 0)
                {
                    //printf("Light off Num Lock LED!\n");
                }
            }
            UEP0_CTRL ^= bUEP_R_TOG;                                     //ͬ����־λ��ת
            break;
        default:
            break;
        }
        UIF_TRANSFER = 0;                                                           //д0����ж�
    }
    if(UIF_BUS_RST)                                                                 //�豸ģʽUSB���߸�λ�ж�
    {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK;
        UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
		Endp2Busy = 0;
        UIF_BUS_RST = 0;                                                             //���жϱ�־
    }
    if (UIF_SUSPEND)                                                                 //USB���߹���/�������
    {
        UIF_SUSPEND = 0;
        if ( USB_MIS_ST & bUMS_SUSPEND )                                             //����
        {
#ifdef DE_PRINTF
            //printf( "zz" );                                                          //˯��״̬
#endif
//             while ( XBUS_AUX & bUART0_TX )
//             {
//                 ;    //�ȴ��������
//             }
//             SAFE_MOD = 0x55;
//             SAFE_MOD = 0xAA;
//             WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO;                                   //USB����RXD0���ź�ʱ�ɱ�����
//             PCON |= PD;                                                               //˯��
//             SAFE_MOD = 0x55;
//             SAFE_MOD = 0xAA;
//             WAKE_CTRL = 0x00;
        }
    }
    else {                                                                             //������ж�,�����ܷ��������
        USB_INT_FG = 0xFF;                                                             //���жϱ�־
//      printf("UnknownInt  N");
    }
}



main()
{
    UINT8 i;
    led1=0;led2=0;led3=0;
    CfgFsys( );                                                           //CH559ʱ��ѡ������
    mDelaymS(5);                                                          //�޸���Ƶ�ȴ��ڲ������ȶ�,�ؼ�
    for(i=0; i<64; i++)                                                   //׼����ʾ����
    {
        UserEp2Buf[i] = i;
    }
    USBDeviceInit();                                                      //USB�豸ģʽ��ʼ��
    EA = 1;                                                               //������Ƭ���ж�
    UEP1_T_LEN = 0;                                                       //Ԥʹ�÷��ͳ���һ��Ҫ���
    UEP2_T_LEN = 0;                                                       //Ԥʹ�÷��ͳ���һ��Ҫ���
    FLAG = 0;
    Ready = 0;

#ifdef SOFT_SPI
    sclk=0;mosi=0;
#else
    SPIMasterModeSet(3);
    SPI_CK_SET(4);
#endif
    spi_reset = 0;spi_cs = 1;spi_dc = 1;
    mDelaymS(100);
    spi_reset = 1;
    mDelaymS(100);
    spi_led = 1;
    mDelaymS(100);
	LCD_WR_REG(0x11); //Sleep out
	mDelaymS(120);              //Delay 120ms
	//************* Start Initial Sequence **********//
	LCD_WR_REG(0x36);
	LCD_WR_DATA8(0x00);

	LCD_WR_REG(0x3A);
	LCD_WR_DATA8(0x05);

	LCD_WR_REG(0xB2);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x33);

	LCD_WR_REG(0xB7);
	LCD_WR_DATA8(0x35);

	LCD_WR_REG(0xBB);
	LCD_WR_DATA8(0x32); //Vcom=1.35V

	LCD_WR_REG(0xC2);
	LCD_WR_DATA8(0x01);

	LCD_WR_REG(0xC3);
	LCD_WR_DATA8(0x15); //GVDD=4.8V  ��ɫ���

	LCD_WR_REG(0xC4);
	LCD_WR_DATA8(0x20); //VDV, 0x20:0v

	LCD_WR_REG(0xC6);
	LCD_WR_DATA8(0x0F); //0x0F:60Hz

	LCD_WR_REG(0xD0);
	LCD_WR_DATA8(0xA4);
	LCD_WR_DATA8(0xA1);

	LCD_WR_REG(0xE0);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x08);
	LCD_WR_DATA8(0x0E);
	LCD_WR_DATA8(0x09);
	LCD_WR_DATA8(0x09);
	LCD_WR_DATA8(0x05);
	LCD_WR_DATA8(0x31);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x48);
	LCD_WR_DATA8(0x17);
	LCD_WR_DATA8(0x14);
	LCD_WR_DATA8(0x15);
	LCD_WR_DATA8(0x31);
	LCD_WR_DATA8(0x34);

	LCD_WR_REG(0xE1);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x08);
	LCD_WR_DATA8(0x0E);
	LCD_WR_DATA8(0x09);
	LCD_WR_DATA8(0x09);
	LCD_WR_DATA8(0x15);
	LCD_WR_DATA8(0x31);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x48);
	LCD_WR_DATA8(0x17);
	LCD_WR_DATA8(0x14);
	LCD_WR_DATA8(0x15);
	LCD_WR_DATA8(0x31);
	LCD_WR_DATA8(0x34);
	LCD_WR_REG(0x21);

	LCD_WR_REG(0x29);

    {
        UINT16 i,j,xsta=0,ysta=0,xend=240,yend=240;
        LCD_Address_Set(xsta,ysta,xend-1,yend-1);//������ʾ��Χ
        for(i=ysta;i<yend;i++)
        {
            for(j=xsta;j<xend;j++)
            {
                LCD_WR_DATA(0x0000);
            }
        }
    }

    while(1)
    {
        mDelaymS(100);
        // led1=0;led2=1;led3=0;
        // mDelaymS(500);
        // led1=1;led2=0;led3=1;
        // mDelaymS(500);
    }

}
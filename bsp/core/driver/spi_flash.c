
/*----------------------------------------------------------------------------------------------------
	INCLUDE HEADE FILES
----------------------------------------------------------------------------------------------------*/
#include "xinc_m0.h"
//#include "bsp_com_spi.h"
#include "spi_flash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"
#include "xc620.h"

/*-------------------------------------------------------------------
NOTE: bsp_spi0_flash�ļ��µı���Ҫ����SHRAM0����.
-------------------------------------------------------------------*/
uint8_t __attribute__((aligned(4)))	txbuff[(PACKET_FULL_LEN+4)];
uint8_t __attribute__((aligned(4)))	rxbuff[(PACKET_FULL_LEN+4)];

/* ---------------------------------------------------------------------------------------------------
- ��������: init_spi_master
- ��������: ��ʼ��spi��ģʽ
- ��������: 2015-09-14
----------------------------------------------------------------------------------------------------*/
void init_spi_flash(uint32_t ch, uint32_t freq)
{
    uint32_t val;
    writeReg32(CPR_SPIx_MCLK_CTL(ch), 0x110010);           	//- spi(x)_mclk = 32Mhz(When TXCO=32Mhz).
    writeReg32(CPR_CTLAPBCLKEN_GRCTL, (0x1000100 << ch)); 	//- enable spi(x) pclk.
    readReg32(CPR_SSI_CTRL, val);
    val |= (ch==0)? 0x01: 0x30;
    writeReg32(CPR_SSI_CTRL, val);
	
    writeReg32(SSIx_EN(ch), 0x00);

    writeReg32(SSIx_IE(ch), 0x00);
    writeReg32(SSIx_CTRL0(ch) , 0x0F);					/* 16bit SPI data */

    writeReg32(SSIx_SE(ch), 0x01);
    writeReg32(SSIx_BAUD(ch), freq);						//- spix_mclk ��Ƶ.

    writeReg32(SSIx_RXFTL(ch), 0x00);
    writeReg32(SSIx_TXFTL(ch), 0x00);

    //writeReg32(SSIx_EN(ch) , 0x01);
}
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_Read_128bitsID
- ��������: ��ȡflash 128bits ID ��������ID���� ��ָ����Զ�ȡһ��оƬ�Ѿ��̻���ֻ����128λID
- ��������: 2015-09-14
----------------------------------------------------------------------------------------------------*/
void spi_flash_read_128bitsID(uint8_t *buff)
{
    while(spi_flash_wait_till_ready());
    uint32_t iWK = 0;
    memset(txbuff,0, PACKET_FULL_LEN+4);
    memset(rxbuff,0, PACKET_FULL_LEN+4);
	  txbuff[1] = CMD_ID;
    
    writeReg32(SSI0_EN, 0x00);
    writeReg32(SSI0_DMAS, 0x03);
    writeReg32(SSI0_DMATDL, 0x4);
    writeReg32(SSI0_DMARDL, 0x4);     //- 1/4 FIFO
    writeReg32(SSI0_EN, 0x01);

  	//- RX Channel
	  writeReg32(DMAS_CHx_SAR(10), 0x40013060);
    writeReg32(DMAS_CHx_DAR(10), (uint32_t)rxbuff);
    writeReg32(DMAS_CHx_CTL1(10),((2 << 8) | 1));
    writeReg32(DMAS_CHx_CTL0(10),24);
    writeReg32(DMAS_EN, 10);

		//- TX Channel
	  writeReg32(DMAS_CHx_SAR(2), (uint32_t)txbuff);
    writeReg32(DMAS_CHx_DAR(2), 0x40013060);
    writeReg32(DMAS_CHx_CTL1(2),((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(2), 24);   //���ջ������׵�ַ������4�ı������Ҵ�С�������ĵı���
    writeReg32(DMAS_EN, 2);

    do	{
    	readReg32(DMAS_INT_RAW, iWK);
    }while((iWK&0x404) != 0x404);

    writeReg32(DMAS_INT_RAW, 0x404);
    writeReg32(DMAS_CLR, 10);
    writeReg32(DMAS_CLR, 2);
    writeReg32(SSI0_EN, 0x00);  
   
    for(int i = 0; i < 12; i++)
    {
        uint8_t temp = rxbuff[2*i+1];
        rxbuff[2*i+1] = rxbuff[2*i];
        rxbuff[2*i] = temp;
    }
    for(int i = 0; i < 16; i++)
				buff[i] = rxbuff[5+i];
}

/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_read_page
- ��������: ��SPI FLASH�ж�ȡһҳ���ݵ�ָ����ַ
- ��������: 2015-09-14
----------------------------------------------------------------------------------------------------*/
void spi_flash_read_page(uint32_t PageAddR, uint8_t *buff)
{
	uint32_t addr = PageAddR;
	uint32_t iWK = 0;

	txbuff[0] = (uint8_t)(addr>>16);
	txbuff[1] = CMD_READ_DATA;
	txbuff[2] = (uint8_t)(addr);
	txbuff[3] = (uint8_t)(addr>>8);		

	writeReg32(SSI0_EN, 0x00);
	writeReg32(SSI0_DMAS, 0x03);
	writeReg32(SSI0_DMATDL, 0x4);          //-
	writeReg32(SSI0_DMARDL, 0x4);          //- 1/4 FIFO
	writeReg32(SSI0_EN, 0x01);

	//- RX Channel
	writeReg32(DMAS_CHx_SAR(10), 0x40013060);
	writeReg32(DMAS_CHx_DAR(10), (uint32_t)buff);
	writeReg32(DMAS_CHx_CTL1(10),((2 << 8)|  1));
	writeReg32(DMAS_CHx_CTL0(10),(PACKET_FULL_LEN+4));
	writeReg32(DMAS_EN, 10);

	//- TX Channel
	writeReg32(DMAS_CHx_SAR(2), (uint32_t)txbuff);
	writeReg32(DMAS_CHx_DAR(2), 0x40013060);
	writeReg32(DMAS_CHx_CTL1(2), ((2 << 8)|  1));
	writeReg32(DMAS_CHx_CTL0(2), (PACKET_FULL_LEN+4));
	writeReg32(DMAS_EN, 2);

	do {
	readReg32(DMAS_INT_RAW, iWK);
	} while((iWK&0x404) != 0x404);

	writeReg32(DMAS_INT_RAW, 0x404);
	writeReg32(DMAS_CLR, 10);
	writeReg32(DMAS_CLR, 2);
	writeReg32(SSI0_EN, 0x00);    
}

/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_wait_till_ready
- ��������: ���spi flash����ready״̬
- ��������: 2015-09-14
----------------------------------------------------------------------------------------------------*/
uint8_t spi_flash_wait_till_ready(void)
{
    uint16_t cmd = (CMD_READ_STATUS<<8);
    uint32_t iWK = 0;
    uint16_t dWK = 0;

    writeReg32(SSI0_EN , 0x00);
    writeReg32(SSI0_DMAS , 0x00);			/* turn off dma*/
    writeReg32(SSI0_DMATDL, 0x0);      	//-
    writeReg32(SSI0_DMARDL, 0x0);      	//-
    writeReg32(SSI0_EN , 0x01);

    writeReg32(SSI0_DATA , cmd);

    do	{
        	readReg32(SSI0_STS, iWK);
    }while((iWK&0x05) != 0x04);

    readReg32(SSI0_DATA , dWK);

    writeReg32(SSI0_EN , 0x00);    
    
    return	(uint8_t)(dWK&0x01);
}

/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_chip_erase
- ��������: ��������оƬ������
- ��������: 2015-09-16
----------------------------------------------------------------------------------------------------*/
void	spi_flash_chip_erase(void)
{
	  uint32_t bWk, cWk;
    readReg32(SSI0_CTRL0 , bWk);

    writeReg32(SSI0_EN , 0x00);
    writeReg32(SSI0_DMAS , 0x00);
    writeReg32(SSI0_CTRL0 , 0x07);				/* 8bit SPI data */
    writeReg32(SSI0_EN , 0x01);

    writeReg32(SSI0_DATA, CMD_CHIP_ERASE);

    do	{
        	readReg32(SSI0_STS, cWk);
    }while((cWk&0x05) != 0x04);

    writeReg32(SSI0_EN , 0x00);
    writeReg32(SSI0_CTRL0 , bWk);
}

/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_erase_block_num
- ��������: ���������block����Ŀ
- ��������: 2015-09-16
----------------------------------------------------------------------------------------------------*/
uint32_t spi_flash_block_num(uint32_t	size)
{
		uint32_t	blk = 0;
		blk = size/FLASH_BLOCK_SIZE;
		if(size % FLASH_BLOCK_SIZE) blk++;
		return blk;
}

/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_block_erase
- ��������: ����оƬָ��block������
- ��������: 2015-09-16
----------------------------------------------------------------------------------------------------*/
void spi_flash_block_erase(uint32_t	blknum)
{
    uint32_t	addr = (blknum*FLASH_BLOCK_SIZE);
    uint32_t  	iWK = 0;

    txbuff[0] = (uint8_t)(addr>>16);
    txbuff[1] = CMD_BLOCK_ERASE;
    txbuff[2] = (uint8_t)(addr);
    txbuff[3] = (uint8_t)(addr>>8);

    writeReg32(SSI0_EN , 0x00);
    writeReg32(SSI0_DMAS , 0x03);
    writeReg32(SSI0_DMATDL, 0x4);          //-
    writeReg32(SSI0_DMARDL, 0x4);          //- 1/4 FIFO
    writeReg32(SSI0_EN , 0x01);

		//- RX Channel
	  writeReg32(DMAS_CHx_SAR(10) , 0x40013060);
    writeReg32(DMAS_CHx_DAR(10) , (uint32_t)rxbuff);
    writeReg32(DMAS_CHx_CTL1(10) ,((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(10) ,4);
    writeReg32(DMAS_EN , 10);

		//- TX Channel
	  writeReg32(DMAS_CHx_SAR(2) , (uint32_t)txbuff);
    writeReg32(DMAS_CHx_DAR(2) , 0x40013060);
    writeReg32(DMAS_CHx_CTL1(2) ,((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(2) ,4);
    writeReg32(DMAS_EN , 2);

    do	{
    	readReg32(DMAS_INT_RAW , iWK);
    }while((iWK&0x404) != 0x404);

    writeReg32(DMAS_INT_RAW, 0x404);
    writeReg32(DMAS_CLR , 10);
    writeReg32(DMAS_CLR , 2);
    writeReg32(SSI0_EN , 0x00);
    
}
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_write_enable
- ��������: ʹ�ܶ�оƬ��д����
- ��������: 2015-09-16
----------------------------------------------------------------------------------------------------*/
void spi_flash_write_enable(void)
{
    uint32_t	bWk, cWk;

    readReg32(SSI0_CTRL0 , bWk);

    writeReg32(SSI0_EN, 0x00);
    writeReg32(SSI0_DMAS , 0x00);
    writeReg32(SSI0_CTRL0 , 0x07);				/* 8bit SPI data */
    writeReg32(SSI0_EN, 0x01);
    writeReg32(SSI0_DATA, CMD_WRITE_ENABLE);

    do	{
        	readReg32(SSI0_STS, cWk);
    }while((cWk&0x05) != 0x04);

    writeReg32(SSI0_EN, 0x00);
    writeReg32(SSI0_CTRL0, bWk);
}

/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_write_page
- ��������: ��SPI FLASHָ����ַд��һҳ����
- ��������: 2015-09-16
----------------------------------------------------------------------------------------------------*/
void spi_flash_write_page(uint32_t PageAddR, uint8_t *buff)
{
    uint32_t addr = PageAddR;
    uint32_t iWK = 0;
    uint32_t i;

    for(i=0; i<PACKET_FULL_LEN; i++)
    {
        txbuff[i+4] = buff[i];
    }

    txbuff[0] = (uint8_t)(addr>>16);
    txbuff[1] = CMD_PAGE_PROGRAM;
    txbuff[2] = (uint8_t)(addr);
    txbuff[3] = (uint8_t)(addr>>8);

    writeReg32(SSI0_EN , 0x00);
    writeReg32(SSI0_DMAS , 0x03);
    writeReg32(SSI0_DMATDL, 0x4);          //-
    writeReg32(SSI0_DMARDL, 0x4);          //- 1/2FIFO
    writeReg32(SSI0_EN , 0x01);

		//- RX Channel
	  writeReg32(DMAS_CHx_SAR(10), 0x40013060);
    writeReg32(DMAS_CHx_DAR(10), (uint32_t)rxbuff);
    writeReg32(DMAS_CHx_CTL1(10),((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(10),(PACKET_FULL_LEN+4));
    writeReg32(DMAS_EN, 10);

		//- TX Channel
	  writeReg32(DMAS_CHx_SAR(2), (uint32_t)txbuff);
    writeReg32(DMAS_CHx_DAR(2), 0x40013060);
    writeReg32(DMAS_CHx_CTL1(2),((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(2),(PACKET_FULL_LEN+4));
    writeReg32(DMAS_EN, 2);

    do {
    	readReg32(DMAS_INT_RAW , iWK);
    }while((iWK&0x404) != 0x404);

    writeReg32(DMAS_INT_RAW, 0x404);
    writeReg32(DMAS_CLR , 10);
    writeReg32(DMAS_CLR , 2);
    writeReg32(SSI0_EN , 0x00);
    
}
#if 1  //ʹ���Ϲ�����flash
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_page_erase   
- ��������: ����оƬָ��page������ ---�Ϲ�����flash�д˹��ܣ�GD���� �� MD���� flashû�д˹���
- ��������: 2016-06-15
----------------------------------------------------------------------------------------------------*/
void spi_flash_page_erase(uint32_t no)
{
    uint32_t addr = (no*FLASH_PAGE_SIZE);
    uint32_t iWK = 0;

    txbuff[0] = (uint8_t)(addr>>16);
    txbuff[1] = CMD_PAGE_ERASE;
    txbuff[2] = (uint8_t)(addr);
    txbuff[3] = (uint8_t)(addr>>8);

    writeReg32(SSI0_EN, 0x00);
    writeReg32(SSI0_DMAS, 0x03);
    writeReg32(SSI0_DMATDL, 0x4);          //-
    writeReg32(SSI0_DMARDL, 0x4);          //- 1/4 FIFO
    writeReg32(SSI0_EN, 0x01);

    //- RX Channel
    writeReg32(DMAS_CHx_SAR(10), 0x40013060);
    writeReg32(DMAS_CHx_DAR(10), (uint32_t)rxbuff);
    writeReg32(DMAS_CHx_CTL1(10),((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(10),4);
    writeReg32(DMAS_EN , 10);

    //- TX Channel
    writeReg32(DMAS_CHx_SAR(2), (uint32_t)txbuff);
    writeReg32(DMAS_CHx_DAR(2), 0x40013060);
    writeReg32(DMAS_CHx_CTL1(2),((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(2),4);
    writeReg32(DMAS_EN, 2);

    do	{
    	readReg32(DMAS_INT_RAW , iWK);
    }while((iWK&0x404) != 0x404);

    writeReg32(DMAS_INT_RAW, 0x404);
    writeReg32(DMAS_CLR, 10);
    writeReg32(DMAS_CLR, 2);
    writeReg32(SSI0_EN, 0x00);    
}
#else
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_sector_erase
- ��������: ����оƬָ��sector������
- ��������: 2016-06-15
----------------------------------------------------------------------------------------------------*/
void spi_flash_sector_erase(uint32_t no)
{
    uint32_t	addr = (no*FLASH_SECTOR_SIZE);
    uint32_t  	iWK = 0;

    txbuff[0] = (uint8_t)(addr>>16);
    txbuff[1] = CMD_SECTOR_ERASE;
    txbuff[2] = (uint8_t)(addr);
    txbuff[3] = (uint8_t)(addr>>8);

    writeReg32(SSI0_EN, 0x00);
    writeReg32(SSI0_DMAS, 0x03);
    writeReg32(SSI0_DMATDL, 0x4);          //-
    writeReg32(SSI0_DMARDL, 0x4);          //- 1/4 FIFO
    writeReg32(SSI0_EN, 0x01);

    //- RX Channel
    writeReg32(DMAS_CHx_SAR(10), 0x40013060);
    writeReg32(DMAS_CHx_DAR(10), (uint32_t)rxbuff);
    writeReg32(DMAS_CHx_CTL1(10),((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(10),4);
    writeReg32(DMAS_EN , 10);

    //- TX Channel
    writeReg32(DMAS_CHx_SAR(2), (uint32_t)txbuff);
    writeReg32(DMAS_CHx_DAR(2), 0x40013060);
    writeReg32(DMAS_CHx_CTL1(2),((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(2),4);
    writeReg32(DMAS_EN, 2);

    do	{
    	readReg32(DMAS_INT_RAW , iWK);
    }while((iWK&0x404) != 0x404);

    writeReg32(DMAS_INT_RAW, 0x404);
    writeReg32(DMAS_CLR, 10);
    writeReg32(DMAS_CLR, 2);
    writeReg32(SSI0_EN, 0x00);    
}
#endif
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_Release_powerdown
- ��������: SPI_FLASH Exit Power-Down
- ��������: 2016-10-24
----------------------------------------------------------------------------------------------------*/
void spi_flash_release_powerdown(void)
{
	  uint32_t bWk, cWk;

    readReg32(SSI0_CTRL0, bWk);

    writeReg32(SSI0_EN, 0x00);
    writeReg32(SSI0_DMAS, 0x00);
    writeReg32(SSI0_CTRL0, 0x07);				/* 8bit SPI data */
    writeReg32(SSI0_EN, 0x01);

    writeReg32(SSI0_DATA, CMD_RELEASE_PWRDWN);
    do	{
        	readReg32(SSI0_STS, cWk);
    }while((cWk&0x05) != 0x04);

    writeReg32(SSI0_EN, 0x00);
    writeReg32(SSI0_CTRL0, bWk);
}

/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_powerdown
- ��������: SPI_FLASH Enter Power-Down
- ��������: 2016-11-07
----------------------------------------------------------------------------------------------------*/
void spi_flash_enter_powerdown(void)
{
	  uint32_t	bWk, cWk;
    readReg32(SSI0_CTRL0, bWk);

    writeReg32(SSI0_EN, 0x00);
    writeReg32(SSI0_DMAS, 0x00);
    writeReg32(SSI0_CTRL0, 0x07);				/* 8bit SPI data */
    writeReg32(SSI0_EN, 0x01);
    writeReg32(SSI0_DATA, CMD_PWRDWN);
    do	{
        	readReg32(SSI0_STS, cWk);
    }while((cWk&0x05) != 0x04);

    writeReg32(SSI0_EN, 0x00);
    writeReg32(SSI0_CTRL0, bWk);
}    

/* ---------------------------------------------------------------------------------------------------
- ��������: spi1_write_read_uint16
- ��������: spi1 ���ⷢ�������ֽ�, �����������ֽ�
- ��������: 2016-11-07
----------------------------------------------------------------------------------------------------*/
uint16_t spi1_write_read_uint16(uint16_t val)
{
    uint32_t	cWk;
    writeReg32(SSI1_EN, 0x00);
    writeReg32(SSI1_DMAS, 0x00);
    writeReg32(SSI1_EN, 0x01);

    writeReg32(SSI1_DATA, val);

    do	{
        readReg32(SSI1_STS, cWk);
    } while((cWk&0x05) != 0x4);
    
    readReg32(SSI1_DATA, val);
    writeReg32(SSI1_EN, 0x00);    
    return	(val);    
}

/* ---------------------------------------------------------------------------------------------------
- ��������: spi1_write_read_stream
- ��������: spi1 ���ⷢ�Ͳ����س���ΪLength��һ��16BIT�������.
- ��������: 2016-11-07
----------------------------------------------------------------------------------------------------*/
void spi1_write_read_stream(uint16_t *input, uint16_t *output, uint16_t length)
{ 
    uint32_t iwk;
    writeReg32(SSI1_EN, 0x00);
    writeReg32(SSI1_DMAS, 0x03);
    writeReg32(SSI1_DMATDL, 0x2);          
    writeReg32(SSI1_DMARDL, 0x2);              
    writeReg32(SSI1_EN, 0x01);

    //- RX Channel
    writeReg32(DMAS_CHx_SAR(11), 0x40014060);
    writeReg32(DMAS_CHx_DAR(11), (uint32_t)output);
    writeReg32(DMAS_CHx_CTL1(11),((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(11),(length<<1));
    writeReg32(DMAS_EN, 11);

    //- TX Channel
    writeReg32(DMAS_CHx_SAR(3), (uint32_t)input);
    writeReg32(DMAS_CHx_DAR(3), 0x40014060);
    writeReg32(DMAS_CHx_CTL1(3),((2 << 8)|  1));
    writeReg32(DMAS_CHx_CTL0(3),(length<<1));
    writeReg32(DMAS_EN, 3);

    do	{
    	readReg32(DMAS_INT_RAW, iwk);
    }while((iwk&0x808) != 0x808);

    writeReg32(DMAS_INT_RAW, 0x808);
    writeReg32(DMAS_CLR, 11);
    writeReg32(DMAS_CLR, 3);
        
    writeReg32(SSI1_EN, 0x00);
}


#if 0

/******************************************2019.04.08����******************************************/
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_Read
- ��������: ��ָ���ĵ�ַ��ʼ��ȡָ�����ȵ�����
- ��������: 2019.04.08
- ��    ��:�¿�ΰ
----------------------------------------------------------------------------------------------------*/
void spi_flash_Read(uint32_t ReadAddR, uint8_t *buff, uint16_t ReadLength)
{
	uint32_t PagePos=ReadAddR/FLASH_PAGE_SIZE;//ҳ��ַ
	uint16_t PageOff=ReadAddR%FLASH_PAGE_SIZE;//ҳƫ�Ƶ�ַ
	uint16_t i=0,PageMain=FLASH_PAGE_SIZE-PageOff;//ҳʣ���ַ
	if(ReadLength<=PageMain)  PageMain=ReadLength; 
	while(1)
	{   	
        spi_flash_read_page(PagePos*FLASH_PAGE_SIZE,rxbuff);
		for(i=0;i<PageMain;i++)
		{
			buff[i]=rxbuff[PageOff+i+4];	  
		}
        if(ReadLength==PageMain) break;
		else
		{
			PagePos++;
			PageOff=0;
			buff+=PageMain;
            ReadAddR+=PageMain;
			ReadLength-=PageMain;
			if(ReadLength>FLASH_PAGE_SIZE) PageMain=FLASH_PAGE_SIZE;
			else PageMain=ReadLength;
		}
  }
}
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_Read_Sector
- ��������: ��SPI FLASH�ж�ȡһ���������ݵ�ָ����ַ
- ��������: 2019.04.08
- ��    ��:�¿�ΰ
----------------------------------------------------------------------------------------------------*/
void spi_flash_Read_Sector(uint32_t	ReadAddR, uint8_t *buff)
{
	uint8_t i=0,j=FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE;
  uint16_t k=0;
	for(i=0;i<j;i++)
	{
		spi_flash_read_page(ReadAddR,rxbuff);
		for(k=0;k<FLASH_PAGE_SIZE;k++)
		{
			*buff=rxbuff[4+k];
			buff++;
		}
		ReadAddR+=FLASH_PAGE_SIZE;
	}
}
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_write_Sector
- ��������: ��SPI FLASHָ������д������
- ��������: 2019.04.08
- ��    ��:�¿�ΰ
---------------------------------------------------------------*/
void spi_flash_write_Sector(uint32_t WriteAddR, uint8_t *buff)
{
	uint8_t i=0,j=FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE;
	for(i=0;i<j;i++)
	{
		spi_flash_write_enable();
		spi_flash_write_page(WriteAddR,buff);
		while(spi_flash_wait_till_ready());
		WriteAddR+=FLASH_PAGE_SIZE;
		buff+=FLASH_PAGE_SIZE;	
	}
}
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_write
- ��������: ��ָ���ĵ�ַ��ʼд��ָ�����ȵ�����
- ��������: 2019.04.08
- ��    ��:�¿�ΰ
----------------------------------------------------------------------------------------------------*/

static volatile uint8_t FlashBuf[FLASH_SECTOR_SIZE];
void spi_flash_write(uint32_t	WriteAddR, uint8_t *buff,uint16_t WriteLength)
{
	uint32_t sectorpos;//������ַ
	uint16_t sectoroff;//����ƫ�Ƶ�ַ
	uint16_t sectorremain;//����ʣ���ַ	   
 	uint16_t i,j;
	sectorpos=WriteAddR/FLASH_SECTOR_SIZE;
	sectoroff=WriteAddR%FLASH_SECTOR_SIZE;
	sectorremain=FLASH_SECTOR_SIZE-sectoroff;	
	if(WriteLength<=sectorremain)  sectorremain=WriteLength; 	
	while(1)
	{   
		spi_flash_Read_Sector(sectorpos*FLASH_SECTOR_SIZE,FlashBuf);        
		for(i=0;i<sectorremain;i++)
		{
			FlashBuf[sectoroff+i]=buff[i];	  
		}		
		spi_flash_write_enable();
		spi_flash_sector_erase(sectorpos);
		while(spi_flash_wait_till_ready());		
		spi_flash_write_Sector(sectorpos*FLASH_SECTOR_SIZE,FlashBuf);		
		if(WriteLength==sectorremain) break;
		else
		{
			sectorpos++;
			sectoroff=0;
			buff+=sectorremain;
            WriteAddR+=sectorremain;
			WriteLength-=sectorremain;
			if(WriteLength>FLASH_SECTOR_SIZE) sectorremain=FLASH_SECTOR_SIZE;
			else sectorremain=WriteLength;
		}
		
	}
}
#else  //ʹ���Ϲ�����flash
/******2020.10.14���� ���½ӿ�Ŀǰ�������Ϲ�����flash ���½ӿ�Ŀ����Ϊʡ����д��������Ҫ�Ĵ�Ļ�����********/
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_Read
- ��������: ��ָ���ĵ�ַ��ʼ��ȡָ�����ȵ�����
- ��������: 2020.10.14
----------------------------------------------------------------------------------------------------*/
void spi_flash_read(uint32_t	ReadAddR, uint8_t *buff,uint16_t  ReadLength)
{
	uint32_t PagePos=ReadAddR/FLASH_PAGE_SIZE;//ҳ��ַ
	uint16_t PageOff=ReadAddR%FLASH_PAGE_SIZE;//ҳƫ�Ƶ�ַ
	uint16_t i=0,PageMain=FLASH_PAGE_SIZE-PageOff;//ҳʣ���ַ
	if(ReadLength<=PageMain)  PageMain=ReadLength; 
	while(1)
	{   	
        spi_flash_read_page(PagePos*FLASH_PAGE_SIZE,rxbuff);
		for(i=0;i<PageMain;i++)
		{
			buff[i]=rxbuff[PageOff+i+4];	  
		}
        if(ReadLength==PageMain) break;
		else
		{
			PagePos++;
			PageOff=0;
			buff+=PageMain;
            ReadAddR+=PageMain;
			ReadLength-=PageMain;
			if(ReadLength>FLASH_PAGE_SIZE) PageMain=FLASH_PAGE_SIZE;
			else PageMain=ReadLength;
		}
  }
}
/* ---------------------------------------------------------------------------------------------------
- ��������: spi_flash_write
- ��������: ��ָ���ĵ�ַ��ʼд��ָ�����ȵ�����
- ��������:2020.10.14
 ----------------------------------------------------------------------------------------------------*/
void	spi_flash_write(uint32_t	WriteAddR, uint8_t *buff,uint16_t WriteLength)
{
	uint8_t FlashBuf[FLASH_PAGE_SIZE ];
	uint32_t PagePos=WriteAddR/FLASH_PAGE_SIZE;//ҳ��ַ
	uint16_t PageOff=WriteAddR%FLASH_PAGE_SIZE;//ҳƫ�Ƶ�ַ
	uint16_t i=0,PageMain=FLASH_PAGE_SIZE-PageOff;//ҳʣ���ַ
  if(WriteLength<=PageMain) PageMain=WriteLength; 
	while(1)
	{   	
	  spi_flash_read_page(PagePos*FLASH_PAGE_SIZE,rxbuff);
		for(i=0;i<FLASH_PAGE_SIZE;i++) FlashBuf[i]=rxbuff[i+4];
		for(i=0;i<PageMain;i++) FlashBuf[PageOff+i]=buff[i];	  	
		spi_flash_write_enable();
		spi_flash_page_erase(PagePos);
		while(spi_flash_wait_till_ready());	
		spi_flash_write_enable();
		spi_flash_write_page(PagePos*FLASH_PAGE_SIZE,FlashBuf );
		while(spi_flash_wait_till_ready());			
		if(WriteLength == PageMain) break;
		else
		{
			PagePos++;
			PageOff=0;
			buff+=PageMain;
			WriteAddR += PageMain;
			WriteLength-=PageMain;
			if(WriteLength>FLASH_PAGE_SIZE) PageMain=FLASH_PAGE_SIZE;
			else PageMain=WriteLength;
		}
  }
}

#endif

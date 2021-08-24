#include "gt20l16s1y.h"

/* ---------------------------------------------------------------------------------------------------
- ��������: read_ASCII_8x16
- ��������: ��ȡ�ֿ�оƬ��һ��ASCII�ַ��� 8x16 �������
- �������: ASCII�룬lattice_buff:������뻺����
----------------------------------------------------------------------------------------------------*/
void read_ASCII_8x16(uint8_t ascii_code, uint8_t *lattice_buff)
{
	uint32_t base_addr = 0x3B7C0;	//8x16�������ʼ��ַ
	uint32_t addr; 					//ASCII�ַ�������оƬ�е�λ��
	
	if ((ascii_code >= 0x20) && (ascii_code <= 0x7E)) {
		addr = (ascii_code - 0x20) * 16 + base_addr;
	}
	SPI_CS_LOW;
	spi_write(0x0B);
	spi_write((addr >> 16) & 0xFF);
	spi_write((addr >> 8) & 0xFF);
	spi_write((addr >> 0) & 0xFF);
	spi_write(0xFF);
	for (int i = 0; i < 16; i++) {
		*(lattice_buff + i) = spi_read();
	}
	SPI_CS_HIGHT;
}

/* ---------------------------------------------------------------------------------------------------
- ��������: read_gb2312_16x16
- ��������: ��ȡ�ֿ�оƬ��һ�����꺺�ֵ� 16x16 �������
- �������: gb2312_code�룬lattice_buff:������뻺����
----------------------------------------------------------------------------------------------------*/
void read_gb2312_16x16(uint32_t gb2312_code, uint8_t *lattice_buff)
{
	uint32_t base_addr = 0;			//16x16�������ʼ��ַ
	uint8_t msb, lsb;				//msb:��������߰�λ lsb:��������Ͱ�λ
	uint32_t addr; 					//���ֵ�����оƬ�е�λ��
	
	msb = (gb2312_code >> 8) & 0xFF;
	lsb = (gb2312_code >> 0) & 0xFF;

	if(msb == 0xA9 && lsb >= 0xA1) {
		addr = (282 + (lsb - 0xA1)) * 32 + base_addr;
	}
	else if(msb >= 0xA1 && msb <= 0xA3 && lsb >= 0xA1) {
		addr = (msb - 0xA1) * 94 + (lsb - 0xA1) * 32 + base_addr;
	}
	else if(msb >= 0xB0 && msb <= 0xF7 && lsb >= 0xA1) {
		addr = ((msb - 0xB0) * 94 + (lsb - 0xA1) + 846) * 32 + base_addr;
	}
	
	SPI_CS_LOW;
	spi_write(0x0B);
	spi_write((addr >> 16) & 0xFF);
	spi_write((addr >> 8) & 0xFF);
	spi_write((addr >> 0) & 0xFF);
	spi_write(0xFF);
	for (int i = 0; i < 32; i++) {
		*(lattice_buff + i) = spi_read();
	}
	SPI_CS_HIGHT;
}


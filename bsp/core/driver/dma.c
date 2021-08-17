#include "dma.h"

/* ---------------------------------------------------------------------------------------------------
- ��������: dma_send
- ��������: Uart��DMA��ʽ����һ������Ϊlength��uint8_t���͵�����  --ע��DMA��ֻ�ܷ���SHRAM0��0x10010000 ~ 0x1001FFFF��
----------------------------------------------------------------------------------------------------*/
static uint8_t __attribute__((aligned(4))) uart_tx_buf[256]={0};

void dma_enable(E_DMA_CH ch)
{
    writeReg32(DMAS_EN, ch);
}

void dma_send(uint8_t ch, uint8_t *dma_buf, uint32_t len)
{

    uint32_t dma_int_status = 0;
    //- TX Channel
    writeReg32(DMAS_CHx_SAR(ch), (uint32_t)dma_buf);   //����ͨ����ʼ��ַ
    writeReg32(DMAS_CHx_DAR(ch), (ch * 0x1000 + 0x40010000));  //���͵�ַдUART_THR
    writeReg32(DMAS_CHx_CTL1(ch), ((2 << 8)));         //ͨ�����ȼ�2 ���߿��8bit
    writeReg32(DMAS_CHx_CTL0(ch), len);
    dma_enable(ch);
    do	{
        readReg32(DMAS_INT_RAW, dma_int_status);
    }while( (dma_int_status & (1+ch)) != (1 + ch));

    writeReg32(DMAS_INT_RAW, (1+ch));
    writeReg32(DMAS_CLR, ch);
}

void dma_recive(uint8_t ch, uint8_t *dma_buf, uint32_t len)
{
    uint32_t dma_int_status = 0;
    //- RX Channel
    writeReg32(DMAS_CHx_SAR(ch), (ch * 0x1000 + 0x40010000);   //����ͨ����ʼ��ַ
    writeReg32(DMAS_CHx_DAR(ch), (uint32_t)dma_buf));  //���յ�ַдUART_THR
    writeReg32(DMAS_CHx_CTL1(ch), ((2 << 8)));         //ͨ�����ȼ�2 ���߿��8bit
    writeReg32(DMAS_CHx_CTL0(ch), len);
    dma_enable(ch);
    do	{
        readReg32(DMAS_INT_RAW, dma_int_status);
    }while((dma_int_status & (1 + ch)) != (1 + ch));

    writeReg32(DMAS_INT_RAW, (1+ch));
    writeReg32(DMAS_CLR, ch);
}


#ifndef __BSP_SMARTSTAR_H__
#define __BSP_SMARTSTAR_H__
#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
* 函 数 名  : hi6551_reg_write
* 功能描述  :写smartstar的寄存器
* 输入参数  :addr:寄存器地址，value：写入的值
* 输出参数  :无
* 返回值：   无
*
*****************************************************************************/
void hi6551_reg_write( u32 addr, u8 value);
/*****************************************************************************
* 函 数 名  : hi6551_reg_read
* 功能描述  :读smartstar的寄存器
* 输入参数  :addr:寄存器地址
* 输出参数  :pValue：读出的值
* 返回值：   无
*
*****************************************************************************/
void hi6551_reg_read( u32 addr, u8 *pValue);
/*****************************************************************************
* 函 数 名  : hi6551_reg_write_mask
* 功能描述  :操作smartstar的寄存器中的某几位
* 输入参数  :addr:寄存器地址，value：写入的值,mask：要操作的bit
* 输出参数  :无
* 返回值：   无
*
*****************************************************************************/
void hi6551_reg_write_mask(u32 addr, u8 value, u8 mask);
/*****************************************************************************
* 函 数 名  : hi6551_reg_read_test
* 功能描述  :读smartstar的寄存器测试接口,输入要读的寄存器地址，返回读出的数据。
* 输入参数  :addr:寄存器地址
* 输出参数  :无
* 返回值：   读出的寄存器值
*
*****************************************************************************/
int  hi6551_reg_read_test(u32 addr);
#ifdef __cplusplus
}
#endif

#endif
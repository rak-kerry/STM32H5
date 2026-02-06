
#include "main.h"
#include <stdio.h>
#include "time.h"

#define NO_ERROR                0x00
#define ACK_ERROR               0x01
#define CHECKSUM_ERROR          0x02
#define NULL_ERROR              0x03
#define FORMAT_ERROR            0x04
#define SETTIME_ERROR           0x05

#define EIO 5 /* I/O error */
#define EINVAL 22 /* Invalid argument */

extern I2C_HandleTypeDef hi2c1;

// HAL 要使用 8-bit address，所以左移1位
#define RX8010_I2C_SLAVE_ADDR0  (0x32 << 1)  

/* Default values for reseting the module */
#define RX8010_ADDR17_DEF_VAL 0xD8
#define RX8010_ADDR30_DEF_VAL 0x00
#define RX8010_ADDR31_DEF_VAL 0x08
#define RX8010_ADDR31_DEF_VAL 0x08
#define RX8010_IRQ_DEF_VAL    0x04
#define RX8010_CTRL_DEF_VAL   0x04

/*
 * RTC register addresses
 */
#define RX8010_SEC     0x10
#define RX8010_MIN     0x11
#define RX8010_HOUR    0x12
#define RX8010_WDAY    0x13
#define RX8010_MDAY    0x14
#define RX8010_MONTH   0x15
#define RX8010_YEAR    0x16
#define RX8010_RESV17  0x17
#define RX8010_ALMIN   0x18
#define RX8010_ALHOUR  0x19
#define RX8010_ALWDAY  0x1A
#define RX8010_TCOUNT0 0x1B
#define RX8010_TCOUNT1 0x1C
#define RX8010_EXT     0x1D
#define RX8010_FLAG    0x1E
#define RX8010_CTRL    0x1F
/* 0x20 to 0x2F are user registers */
#define RX8010_RESV30  0x30
#define RX8010_RESV31  0x32
#define RX8010_IRQ     0x32

#define BIT(n) (1U << (n))
#define RX8010_EXT_WADA  BIT(3)
#define RX8010_FLAG_VLF  BIT(1)
#define RX8010_FLAG_AF   BIT(3)
#define RX8010_FLAG_TF   BIT(4)
#define RX8010_FLAG_UF   BIT(5)
#define RX8010_CTRL_AIE  BIT(3)
#define RX8010_CTRL_UIE  BIT(5)
#define RX8010_CTRL_STOP BIT(6)
#define RX8010_CTRL_TEST BIT(7)

//-----------------------------------------------------

unsigned char digit_to_bcd(int digit) {
    return ((digit / 10) << 4) | (digit % 10);
}

int bcd_to_digit(unsigned char bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

uint8_t rx8010sj_rtc_read(uint8_t reg_addr)
{
    uint8_t value = 0;

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, RX8010_I2C_SLAVE_ADDR0, &reg_addr, 1, HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        printf("%s Error - status %d \r\n", __func__, status) ;
        return 0xFF;
    }

    status = HAL_I2C_Master_Receive(&hi2c1, RX8010_I2C_SLAVE_ADDR0, &value, 1, HAL_MAX_DELAY);
    if (status != HAL_OK)
    {
        printf("%s Error - status %d \r\n", __func__, status) ;
        return 0xFF;
    }
    return value;
}

int rx8010sj_rtc_write(uint8_t reg_addr, uint8_t value)
{
    uint8_t buffer[2];
    buffer[0] = reg_addr;
    buffer[1] = value;

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, RX8010_I2C_SLAVE_ADDR0, buffer, 2, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        printf("%s Error - status %d \n\r", __func__, status) ;
        return -1;
    }
    return 0;
}

int validate_time(struct tm *tm)
{
	if ((tm->tm_year < 2000) || (tm->tm_year > 2099))
		return -EINVAL;

	if ((tm->tm_mon < 1) || (tm->tm_mon > 12))
		return -EINVAL;

	if ((tm->tm_mday < 1) || (tm->tm_mday > 31))
		return -EINVAL;

	if ((tm->tm_wday < 0) || (tm->tm_wday > 6))
		return -EINVAL;

	if ((tm->tm_hour < 0) || (tm->tm_hour > 23))
		return -EINVAL;

	if ((tm->tm_min < 0) || (tm->tm_min > 59))
		return -EINVAL;

	if ((tm->tm_sec < 0) || (tm->tm_sec > 59))
		return -EINVAL;

	return 0;
}

int RX8010_init(void)
{
    int ret = NO_ERROR;
   	uint8_t ctrl[2];
   	int need_clear = 0;

    printf("RX8010SJ_init \r\n");

   	/* Initialize reserved registers as specified in datasheet */
   	ret = rx8010sj_rtc_write(RX8010_RESV17, RX8010_ADDR17_DEF_VAL);
   	if (ret < 0) goto error;

   	ret = rx8010sj_rtc_write(RX8010_RESV30, RX8010_ADDR30_DEF_VAL);
   	if (ret < 0) goto error;

   	ret = rx8010sj_rtc_write(RX8010_RESV31, RX8010_ADDR31_DEF_VAL);
   	if (ret < 0) goto error;

   	ret = rx8010sj_rtc_write(RX8010_IRQ, 0x00);
   	if (ret < 0) goto error;

   	for (int i = 0; i < 2; i++)
   	{
   		ret = rx8010sj_rtc_read(RX8010_FLAG + i);
   		if (ret < 0) goto error;

   		ctrl[i] = ret;
   	}

   	if (ctrl[0] & RX8010_FLAG_VLF)
   		printf("RTC low voltage detected\r\n");

   	if (ctrl[0] & RX8010_FLAG_AF) {
    	printf("Alarm was detected\r\n");
    	need_clear = 1;
    }

    if (ctrl[0] & RX8010_FLAG_TF)   need_clear = 1;

    if (ctrl[0] & RX8010_FLAG_UF)  	need_clear = 1;

    if (need_clear)
    {
    	ctrl[0] &= ~(RX8010_FLAG_AF | RX8010_FLAG_TF | RX8010_FLAG_UF);
    	ret = rx8010sj_rtc_write(RX8010_FLAG, ctrl[0]);
    	if (ret < 0) goto error;
    }
    return 0;
error:
	printf("%s : Error rtc init.\r\n", __func__);
	return -1;
}


/* Get the current time from the RTC */
int rx8010sj_rtc_get(struct tm* tmp)
{
	uint8_t date[7];
	int flagreg;
	int ret;

	flagreg = rx8010sj_rtc_read(RX8010_FLAG);
	if (flagreg < 0) {
		printf("Error reading from RTC. err: %d\r\n", flagreg);
	}

	if (flagreg & RX8010_FLAG_VLF) {
		printf("RTC low voltage detected\r\n");
	}

	for (int i = 0; i < 7; i++) {
		ret = rx8010sj_rtc_read(RX8010_SEC + i);
		if (ret < 0) {
			printf("Error reading from RTC. err: %d\r\n", ret);
			return -1;
		}
		date[i] = ret;
	}
	tmp->tm_sec = bcd_to_digit(date[RX8010_SEC - RX8010_SEC] & 0x7f);
	tmp->tm_min = bcd_to_digit(date[RX8010_MIN - RX8010_SEC] & 0x7f);
	tmp->tm_hour = bcd_to_digit(date[RX8010_HOUR - RX8010_SEC] & 0x3f);
	tmp->tm_mday = bcd_to_digit(date[RX8010_MDAY - RX8010_SEC] & 0x3f);
	tmp->tm_mon = bcd_to_digit(date[RX8010_MONTH - RX8010_SEC] & 0x1f);
	tmp->tm_year = bcd_to_digit(date[RX8010_YEAR - RX8010_SEC]) + 2000;
	tmp->tm_wday = 0;
	tmp->tm_yday = 0;
	tmp->tm_isdst = 0;
	return 0;
}


/* Set the RTC */
int rx8010sj_rtc_set(struct tm *tm)
{
	uint8_t date[7];
	int ctrl, flagreg;
	int ret;

	ret = validate_time(tm);
	if (ret < 0)
		return -EINVAL;

	/* set STOP bit before changing clock/calendar */
	ctrl = rx8010sj_rtc_read(RX8010_CTRL);
	if (ctrl < 0)
		return ctrl;
	ret = rx8010sj_rtc_write(RX8010_CTRL, ctrl | RX8010_CTRL_STOP);
	if (ret < 0)
		return ret;

	date[RX8010_SEC - RX8010_SEC] = digit_to_bcd(tm->tm_sec);
	date[RX8010_MIN - RX8010_SEC] = digit_to_bcd(tm->tm_min);
	date[RX8010_HOUR - RX8010_SEC] = digit_to_bcd(tm->tm_hour);
	date[RX8010_WDAY - RX8010_SEC] = digit_to_bcd(tm->tm_wday);
	date[RX8010_MDAY - RX8010_SEC] = digit_to_bcd(tm->tm_mday);
	date[RX8010_MONTH - RX8010_SEC] = digit_to_bcd(tm->tm_mon);
	date[RX8010_YEAR - RX8010_SEC] = digit_to_bcd(tm->tm_year - 2000);

	for (int i = 0; i < 7; i++)
	{
		ret = rx8010sj_rtc_write(RX8010_SEC + i, date[i]);
		if (ret < 0)
		{
			printf("Error writing to RTC. err: %d\r\n", ret);
			return -EIO;
		}
	}

	/* clear STOP bit after changing clock/calendar */
	ctrl = rx8010sj_rtc_read(RX8010_CTRL);
	if (ctrl < 0)
		return ctrl;

	ret = rx8010sj_rtc_write(RX8010_CTRL, ctrl & ~RX8010_CTRL_STOP);
	if (ret < 0)
		return ret;

	flagreg = rx8010sj_rtc_read(RX8010_FLAG);
	if (flagreg < 0)
		return flagreg;

	if (flagreg & RX8010_FLAG_VLF)
		ret = rx8010sj_rtc_write(RX8010_FLAG, flagreg & ~RX8010_FLAG_VLF);

	return 0;
}

#include "return_codes.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define EXP_H 5
#define EXP_F 8
#define MANTIS_H 10
#define MANTIS_F 23
#define EXP_H_DIV 15
#define EXP_F_DIV 127
#define EXP_H_M 16
#define EXP_F_M 128
#define MANTIS_H_MAX 4092
#define MANTIS_F_MAX 16777214

struct Number
{
	uint8_t sign;
	uint8_t zero;
	uint8_t cnt_one;
	uint8_t first_one;
	uint8_t mantis_const;
	int16_t exponent;
	uint64_t mantisa;
} typedef Number;

void cheak_one(Number *num1_struct, Number const *num2_struct)
{
	num1_struct->cnt_one += num2_struct->cnt_one;
	num1_struct->zero |= num2_struct->zero;
	num1_struct->first_one |= num2_struct->first_one;
}

void overfllow_add_sub(Number *num1_struct)
{
	uint64_t local_mantisa = num1_struct->mantisa;
	int16_t local_exponent = num1_struct->exponent;
	uint8_t local_cnt_one = num1_struct->cnt_one;
	uint8_t local_mantis_const = num1_struct->mantis_const;
	uint8_t local_first_one = num1_struct->first_one;
	while (local_mantisa >= (1 << (local_mantis_const + 1)) && local_mantisa != 0)
	{
		local_exponent++;
		local_cnt_one += (local_mantisa & 1);
		local_first_one = local_mantisa & 1;
		local_mantisa >>= 1;
	}
	while (local_mantisa < (1 << local_mantis_const) && local_mantisa != 0)
	{
		local_exponent--;
		local_mantisa <<= 1;
	}
	num1_struct->exponent = local_exponent;
	num1_struct->cnt_one = local_cnt_one;
	num1_struct->first_one = local_first_one;
	num1_struct->mantisa = local_mantisa;
}

void overfllow(Number *num1_struct)
{
	uint64_t local_mantisa = num1_struct->mantisa;
	int16_t local_exponent = num1_struct->exponent;
	uint8_t local_cnt_one = num1_struct->cnt_one;
	uint8_t local_mantis_const = num1_struct->mantis_const;
	uint8_t local_first_one = num1_struct->first_one;
	local_exponent += (local_mantisa > (1 << (local_mantis_const + 1)));
	while (local_mantisa > (1 << (local_mantis_const + 1)) && local_mantisa != 0)
	{
		local_cnt_one += (local_mantisa & 1);
		local_first_one = local_mantisa & 1;
		local_mantisa >>= 1;
	}
	local_exponent -= (local_mantisa < (1 << (local_mantis_const)) && local_mantisa != 0);
	while (local_mantisa < (1 << (local_mantis_const)) && local_mantisa != 0)
	{
		local_mantisa <<= 1;
	}
	num1_struct->exponent = local_exponent;
	num1_struct->cnt_one = local_cnt_one;
	num1_struct->first_one = local_first_one;
	num1_struct->mantisa = local_mantisa;
}

void rounding(Number *num1_struct, uint8_t format)
{
	uint64_t local_mantisa = num1_struct->mantisa;
	uint8_t local_cnt_one = num1_struct->cnt_one;
	if (!num1_struct->sign)
	{
		if (format == 2 && (local_cnt_one != 0 || num1_struct->zero))
		{
			local_mantisa++;
		}
	}
	else if (format == 3 && (local_cnt_one != 0 || num1_struct->zero))
	{
		local_mantisa++;
	}
	if (format == 1 && ((num1_struct->first_one && (local_cnt_one > 1 || local_mantisa & 1))))
	{
		local_mantisa++;
	}
	num1_struct->mantisa = local_mantisa;
	num1_struct->cnt_one = local_cnt_one;
}

void step(Number *num1_struct, uint16_t move)
{
	uint64_t local_mantisa = num1_struct->mantisa;
	while (move != 0)
	{
		move--;
		num1_struct->cnt_one += (local_mantisa & 1);
		num1_struct->first_one = local_mantisa & 1;
		local_mantisa >>= 1;
	}
	num1_struct->mantisa = local_mantisa;
}

uint8_t check_nan(Number const *num1_struct)
{
	return (num1_struct->exponent == (num1_struct->mantis_const == 10 ? EXP_H_M : EXP_F_M) && num1_struct->mantisa != 0);
}

uint8_t len_int(uint64_t number)
{
	uint8_t len = 0;
	while (number != 0)
	{
		number >>= 4;
		len++;
	}
	return len;
}

void printStruct(Number *num1_struct, uint8_t format, char c)
{
	rounding(num1_struct, format);
	uint64_t local_mantisa = num1_struct->mantisa;
	int16_t local_exponent = num1_struct->exponent;
	uint8_t local_mantis_const = num1_struct->mantis_const;
	uint8_t local_sign = num1_struct->sign;
	uint8_t max_exp = local_mantis_const == 10 ? EXP_H_M : EXP_F_M;
	local_mantisa -= (local_mantisa != 0) ? (1 << local_mantis_const) : 0;
	local_mantisa <<= (c == 'f' ? 1 : 2);
	if (local_exponent > max_exp)
	{
		if ((format == 3 && !local_sign) || (format == 2 && local_sign) || format == 0)
		{
			local_exponent = max_exp - 1;
			local_mantisa = local_mantis_const == 10 ? MANTIS_H_MAX : MANTIS_F_MAX;
		}
		else if (format == 3 && (local_sign))
		{
			local_mantisa = 0;
		}
	}
	if (local_mantisa == 0 && local_exponent >= max_exp)
	{
		printf(local_sign ? "-inf" : "inf");
		return;
	}
	if (local_exponent < (local_mantis_const == 10 ? -24 : -149))
	{
		local_mantisa = 0;
		if ((!local_sign && format == 2) || (local_sign && (format == 3)))
		{
			local_exponent = local_mantis_const == 10 ? -24 : -149;
		}
		else
		{
			local_exponent = 0;
		}
	}
	if (local_exponent == (local_mantis_const == 10 ? -EXP_H_DIV : -EXP_F_DIV) && local_mantisa == 0)
	{
		local_mantisa = 0;
		local_exponent = 0;
	}
	printf(!local_sign ? "0x" : "-0x");
	printf((local_mantisa == 0 && local_exponent == 0) ? "0." : "1.");
	for (uint8_t i = 0; i < (c == 'f' ? 6 : 3) - len_int(local_mantisa) - (local_mantisa == 0); i++)
	{
		putchar('0');
	}
	printf("%" PRIx64, local_mantisa);
	putchar('p');
	if (local_exponent >= 0)
	{
		putchar('+');
	}
	printf("%d", local_exponent);
}

uint8_t checkZero(Number const *num1_struct)
{
	return ((num1_struct->exponent == 0) && (num1_struct->mantisa == 0));
}

void exponentReduction(Number *num1_struct, Number *num2_struct)
{
	if (num1_struct->exponent < num2_struct->exponent)
	{
		Number *num = num1_struct;
		num1_struct = num2_struct;
		num2_struct = num;
	}
	uint8_t delta = num1_struct->exponent - num2_struct->exponent;
	if (delta <= 40)
	{
		num1_struct->mantisa <<= (delta);
		num1_struct->exponent = num2_struct->exponent;
	}
	else
	{
		num1_struct->zero = (!checkZero(num1_struct) && !checkZero(num2_struct));
		step(num2_struct, delta);
		num2_struct->exponent = num1_struct->exponent;
	}
}

uint8_t checkInf(Number const *num1_struct, char c)
{
	return (num1_struct->exponent == (c == 'h' ? EXP_H_M : EXP_F_M) && num1_struct->mantisa == 0);
}

Number buildStruct(uint64_t num1, char c)
{
	uint8_t exp_cnst = EXP_H;
	uint8_t exp_divide = EXP_H_DIV;
	uint8_t mantis_const = MANTIS_H;
	if (c == 'f')
	{
		mantis_const = MANTIS_F;
		exp_cnst = EXP_F;
		exp_divide = EXP_F_DIV;
	}
	Number num_struct;
	num_struct.mantis_const = mantis_const;
	num_struct.mantisa = num1 % (1 << mantis_const);
	num1 /= (1 << mantis_const);
	num_struct.exponent = (num1 % (1 << exp_cnst));
	num_struct.exponent -= ((num_struct.exponent == 0 && num_struct.mantisa == 0) ? 0 : exp_divide);
	num1 /= (1 << exp_cnst);
	num_struct.sign = num1 & 1;
	num_struct.cnt_one = num_struct.first_one = 0;
	if (num_struct.exponent != -exp_divide && num_struct.mantisa != 0)
	{
		num_struct.mantisa = num_struct.mantisa + (1 << mantis_const);
	}
	if (num_struct.exponent == -exp_divide && num_struct.mantisa != 0 && num_struct.exponent != exp_divide)
	{
		uint8_t delta = mantis_const - len_int(num_struct.mantisa);
		num_struct.mantisa = num_struct.mantisa << delta;
		num_struct.exponent = 1 - exp_divide - delta;
	}
	num_struct.zero = 0;
	overfllow_add_sub(&num_struct);
	return num_struct;
}

uint8_t check_nan_inf(Number const *num1_struct)
{
	if (num1_struct->exponent == (num1_struct->mantis_const == 10 ? EXP_H_M : EXP_F_M))
	{
		printf((num1_struct->mantisa == 0) ? (num1_struct->sign ? "-inf" : "inf") : "nan");
		return 1;
	}
	return 0;
}

uint8_t addStruct(Number *num1_struct, Number *num2_struct, char c)
{
	if (checkInf(num1_struct, c) && checkInf(num2_struct, c))
	{
		printf("nan");
		return 1;
	}
	exponentReduction(num1_struct, num2_struct);
	num1_struct->mantisa += num2_struct->mantisa;
	overfllow_add_sub(num1_struct);
	return 0;
}

uint8_t multiplyStruct(Number *num1_struct, Number const *num2_struct, char c)
{
	if (checkZero(num1_struct) && checkInf(num2_struct, c) || (checkZero(num2_struct) && checkInf(num1_struct, c)))
	{
		printf("nan");
		return 1;
	}
	if (checkZero(num1_struct) || checkZero(num2_struct))
	{
		num1_struct->sign ^= num2_struct->sign;
		num1_struct->exponent = num1_struct->mantisa = 0;
		return 0;
	}
	int16_t local_exponent1 = num1_struct->exponent;
	num1_struct->sign = num1_struct->sign != num2_struct->sign;
	local_exponent1 += num2_struct->exponent;
	num1_struct->mantisa *= num2_struct->mantisa;
	if ((num1_struct->mantisa > 2 && ((num1_struct->mantisa >> (((num1_struct->mantis_const + 1) << 1) - 1)))))
	{
		local_exponent1++;
	}
	num1_struct->exponent = local_exponent1 - (!checkZero(num1_struct) && !checkInf(num1_struct, c));
	overfllow(num1_struct);
	return 0;
}

uint8_t substractStruct(Number *num1_struct, Number *num2_struct, uint8_t round)
{
	if (check_nan_inf(num1_struct) || check_nan_inf(num2_struct))
	{
		return 1;
	}
	exponentReduction(num1_struct, num2_struct);
	uint64_t local_mantisa1 = num1_struct->mantisa;
	uint64_t local_mantisa2 = num2_struct->mantisa;
	if (local_mantisa1 < local_mantisa2)
	{
		num1_struct->sign = 1;
		local_mantisa1 = local_mantisa2 - local_mantisa1;
	}
	else
	{
		local_mantisa1 -= local_mantisa2;
	}
	cheak_one(num1_struct, num2_struct);
	local_mantisa1 -= (num1_struct->zero && round != 1);
	num1_struct->mantisa = local_mantisa1;
	num2_struct->mantisa = local_mantisa2;
	overfllow_add_sub(num1_struct);
	return 0;
}

uint8_t divideStruct(Number *num1_struct, Number const *num2_struct, char c)
{
	uint8_t local_sign = num1_struct->sign;
	if (checkInf(num1_struct, c) && checkInf(num2_struct, c) || (checkZero(num1_struct) && checkZero(num2_struct)))
	{
		printf("nan");
		return 1;
	}
	if (checkZero(num1_struct) || checkInf(num2_struct, c))
	{
		num1_struct->sign ^= num2_struct->sign;
		return 0;
	}
	if (checkZero(num2_struct) || checkInf(num1_struct, c))
	{
		printf(local_sign != num2_struct->sign ? "-inf" : "inf");
		return 1;
	}
	uint64_t local_mantisa1 = num1_struct->mantisa;
	local_sign = local_sign != num2_struct->sign;
	num1_struct->exponent -= num2_struct->exponent;
	local_mantisa1 <<= 40;
	local_mantisa1 /= num2_struct->mantisa;
	num1_struct->exponent -= (local_mantisa1 >> 40 < 1);
	num1_struct->mantisa = local_mantisa1;
	num1_struct->sign = local_sign;
	num1_struct->exponent -= (!checkZero(num1_struct) && !check_nan(num1_struct) && !checkInf(num1_struct, c));
	overfllow(num1_struct);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc != 4 && argc != 6)
	{
		fprintf(stderr, "no arguments");
		return ERROR_ARGUMENTS_INVALID;
	}
	char c;
	uint64_t num1, num2;
	int8_t format;
	char operation;
	sscanf(argv[1], "%c", &c);
	sscanf(argv[2], "%" SCNi8, &format);
	sscanf(argv[3], "%" SCNi64, &num1);
	if (c != 'f' && c != 'h' || strlen(argv[1]) > 1)
	{
		fprintf(stderr, "illegal format numbers");
		return ERROR_ARGUMENTS_INVALID;
	}
	if (format < 0 || format > 3)
	{
		fprintf(stderr, "illegal format rounding numbers");
		return ERROR_ARGUMENTS_INVALID;
	}
	Number num1_struct = buildStruct(num1, c);
	if (check_nan(&num1_struct))
	{
		printf("nan");
		return SUCCESS;
	}
	if (argc == 4)
	{
		printStruct(&num1_struct, format, c);
		return SUCCESS;
	}
	sscanf(argv[4], "%c", &operation);
	if (operation == 'x' || operation == 'M')
	{
		operation = '*';
	}
	sscanf(argv[5], "%" SCNx64, &num2);
	Number num2_struct = buildStruct(num2, c);
	if (check_nan(&num2_struct))
	{
		printf("nan");
		return SUCCESS;
	}
	uint8_t skip_print_stuct = 0;
	switch (operation)
	{
	case '+':
		if (num1_struct.sign == num2_struct.sign)
		{
			skip_print_stuct = addStruct(&num1_struct, &num2_struct, c);
		}
		else
		{
			if (num1_struct.sign)
			{
				num1_struct.sign = 0;
				skip_print_stuct = substractStruct(&num2_struct, &num1_struct, format);
				num1_struct = num2_struct;
				break;
			}
			num2_struct.sign = 0;
			skip_print_stuct = substractStruct(&num1_struct, &num2_struct, format);
		}
		break;
	case '-':
		if (!num1_struct.sign && !num2_struct.sign)
		{
			skip_print_stuct = substractStruct(&num1_struct, &num2_struct, format);
			break;
		}
		else if (num1_struct.sign == num2_struct.sign == 1)
		{
			num1_struct.sign = num2_struct.sign = 0;
			skip_print_stuct = substractStruct(&num2_struct, &num1_struct, format);
			num1_struct = num2_struct;
			break;
		}
		num2_struct.sign = num1_struct.sign;
		skip_print_stuct = addStruct(&num1_struct, &num2_struct, c);
		break;
	case '*':
		skip_print_stuct = multiplyStruct(&num1_struct, &num2_struct, c);
		break;
	case '/':
		skip_print_stuct = divideStruct(&num1_struct, &num2_struct, c);
		break;
	default:
		fprintf(stderr, "illegal operation format numbers");
		return ERROR_ARGUMENTS_INVALID;
	}
	cheak_one(&num1_struct, &num2_struct);
	if (!skip_print_stuct)
		printStruct(&num1_struct, format, c);
	return SUCCESS;
}

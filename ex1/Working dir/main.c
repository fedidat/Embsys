void writeHexDigit(char d, int isSecondDigit)
{
	unsigned int symbol, 
		LED_A = 1, LED_B = 2, LED_C = 4, LED_D = 8,
		LED_E = 16, LED_F = 32, LED_G = 64;
	switch(d)
	{
	case 0x0:
	default:
		symbol = LED_A | LED_B | LED_C | LED_D | LED_E | LED_F;
		break;
	case 0x1:
		symbol = LED_B | LED_C;
		break;
	case 0x2:
		symbol = LED_A | LED_B | LED_D | LED_E | LED_G;
		break;
	case 0x3:
		symbol = LED_A | LED_B | LED_C | LED_D | LED_G;
		break;
	case 0x4:
		symbol = LED_B | LED_C | LED_F | LED_G;
		break;
	case 0x5:
		symbol = LED_A | LED_C | LED_D | LED_F | LED_G;
		break;
	case 0x6:
		symbol = LED_A | LED_C | LED_D | LED_E | LED_F | LED_G;
		break;
	case 0x7:
		symbol = LED_A | LED_B | LED_C;
		break;
	case 0x8:
		symbol = LED_A | LED_B | LED_C | LED_D | LED_E | LED_F | LED_G;
		break;
	case 0x9:
		symbol = LED_A | LED_B | LED_C | LED_D | LED_F | LED_G;
		break;
	case 0xa:
		symbol = LED_A | LED_B | LED_C | LED_E | LED_F | LED_G;
		break;
	case 0xb:
		symbol = LED_C | LED_D | LED_E | LED_F | LED_G;
		break;
	case 0xc:
		symbol = LED_A | LED_D | LED_E | LED_F;
		break;
	case 0xd:
		symbol = LED_B | LED_C | LED_D | LED_E | LED_G;
		break;
	case 0xe:
		symbol = LED_A | LED_D | LED_E | LED_F | LED_G;
		break;
	case 0xf:
		symbol = LED_A | LED_E | LED_F | LED_G;
		break;
	}
	_sr(symbol + isSecondDigit*128, 0x104);
}

int main()
{
	int T = 0, last = 0, C = 0;
	while(1)
	{
		T = _lr(0x100);
		if(T - last >= 100)
		{
			C = (C + 1) % 256;
			writeHexDigit(C%16, 0);
			writeHexDigit(C/16, 1);
			last = T;
		}
	}
	return 0;
}

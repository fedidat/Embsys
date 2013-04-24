void writeHexDigit(char d, int isSecondDigit)
{
	unsigned int symbol;
	switch(d)
	{
	case 0:
	default:
		symbol = 63;
		break;
	case 1:
		symbol = 6;
		break;
	case 2:
		symbol = 91;
		break;
	case 3:
		symbol = 79;
		break;
	case 4:
		symbol = 102;
		break;
	case 5:
		symbol = 109;
		break;
	case 6:
		symbol = 125;
		break;
	case 7:
		symbol = 7;
		break;
	case 8:
		symbol = 127;
		break;
	case 9:
		symbol = 111;
		break;
	case 10:
		symbol = 119;
		break;
	case 11:
		symbol = 124;
		break;
	case 12:
		symbol = 57;
		break;
	case 13:
		symbol = 94;
		break;
	case 14:
		symbol = 121;
		break;
	case 15:
		symbol = 113;
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
		if((T - last >= 0 && T - last >= 100)
			|| (T - last < 0 && T - last <= -100))
		{
			C = (C + 1) % 256;
			writeHexDigit(C%16, 0);
			writeHexDigit(C/16, 1);
			last = T;
		}
	}
	return 0;
}

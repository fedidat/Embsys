void replace (int *num1, int *num2)
{
	int temp;

temp = *num1;
*num1 = *num2;
*num2 = temp;
}


void modify(int list[], int size)
{
	int out, in;

	for(out=0; out<size; out++)
	{
		for(in=out+1; in<size; in++)
		{
			if(list[out] > list[in])
			{
				replace (&list[out], &list[in]);
			}
		}
	}
}



int data[] = {4, 8, 2, 6, 7, 8, 10, 5};

int main (void)
{
	int nItems = sizeof(data) / sizeof(int);

	modify(data, nItems);

	return 0;
}


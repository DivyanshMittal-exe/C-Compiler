int a[3];

int
main()
{

a[0] = 0;
a[1] = 1;
a[2] = 2;
	if (a[0] != 0)
		return 1;
	if (a[1] != 1)
		return 2;
	if (a[2] != 2)
		return 3;
	
	return 0;
}

int printf(const char *format, ...);
int strcpy(char *dest, const char *src);

int main()
{
   char a[10];
   strcpy(a, "abcdefghij");
   printf("%s\n", &a[1]);

   return 0;
}

/* vim: set expandtab ts=4 sw=3 sts=3 tw=80 :*/


int printf(char const *format, ...);

int main() {

  int a = 1;
  int b = a;
  char t = 's';
  char *s = "Hello";
  char j = s[1];
  s[2] = 'a';
  printf("j = %c\n", j);
  printf("s = %s\n", s);
  // int c = 1, d = 2;
  // if (c != 2) {
  //   d = 3;
  // } else {
  //   d = 4;
  // }
  // printf("hello, world\n");
  // printf("d = %d\n", d);
  // int x = 1;
  //
  // while (x < 4) {
  //   printf("x = %d\n", x);
  //   x = x + 1;
  // }

  return 0;
}

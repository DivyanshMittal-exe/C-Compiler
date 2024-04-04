
int printf(char const *format, ...);

int main() {
  int a;
  a = 1;
  int b;
  b = a;
  int c = 1, d = 2;
  a = c + d;
  printf("hello, world\n");
  printf("a = %d\n", a);
  return 0;
}

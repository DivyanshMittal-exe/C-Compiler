

// int factorial(int n) {
//   if (n == 0) {
//     return 1;
//   } else {
//
//     int j = factorial(n - 1);
//     int v = n * j;
//     return v;
//   }
// }
//

int ver(int n) {
  if (n == 0) {
    return 1;
  } else {
    return n * ver(n - 1);
  }
}

int printf(char const *format, ...);

int main() {

  // char *s = "hellosadasdsadsadsad";
  //
  int k = 0;
  printf("k = %d\n", k);
  k = ver(5);
  printf("k = %d\n", k);
  // int j = 0;
  // s[j] = 'a';
  // char t = s[1];
  // printf("t = %c\n", t);
  // printf("s = %s\n", s);
  // int c = 1, d = 2;
  // if (c != 2) {
  //   d = 3;
  // } else {
  //   d = 4;
  // }
  // printf("hello, world\n");
  // printf("d = %d\n", d);
  // int x = 1;
  // while (x < 4) {
  //   printf("x = %d\n", x);
  //   x = x + 1;
  // }

  return 0;
}

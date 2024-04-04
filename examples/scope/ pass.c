int x;

void foo(int foo) {}

void bar(int n) {
  int bar;
  int i = 1;
  for (int i = 0; i < n; i++) {
    int i = 2;
    {
      {
        {
          {
            {
              {
                { i = i + 3; }
              }
            }
          }
        }
      }
    }
  }
}

void bar2(int all) {
  int x;
  int y;
  if (x) {
    int x;
    if (y) {
      int y;
      int x;
      while (x) {
        int x;
        for (int x; x++;) {
          for (int t; t++;) {
            switch (x) {
            case 1:
              break;
            default:
              break;
            }
          }
        }
      }
    }
  }
}

int main2(int a, int b, int c) {
  int x = 1;
  if (x) {
    if (x) {
      a = a + 1;
    }
  }
  return a;
}

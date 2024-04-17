# cc
C compiler

The C language grammar (c.y and c.l files) have been taken from:

http://www.quut.com/c/ANSI-C-grammar-y-2011.html


# Known issues

```int strlen(char *)``` is not supported, use
```int strlen(char * s)```


pointer arithmetic not supported

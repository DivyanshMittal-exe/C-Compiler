; ModuleID = 'global_module'
source_filename = "global_module"

@str = global [13 x i8] c"Hello World\0A\00"
@str.1 = global [3 x i8] c"%s\00"

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %s = alloca i8*, align 8
  store i8* getelementptr inbounds ([13 x i8], [13 x i8]* @str, i32 0, i32 0), i8** %s, align 8
  %0 = load i8*, i8** %s, align 8
  %1 = getelementptr i8, i8* %0, i32 1
  store i8 111, i8* %1, align 1
  %2 = load i8*, i8** %s, align 8
  %3 = getelementptr i8, i8* %2, i32 1
  %4 = load i8, i8* %3, align 1
  %c = alloca i8, align 1
  store i8 %4, i8* %c, align 1
  %c1 = load i8, i8* %c, align 1
  %5 = load i8*, i8** %s, align 8
  %6 = getelementptr i8, i8* %5, i32 2
  store i8 %c1, i8* %6, align 1
  %s2 = load i8*, i8** %s, align 8
  %printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @str.1, i32 0, i32 0), i8* %s2)
  ret i32 0
  ret i32 0
}

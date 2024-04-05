; ModuleID = 'global_module'
source_filename = "global_module"

@str = constant [6 x i8] c"Hello\00"
@str.1 = constant [8 x i8] c"j = %c\0A\00"
@str.2 = constant [8 x i8] c"s = %s\0A\00"

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %a = alloca i32, align 4
  store i32 1, i32* %a, align 4
  %a1 = load i32, i32* %a, align 4
  %b = alloca i32, align 4
  store i32 %a1, i32* %b, align 4
  %t = alloca i8, align 1
  store i8 115, i8* %t, align 1
  %s = alloca i8*, align 8
  store i8* getelementptr inbounds ([6 x i8], [6 x i8]* @str, i32 0, i32 0), i8** %s, align 8
  %s2 = load i8*, i8** %s, align 8
  %0 = getelementptr i8, i8* %s2, i32 1
  %1 = load i8, i8* %0, align 1
  %j = alloca i8, align 1
  store i8 %1, i8* %j, align 1
  %2 = getelementptr i8*, i8** %s, i32 2
  %3 = load i8*, i8** %2, align 8
  store i8 97, i8* %3, align 1
  %j3 = load i8, i8* %j, align 1
  %4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @str.1, i32 0, i32 0), i8 %j3)
  %s4 = load i8*, i8** %s, align 8
  %5 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @str.2, i32 0, i32 0), i8* %s4)
  ret i32 0
  ret i32 0
}

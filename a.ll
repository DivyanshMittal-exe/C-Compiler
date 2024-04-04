; ModuleID = 'global_module'
source_filename = "global_module"

@str = constant [17 x i8] c"\22hello, world\\n\22\00"
@str.1 = constant [11 x i8] c"\22a = %d\\n\22\00"

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %a = alloca i32, align 4
  store i32 0, i32* %a, align 4
  store i32 1, i32* %a, align 4
  %b = alloca i32, align 4
  store i32 0, i32* %b, align 4
  %a1 = load i32, i32* %a, align 4
  store i32 %a1, i32* %b, align 4
  %c = alloca i32, align 4
  store i32 1, i32* %c, align 4
  %d = alloca i32, align 4
  store i32 2, i32* %d, align 4
  %c2 = load i32, i32* %c, align 4
  %d3 = load i32, i32* %d, align 4
  %add = add i32 %c2, %d3
  store i32 %add, i32* %a, align 4
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @str, i32 0, i32 0))
  %a4 = load i32, i32* %a, align 4
  %1 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @str.1, i32 0, i32 0), i32 %a4)
  ret i32 0
  ret i32 0
}

; ModuleID = 'global_module'
source_filename = "global_module"

@str = global [4 x i8] c"yo\0A\00"

declare i32 @printf(i8*, ...)

define void @fred() {
entry:
  %printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str, i32 0, i32 0))
  ret void
}

define i32 @main() {
entry:
  call void @fred()
  %return_alloca = alloca i32, align 4
  ret i32 0
  ret i32 0
}

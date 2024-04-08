; ModuleID = 'global_module'
source_filename = "global_module"

@str = global [8 x i8] c"k = %d\0A\00"
@str.1 = global [8 x i8] c"k = %d\0A\00"

define i32 @ver(i32 %0) {
entry:
  %n = alloca i32, align 4
  store i32 %0, i32* %n, align 4
  %n1 = load i32, i32* %n, align 4
  %equal = icmp eq i32 %n1, 0
  %ifcond = icmp ne i1 %equal, false
  br i1 %ifcond, label %then, label %else

then:                                             ; preds = %entry
  ret i32 1
  br label %ifcont

else:                                             ; preds = %entry
  %n2 = load i32, i32* %n, align 4
  %n3 = load i32, i32* %n, align 4
  %sub = sub i32 %n3, 1
  %ver_call = call i32 @ver(i32 %sub)
  %mul = mul i32 %n2, %ver_call
  ret i32 %mul
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  ret i32 0
}

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %k = alloca i32, align 4
  store i32 0, i32* %k, align 4
  %k1 = load i32, i32* %k, align 4
  %printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @str, i32 0, i32 0), i32 %k1)
  %ver_call = call i32 @ver(i32 5)
  store i32 %ver_call, i32* %k, align 4
  %k2 = load i32, i32* %k, align 4
  %printf_call3 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @str.1, i32 0, i32 0), i32 %k2)
  ret i32 0
  ret i32 0
}

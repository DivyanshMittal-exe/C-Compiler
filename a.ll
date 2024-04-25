; ModuleID = 'global_module'
source_filename = "global_module"

@str = global [10 x i8] c"char: %c\0A\00"
@str.1 = global [9 x i8] c"int: %d\0A\00"
@str.2 = global [11 x i8] c"float: %f\0A\00"
@str.3 = global [7 x i8] c"%d %d\0A\00"
@str.4 = global [7 x i8] c"%d %d\0A\00"
@str.5 = global [7 x i8] c"%f %f\0A\00"

declare i32 @printf(i8*, ...)

define void @charfunc(i8 %0) {
entry:
  %a = alloca i8, align 1
  store i8 %0, i8* %a, align 1
  %a1 = load i8, i8* %a, align 1
  %printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @str, i32 0, i32 0), i8 %a1)
  ret void
}

define void @intfunc(i32 %0) {
entry:
  %a = alloca i32, align 4
  store i32 %0, i32* %a, align 4
  %a1 = load i32, i32* %a, align 4
  %printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([9 x i8], [9 x i8]* @str.1, i32 0, i32 0), i32 %a1)
  ret void
}

define void @floatfunc(double %0) {
entry:
  %a = alloca double, align 8
  store double %0, double* %a, align 8
  %a1 = load double, double* %a, align 8
  %printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([11 x i8], [11 x i8]* @str.2, i32 0, i32 0), double %a1)
  ret void
}

define i32 @main() {
entry:
  call void @charfunc(i8 97)
  call void @charfunc(i32 98)
  call void @charfunc(double 9.900000e+01)
  call void @intfunc(i8 97)
  call void @intfunc(i32 98)
  call void @intfunc(double 9.900000e+01)
  call void @floatfunc(i8 97)
  call void @floatfunc(i32 98)
  call void @floatfunc(double 9.900000e+01)
  %b = alloca i8, align 1
  store i8 97, i8* %b, align 1
  %c = alloca i8, align 1
  store i8 97, i8* %c, align 1
  %printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @str.3, i32 0, i32 0), i32 97, double 9.700000e+01)
  %d = alloca i32, align 4
  store i32 97, i32* %d, align 4
  %e = alloca i32, align 4
  store i32 97, i32* %e, align 4
  %d1 = load i32, i32* %d, align 4
  %printf_call2 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @str.4, i32 0, i32 0), i32 %d1, double 9.700000e+01)
  %f = alloca double, align 8
  store double 9.700000e+01, double* %f, align 8
  %g = alloca double, align 8
  store double 9.700000e+01, double* %g, align 8
  %f3 = load double, double* %f, align 8
  %printf_call4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @str.5, i32 0, i32 0), double %f3, i32 97)
  %return_alloca = alloca i32, align 4
  ret i32 0
  ret i32 0
}

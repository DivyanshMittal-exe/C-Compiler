; ModuleID = 'global_module'
source_filename = "global_module"

@str = global [4 x i8] c"%d\0A\00"
@str.1 = global [4 x i8] c"%d\0A\00"
@str.2 = global [4 x i8] c"%d\0A\00"
@str.3 = global [4 x i8] c"%d\0A\00"

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %Count = alloca i32, align 4
  store i32 0, i32* %Count, align 4
  br label %for.init

for.init:                                         ; preds = %entry
  store volatile i32 0, i32* %Count, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.iter8, %for.init
  %Count1 = load i32, i32* %Count, align 4
  %lt = icmp slt i32 %Count1, 4
  %make_bool = icmp ne i1 %lt, false
  br i1 %make_bool, label %for.body2, label %for.merge9

for.body2:                                        ; preds = %for.cond
  %Count3 = load i32, i32* %Count, align 4
  %printf_call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str, i32 0, i32 0), i32 %Count3)
  %Count4 = load i32, i32* %Count, align 4
  br label %switch

switch:                                           ; preds = %for.body2
  switch i32 %Count4, label %default [
    i32 1, label %case0
    i32 2, label %case2
  ]

case0:                                            ; preds = %switch
  %printf_call5 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str.1, i32 0, i32 0), i32 1)
  br label %switchcont

case2:                                            ; preds = %switch
  %printf_call6 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str.2, i32 0, i32 0), i32 2)
  br label %switchcont

default:                                          ; preds = %switch
  %printf_call7 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @str.3, i32 0, i32 0), i32 0)
  br label %switchcont

switchcont:                                       ; preds = %default, %switchcont, %switchcont, %case2, %switchcont, %case0
  br label %switchcont
  br label %switchcont
  br label %switchcont
  br label %for.iter8

for.iter8:                                        ; preds = %switchcont
  %INC_OP_LOAD2 = load i32, i32* %Count, align 4
  %inc = add i32 %INC_OP_LOAD2, 1
  store i32 %inc, i32* %Count, align 4
  br label %for.cond

for.merge9:                                       ; preds = %for.cond
  %return_alloca = alloca i32, align 4
  ret i32 0
  ret i32 0
}

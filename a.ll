; ModuleID = 'global_module'
source_filename = "global_module"

@c = global i32 0

define i32 @main() {
entry:
  br i1 false, label %then, label %else

then:                                             ; preds = %entry
  %return_alloca = alloca i32, align 4
  ret i32 1
  br label %ifcont12

else:                                             ; preds = %entry
  br i1 false, label %then1, label %else2

then1:                                            ; preds = %else
  br label %ifcont11

else2:                                            ; preds = %else
  br i1 true, label %then3, label %else8

then3:                                            ; preds = %else2
  %c = load i32, i32* @c, align 4
  %make_bool = icmp ne i32 %c, 0
  %ifcond = icmp ne i1 %make_bool, false
  br i1 %ifcond, label %then4, label %else6

then4:                                            ; preds = %then3
  %return_alloca5 = alloca i32, align 4
  ret i32 1
  br label %ifcont

else6:                                            ; preds = %then3
  %return_alloca7 = alloca i32, align 4
  ret i32 0
  br label %ifcont

ifcont:                                           ; preds = %else6, %then4
  br label %ifcont10

else8:                                            ; preds = %else2
  %return_alloca9 = alloca i32, align 4
  ret i32 1
  br label %ifcont10

ifcont10:                                         ; preds = %else8, %ifcont
  br label %ifcont11

ifcont11:                                         ; preds = %ifcont10, %then1
  br label %ifcont12

ifcont12:                                         ; preds = %ifcont11, %then
  %return_alloca13 = alloca i32, align 4
  ret i32 1
  ret i32 0
}

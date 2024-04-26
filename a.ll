; ModuleID = 'global_module'
source_filename = "global_module"

@str = global [5 x i8] c"else\00"
@str.1 = global [33 x i8] c"It should be called tree-shaking\00"

declare i32 @puts(i8*)

define void @func() {
entry:
  ret void
}

define void @const_folding_simplify_check() {
entry:
  %simple = alloca i32, align 4
  store i32 0, i32* %simple, align 4
  %tricky_1 = alloca i32, align 4
  store i32 0, i32* %tricky_1, align 4
  %tricky_2 = alloca i32, align 4
  store i32 0, i32* %tricky_2, align 4
  %tricky_3 = alloca i32, align 4
  store i32 0, i32* %tricky_3, align 4
  store volatile i32 14, i32* %simple, align 4
  store volatile i32 0, i32* %simple, align 4
  store volatile i32 0, i32* %tricky_1, align 4
  store volatile i32 0, i32* %tricky_2, align 4
  store volatile i32 0, i32* %tricky_3, align 4
  ret void
}

define void @remove_deadcode() {
entry:
  %simple = alloca i32, align 4
  store i32 0, i32* %simple, align 4
  store volatile i32 10, i32* %simple, align 4
  %puts_call = call i32 @puts(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str, i32 0, i32 0))
  %puts_call1 = call i32 @puts(i8* getelementptr inbounds ([33 x i8], [33 x i8]* @str.1, i32 0, i32 0))
  ret void
}

define i32 @main(i32 %0, i8** %1) {
entry:
  %argc = alloca i32, align 4
  store i32 %0, i32* %argc, align 4
  %argv = alloca i8**, align 8
  store i8** %1, i8*** %argv, align 8
  call void @const_folding_simplify_check()
  call void @remove_deadcode()
  %return_alloca = alloca i32, align 4
  ret i32 0
  ret i32 0
}

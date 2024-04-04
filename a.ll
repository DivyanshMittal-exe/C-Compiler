; ModuleID = 'global_module'
source_filename = "global_module"

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
  ret i32 0
  ret i32 0
}

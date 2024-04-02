; ModuleID = 'global_module'
source_filename = "global_module"

define i32 @main() {
entry:
  %a = alloca i32, align 4
  store i32 0, i32* %a, align 4
  store i32 1, i32* %a, align 4
  %b = alloca i32, align 4
  store i32 0, i32* %b, align 4
  store i32* %a, i32* %b, align 8
  %c = alloca i32, align 4
  store i32 1, i32* %c, align 4
  %d = alloca i32, align 4
  store i32 2, i32* %d, align 4
  ret void
  ret i32 0
}

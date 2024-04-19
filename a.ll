; ModuleID = 'global_module'
source_filename = "global_module"

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 0, i32* %x, align 4
  store volatile i32 4, i32* %x, align 4
  ret i32 0
}

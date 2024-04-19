; ModuleID = 'global_module'
source_filename = "global_module"

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 0, i32* %x, align 4
  %p = alloca i32*, align 8
  store i32* null, i32** %p, align 8
  %pp = alloca i32**, align 8
  store i32** null, i32*** %pp, align 8
  store volatile i32 0, i32* %x, align 4
  store volatile i32* %x, i32** %p, align 8
  store volatile i32** %p, i32*** %pp, align 8
  %p1 = load i32*, i32** %p, align 8
  %deref = load i32, i32* %p1, align 4
  %make_bool = icmp ne i32 %deref, 0
  %ifcond = icmp ne i1 %make_bool, false
  br i1 %ifcond, label %then, label %else

then:                                             ; preds = %entry
  %return_alloca = alloca i32, align 4
  ret i32 1
  br label %ifcont

else:                                             ; preds = %entry
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %pp2 = load i32**, i32*** %pp, align 8
  %deref3 = load i32*, i32** %pp2, align 8
  %deref4 = load i32, i32* %deref3, align 4
  %make_bool5 = icmp ne i32 %deref4, 0
  %ifcond6 = icmp ne i1 %make_bool5, false
  br i1 %ifcond6, label %then7, label %else9

then7:                                            ; preds = %ifcont
  %return_alloca8 = alloca i32, align 4
  ret i32 1
  br label %ifcont12

else9:                                            ; preds = %ifcont
  %deref10 = load i32**, i32*** %pp, align 8
  %deref11 = load i32*, i32** %deref10, align 8
  store volatile i32 1, i32* %deref11, align 4
  br label %ifcont12

ifcont12:                                         ; preds = %else9, %then7
  %return_alloca13 = alloca i32, align 4
  ret i32 1
  ret i32 0
}

; ModuleID = './stress/00015.c'
source_filename = "./stress/00015.c"
target datalayout = "e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-pc-linux-gnu"

@.str = private unnamed_addr constant [13 x i8] c"Hello World\0A\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c"%s\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i8*, align 4
  %3 = alloca i8, align 1
  store i32 0, i32* %1, align 4
  store i8* getelementptr inbounds ([13 x i8], [13 x i8]* @.str, i32 0, i32 0), i8** %2, align 4
  %4 = load i8*, i8** %2, align 4
  %5 = getelementptr inbounds i8, i8* %4, i32 1
  store i8 111, i8* %5, align 1

  %6 = load i8*, i8** %2, align 4
  %7 = getelementptr inbounds i8, i8* %6, i32 1
  %8 = load i8, i8* %7, align 1
  store i8 %8, i8* %3, align 1
  
  %9 = load i8, i8* %3, align 1
  %10 = load i8*, i8** %2, align 4
  %11 = getelementptr inbounds i8, i8* %10, i32 2
  store i8 %9, i8* %11, align 1
  %12 = load i8*, i8** %2, align 4
  %13 = call i32 (i8*, ...) @printf(i8* noundef getelementptr inbounds ([3 x i8], [3 x i8]* @.str.1, i32 0, i32 0), i8* noundef %12)
  ret i32 0
}

declare i32 @printf(i8* noundef, ...) #1

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="i686" "target-features"="+cx8,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="i686" "target-features"="+cx8,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4, !5}
!llvm.ident = !{!6}

!0 = !{i32 1, !"NumRegisterParameters", i32 0}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 7, !"PIC Level", i32 2}
!3 = !{i32 7, !"PIE Level", i32 2}
!4 = !{i32 7, !"uwtable", i32 1}
!5 = !{i32 7, !"frame-pointer", i32 2}
!6 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}

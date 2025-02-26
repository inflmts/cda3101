################################################################################
#
#   pa2.s
#   Author: Daniel Li
#   Date: 02/28/2025
#
################################################################################
#
#@  set -e
#@  gcc -o pa2 pa2.s -g -static
#@  for i in -1 0 1 2 3 4 5 6 7 8 9 10 11; do
#@    echo "[testing $i]"
#@    echo $i | ./pa2
#@  done
#@  exit
#
#   Objective
#   ---------
#
#   Use the following algorithm to calculate the nth number in the fibonacci
#   sequence. Inputs will be from 1 to 10. Input of 1 should return 0, input of
#   2 should return 1, input of 3 should return 1, input of 4 should return 2,
#   input of 5 should return 3, and so on.
#
#   procedure fib(argument: n)
#     return fib(n-2) + fib(n-1)
#
#   You must use this assignment. Yes, I know every caution you've been given
#   to NEVER use such an algorithm. This is an exercise in understanding
#   recursion and calling procedures in assembly language.
#
#   If you use a loop or an equivalent to a case/switch table, your score on
#   the assignment will be 20/100.
#
#   Requirements and Specifications
#   -------------------------------
#
#   Your input cannot be stored as a global variable at any point. This means
#   you cannot store it at a data section address, even when accepting them
#   from scanf; they must be returned from scanf on the stack.
#
#   X19-X27 are global variables. You may not use X19-X27 in your recursive
#   function. If you want, you may use X19-X27 in your main function. You can
#   use any registers you want to in your main function.
#
#   A recursion procedure requires:
#
#   - Allocate stack space
#   - Save the return address and argument on the stack
#   - Recursively call procedure with BL
#   - Unwind the stack by restoring the return address and arguments and deallocating stack memory
#
#   If the code is not recursive, i.e. uses a loop or a the equivalent of a
#   case/switch table, your score will be 20/100.
#
#   Hints and Warnings
#   ------------------
#
#   You must put the local values and return address on the stack before any bl
#   call, and restore the argument and the return address after the bl call.
#
#   If you copy code from any source, e.g. friend, ai tool, stack overflow,
#   this is an honor code violation and you will fail the class.
#
#   Warning:
#   - The use of C/C++ functions except for printf and scanf is strictly prohibited.
#   - The use of automated tools (e.g. GCC compiler) to turn C code into ARM is strictly prohibited.
#   - Copy pasting code from any source is an honor code violation and will result in failing the class
#
################################################################################

.section .data
input_prompt:   .asciz "Please enter a number betwen 1 and 10 \n"
input_spec:     .asciz "%lld"
output_spec:    .asciz "%lld\n"
oob_msg:        .asciz "Input is out of bounds \n"

.section .text
.global main

fib:
  # if the argument is 1, return 0
  cmp x0, #1
  b.eq fib1

  # if the argument is 2, return 1
  cmp x0, #2
  b.eq fib2

  # save return address and argument
  sub sp, sp, #24
  str x30, [sp, #0]
  str x0, [sp, #8]

  # call fib(n-2) and save result
  sub x0, x0, #2
  bl fib
  str x0, [sp, #16]

  # call fib(n-1) and add to result
  ldr x0, [sp, #8]
  sub x0, x0, #1
  bl fib
  mov x1, x0
  ldr x0, [sp, #16]
  add x0, x0, x1

  # restore stack and return
  ldr x30, [sp, #0]
  add sp, sp, #24
  ret

fib1:
  mov x0, #0
  ret

fib2:
  mov x0, #1
  ret

oob:
  ldr x0, =oob_msg
  bl printf
  b exit

main:
  # allocate stack space
  sub sp, sp, #8

  # prompt for input
  ldr x0, =input_prompt
  bl printf

  # get input
  ldr x0, =input_spec
  add x1, sp, #0
  bl scanf

  # check input
  ldr x0, [sp, #0]
  cmp x0, #1
  b.lt oob
  cmp x0, #10
  b.gt oob

  # call fib
  bl fib

  # print result
  mov x1, x0
  ldr x0, =output_spec
  bl printf

  # restore stack
  add sp, sp, #8

# branch to this label on program completion
exit:
#!ldr x0, stdout
#!bl fflush

  mov x0, 0
  mov x8, 93
  svc 0
  ret
